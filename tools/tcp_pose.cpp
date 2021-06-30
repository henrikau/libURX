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

void usage(const std::string bname, int retcode)
{
    std::cout << "Usage: " << std::endl;
    std::cout << bname << "[-i UR_Controller_IPv4 -f URScript.file] | [-h help]" << std::endl;
    exit(retcode);
}

int main(int argc, char *argv[])
{
    std::atomic<bool> running(true);

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


    // target pose for TCP in base frame
    // x,y,z [m]  ,   rx,ry,rz [rad]
    std::vector<double> start_pose(urx::DOF);
    start_pose[0] =  0.43;
    start_pose[1] = -0.57;
    start_pose[2] =  0.58;
    start_pose[3] =  1.4;
    start_pose[4] = -1.1;
    start_pose[5] =  1.3;
    //start_pose[3] = urx::deg_to_rad( 90.0);
    //start_pose[4] = urx::deg_to_rad(    0);
    //start_pose[5] = urx::deg_to_rad(    0);

    std::vector<double> end_pose(urx::DOF);
    end_pose[0] =  0.8;
    end_pose[1] = -0.7;
    end_pose[2] =  0.7;
    end_pose[3] = urx::deg_to_rad(    0);
    end_pose[4] = urx::deg_to_rad( 90.0);
    end_pose[5] = urx::deg_to_rad(    0);

    std::vector<std::vector<double>> target_poses = trace_linear(start_pose, end_pose, 100);

    int step = 0;

    urx::Robot_State state;
    for(int i = 0; i < (int)target_poses.size(); i++){
        state = robot.state();

        double err;
        for(int j = 0; j < (int)target_poses[i].size(); j++){
            err = target_poses[i][j] - state.tcp_pose[j];
            std::cout << std::setw(14) << std::left << err << " ";
        }
        std::cout << "   " << step++ << std::endl;
        
        robot.update_TCP_pose(target_poses[i]);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    while (running) {
        urx::Robot_State state = robot.state();

        double err;
        for(int j = 0; j < (int)state.tcp_pose.size(); j++){
            err = target_poses.back()[j] - state.tcp_pose[j];
            std::cout << std::setw(14) << std::left << err << " ";
        }
        std::cout << "   " << step++ << std::endl;

        // if all errors are ok, we are done
        if (close_vec(state.tcp_pose, end_pose, 0.0001)) {
            running = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "Position reached, terminating program." << std::endl;

    robot.stop();
    return 0;
}
