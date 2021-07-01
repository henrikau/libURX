/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#include <urx/robot.hpp>
#include <urx/helper.hpp>
#include <unistd.h>

#include <chrono>   
#include <thread>
#include <iomanip> 


std::vector<std::vector<double>> trace_linear(std::vector<double> start_pose, std::vector<double> end_pose, int steps){

    std::vector<std::vector<double>> target_poses(steps);
    std::vector<double> pose(urx::DOF);

    for(int i = 0; i < steps; i++){
        for(int j = 0; j < (int)urx::DOF; j++){
            pose[j] = start_pose[j] + i*(end_pose[j] - start_pose[j]) / (steps-1);
        }
        target_poses[i] = pose;
    }
    return target_poses;
}

double new_speed(const double err, const double prev_err, const double prev_speed)
{
    constexpr double k_p = 0.7;
    constexpr double k_i = 0.005;
    constexpr double k_d = 1.1;

    double p = k_p * err;
    double i = k_i *  prev_speed; // accumlated speed, not really summed error
    double d = (err - prev_err * k_d)/k_d;
    return  p + i + d;
}

void usage(const std::string bname, int retcode)
{
    std::cout << "Usage: " << std::endl;
    std::cout << bname << "[-i UR_Controller_IPv4 -f URScript.file] | [-h help]" << std::endl;
    exit(retcode);
}

int main(int argc, char *argv[])
{
    std::atomic<bool> running(true);
    std::vector<double> w(urx::DOF);;
    std::vector<double> prev_err(urx::DOF);

    char ip4[16] = {0};
    char sfile[256] = {0};
    int opt;
    while ((opt = getopt(argc, argv, "i:f:h")) != -1) {
        switch (opt) {
        case 'i':
            strncpy(ip4, optarg, 15);
            break;
        case 'f':
            strncpy(sfile, optarg, 255);
            break;
        case 'h':
            usage(argv[0], 0);
            break;
        }

    }

    if (strlen(ip4) == 0 || strlen(sfile) == 0) {
        std::cerr << "Need IP of remote to connect to and script to upload!" << std::endl;
        usage(argv[0], 1);
    }

    // Robot startup
    urx::Robot robot = urx::Robot(ip4);
    if (!robot.init_output_TCP_pose() ||
        !robot.init_input_TCP_pose() ||
        !robot.upload_script(sfile))   {
        std::cout << "urx::Robot init failed" << std::endl;
        return 2;
    }
    if (!robot.start()) {
        std::cout << "urx::Robot startup failed" << std::endl;
        return 3;
    }


//    // target pose for TCP in base frame
//    // x,y,z [m]  ,   rx,ry,rz [rad]
//    std::vector<double> start_pose(urx::DOF);
//    start_pose[0] =  0.43;
//    start_pose[1] = -0.57;
//    start_pose[2] =  0.58;
//    start_pose[3] =  1.4;
//    start_pose[4] = -1.1;
//    start_pose[5] =  1.3;
//    //start_pose[3] = urx::deg_to_rad( 90.0);
//    //start_pose[4] = urx::deg_to_rad(    0);
//    //start_pose[5] = urx::deg_to_rad(    0);
//
    std::vector<double> q_ref(urx::DOF);
    std::vector<double> end_pose(urx::DOF);
    end_pose[0] =  0.6;
    end_pose[1] = -0.6;
    end_pose[2] =  0.6;
    end_pose[3] = urx::deg_to_rad( 90.0);
    end_pose[4] = urx::deg_to_rad(    0);
    end_pose[5] = urx::deg_to_rad(    0);
//
//    std::vector<std::vector<double>> target_poses = trace_linear(start_pose, end_pose, 100);
//
//    int step = 0;
//
//    urx::Robot_State state;
//    for(int i = 0; i < (int)target_poses.size(); i++){
//        state = robot.state();
//
//        double err;
//        for(int j = 0; j < (int)target_poses[i].size(); j++){
//            err = target_poses[i][j] - state.tcp_pose[j];
//            std::cout << std::setw(14) << std::left << err << " ";
//        }
//        std::cout << "   " << step++ << std::endl;
//        
//        robot.update_TCP_pose_ref(target_poses[i]);
//
//        std::this_thread::sleep_for(std::chrono::milliseconds(200));
//    }

    robot.update_TCP_pose_ref(end_pose);
    sleep(5);
    double* q_ref_arr = robot.get_q_ref();
    for(int i = 0; i < (int)q_ref.size(); i++){
        q_ref[i] = q_ref_arr[i];
        std::cout << q_ref[i] << " ";
    }
    std::cout << std::endl;

    // target position for joints
    //q_ref[urx::BASE]     = urx::deg_to_rad( 180.0);
    //q_ref[urx::SHOULDER] = urx::deg_to_rad(-135.0);
    //q_ref[urx::ELBOW]    = urx::deg_to_rad(  90.0);
    //q_ref[urx::W1]       = urx::deg_to_rad( -90.0);
    //q_ref[urx::W2]       = urx::deg_to_rad(  90.0);
    //q_ref[urx::W3]       = urx::deg_to_rad( -90.0);

    while (running) {
//        urx::Robot_State state = robot.state();
//
//        double err;
//        for(int j = 0; j < (int)state.tcp_pose.size(); j++){
//            err = target_poses.back()[j] - state.tcp_pose[j];
//            std::cout << std::setw(14) << std::left << err << " ";
//        }
//        std::cout << "   " << step++ << std::endl;
//
//        // if all errors are ok, we are done
//        if (close_vec(state.tcp_pose, end_pose, 0.0001)) {
//            running = false;
//        }
//        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        urx::Robot_State state = robot.state();
        for (std::size_t i = 0; i < urx::DOF; ++i) {
            double err = q_ref[i] - state.jq[i];
            w[i] = new_speed(err, prev_err[i], w[i]);
            prev_err[i] = err;
        }

        // if all errors are ok, we are done (check errors in q_ref or tcp_pose_ref?)
        if (close_vec(state.jq, q_ref, 0.0001)) {
            running = false;
            for (std::size_t i = 0; i < urx::DOF; ++i)
                w[i] = 0.0;
        }
        robot.update_w(w);
    }

    std::cout << "Position reached, terminating program." << std::endl;

    robot.stop();
    return 0;
}
