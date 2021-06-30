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
    // target pose for TCP in base frame
    // x,y,z [m]  ,   rx,ry,rz [rad]
    std::vector<double> aq(urx::DOF);
    aq[urx::TCP_x]   =  0.5;
    aq[urx::TCP_y]   = -0.6;
    aq[urx::TCP_z]   =  0.6;
    aq[urx::TCP_rx]  = urx::deg_to_rad( 90.0);
    aq[urx::TCP_ry]  = urx::deg_to_rad(    0);
    aq[urx::TCP_rz]  = urx::deg_to_rad(    0);

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

    robot.update_TCP_pose(aq);

    while (running) {
        urx::Robot_State state = robot.state();

        // if all errors are ok, we are done
        if (close_vec(state.tcp_pose, aq, 0.0001)) {
            running = false;
            for (std::size_t i = 0; i < urx::DOF; ++i)
                w[i] = 0.0;
        }
    }

    std::cout << "Position reached, terminating program." << std::endl;

    robot.stop();
    return 0;
}
