/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#include <iostream>
#include <fstream>
#include <chrono>

#include <urx/rtde_handler.hpp>
#include <urx/helper.hpp>
#include <urx/urx_handler.hpp>
#include <unistd.h>

using namespace std::chrono;     // ugly, but makes using chrono a lot easier

void set_vec(double tq[], double nq[])
{
    for (size_t c = 0; c<urx::END;c++)
        tq[c] = nq[c];
}

bool close(const double a, const double b)
{
    const double e = 0.0001;
    double diff = a-b;
    return (diff < e) && (diff > -e);
}

bool close_vec(double actual[], double target[])
{
    for (size_t c = 0; c<urx::END;c++) {
        if (!close(actual[c], target[c])) {
            return false;
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " IP path/to/script" << std::endl;
        return -1;
    }

    double ts;
    double target_q[urx::END];
    double actual_q[urx::END];


    urx::RTDE_Handler h(argv[1]);

    h.connect_ur();
    if (!h.set_version()) {
        std::cout << "Setting version FAILED" << std::endl;
        return -1;
    }

    urx::RTDE_Recipe out = urx::RTDE_Recipe();
    out.add_field("timestamp", &ts);
    out.add_field("target_q", target_q);
    out.add_field("actual_q", actual_q);
    if (!h.register_recipe(&out)) {
        printf("%s: Failed setting output_recipe!\n", __func__);
        return -1;
    }

    // target position for joints
    double aq[] = { urx::deg_to_rad(   0.0), //
                    urx::deg_to_rad( -90.0), //
                    urx::deg_to_rad(  90.0), // ELBOW
                    urx::deg_to_rad( -90.0),
                    urx::deg_to_rad(   0.0),
                    urx::deg_to_rad(   0.0)};

    int32_t in_seqnr = 1;
    int32_t cmd = 2;
    double in_tqd[urx::END] {0.0};
    urx::RTDE_Recipe in = urx::RTDE_Recipe();

    in.dir_input();
    in.add_field("input_int_register_0", &in_seqnr);
    in.add_field("input_int_register_1", &cmd);
    in.add_field("input_double_register_0", &in_tqd[urx::BASE]);
    in.add_field("input_double_register_1", &in_tqd[urx::SHOULDER]);
    in.add_field("input_double_register_2", &in_tqd[urx::ELBOW]);
    in.add_field("input_double_register_3", &in_tqd[urx::W1]);
    in.add_field("input_double_register_4", &in_tqd[urx::W2]);
    in.add_field("input_double_register_5", &in_tqd[urx::W3]);

    if (!h.register_recipe(&in)) {
        printf("%s: Failed adding recipe as expected\n", __func__);
        return -1;
    }
    printf("Ready to start loop\n");
    h.start();

    // Make sure speed is 0 before we go
    in_tqd[urx::ELBOW]= 0.0;
    cmd = 2;
    h.send(in.recipe_id());
    cmd = 0;
    if (!h.send(in.recipe_id())) {
        printf("Failed sending data to remote\n");
    }

    urx::URX_Handler uh = urx::URX_Handler(argv[1]);
    if (!uh.upload_script(argv[2])) {
        std::cout << "Failed uploading script to UR Controller" << std::endl;
        return -1;
    }

    in_tqd[urx::ELBOW]= 0.5;
    cmd = 2;
    h.send(in.recipe_id());
    cmd = 0;
    if (!h.send(in.recipe_id())) {
        printf("Failed sending data to remote\n");
    }

    bool running = true;
    double speed = 0;
    double err = 0, prev_err = 0;

    double k_p = 0.7;
    double k_i = 0.005;
    double k_d = 1.1;
    int stabilizer = 25;       // period to make sure we are stable
    std::ofstream log_csv;
    log_csv.open("log.csv");
    log_csv << "k_p,"<<k_p<<std::endl;
    log_csv << "k_i,"<<k_i<<std::endl;
    log_csv << "k_d,"<<k_d<<std::endl;
    log_csv << "ts,target_q,actual_q,err,speed,p,i,d" << std::endl;

    microseconds start = duration_cast<microseconds >(system_clock::now().time_since_epoch());

    while (running) {
        h.recv();
        err = aq[urx::ELBOW] - actual_q[urx::ELBOW];
        double p = k_p * err;
        double i = k_i * speed; // accumlated speed, not really summed error
        double d = (err - prev_err * k_d)/k_d;
        speed = p + i + d;
        prev_err = err;
        microseconds now = duration_cast<microseconds >(system_clock::now().time_since_epoch());
        log_csv << now.count() << "," << aq[urx::ELBOW] << "," << actual_q[urx::ELBOW] << "," << err << "," << speed << "," << p << "," << i << "," << d << std::endl;
#if 0
        std::cout<< "Target: " << aq[urx::ELBOW] << \
            " | actual_q[ELBOW]: " << actual_q[urx::ELBOW] << \
            " | target_q[ELBOW]" << target_q[urx::ELBOW] << \
            " | diff: " << (target_q[urx::ELBOW] - actual_q[urx::ELBOW]) << std::endl;
#endif
        // while elbow is close to target, stop motion
        if (err < -0.00005 || err > 0.00005) {
            in_tqd[urx::ELBOW] = speed;
            cmd = 2;
            if (!h.send(in.recipe_id())) {
                printf("Failed updating speed\n");
            }
            // avoid excessive speed updates, robot cannot clear 
            cmd = 0;
            if (!h.send(in.recipe_id())) {
                printf("Failed sending data to remote\n");
            }
            stabilizer = 25;
        } else {
            if (--stabilizer <= 0) {
                std::cout << "Target REACHED!" << std::endl;
                h.send_emerg_msg("Target Reached, stopping robot");
                cmd = 3;
                in_tqd[urx::ELBOW] = 0.0;
                h.send(in.recipe_id());
                running = false;
            }
        }
    }
    // log_csv.close();
    // usleep(5000);
    // cmd = 3;
    // h.send(in.recipe_id());
    // usleep(5000);
    // cmd=3;
    // h.send(in.recipe_id());
    // usleep(5000);

    microseconds end = duration_cast<microseconds >(system_clock::now().time_since_epoch());
    seconds diff_s = duration_cast<seconds>(end-start);
    milliseconds diff_ms = duration_cast<milliseconds>(end-start);
    std::cout << "Robot reached target after " << diff_s.count() << "s (" << diff_ms.count() << " ms)"<< std::endl;
    cmd = 1;
    h.send(in.recipe_id());
    h.stop();
    printf("Robot stopped\n");
    return 0;
}
