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

constexpr int poseDimension = 6;

std::vector<std::vector<double>> trace_linear(std::vector<double> start_pose, std::vector<double> end_pose, int steps){

    std::vector<std::vector<double>> target_poses(steps);
    std::vector<double> pose(poseDimension);

    for(int i = 0; i < steps; i++){
        for(int j = 0; j < poseDimension; j++){
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
    urx::Robot robot(ip4);
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
    // [x,y,z,rx,ry,rz], translation is given in [m] and rotation in [rad] with axis-angle representation.
    std::vector<double> start_pose(poseDimension);
    start_pose[0] =  0.43;
    start_pose[1] = -0.57;
    start_pose[2] =  0.58;
    start_pose[3] =  1.4;
    start_pose[4] = -1.1;
    start_pose[5] =  1.3;

    std::vector<double> end_pose(poseDimension);
    end_pose[0] =  0.9;
    end_pose[1] = -0.7;
    end_pose[2] =  0.8;
    end_pose[3] =  1.8;
    end_pose[4] = -2.6;
    end_pose[5] =  2.5;

    std::vector<std::vector<double>> target_poses = trace_linear(start_pose, end_pose, 100);
    int step = 0;
    auto timestamp = std::chrono::system_clock::now().time_since_epoch();

    robot.calculate_q_ref(target_poses[step]);

    while (running) {
        urx::Robot_State state = robot.state();

        // Send new tcp pose ref when we are sufficiently close to the current reference
       // if (close_vec(state.jq, state.jq_ref, 1) && ((step+1) < (int)target_poses.size())) {
       //     step++;
       //     robot.calculate_q_ref(target_poses[step]);
       // } 

        // Send new tcp pose ref each 100ms
        auto now = std::chrono::system_clock::now().time_since_epoch();
        std::chrono::duration<double> diff = now - timestamp;
        if ( (diff.count() > 0.1) && ((step+1) < (int)target_poses.size())){
            timestamp = now;
            step++;
            robot.calculate_q_ref(target_poses[step]);
        }

        if(robot.q_ref_initialized()){
            // Calculate new angular speed for each joint
            for (std::size_t i = 0; i < urx::DOF; ++i) {
                double err = state.jq_ref[i] - state.jq[i];
                w[i] = new_speed(err, prev_err[i], w[i]);
                prev_err[i] = err;
            }
        } else{
            // If control reference is not valid yet, dont move
            w = {0,0,0,0,0,0};
        }

        // if we have reached the final control reference, we are done
        if ((step+1) == (int)target_poses.size() && 
            close_vec(state.jq_ref, state.jq, 0.01)) {
            running = false;

            // Simply send zero input (should not be needed if robot.stop() sends stop_robot cmd (which is not implemented yet)?)
            // This simple method also requires this for-loop to be at the end of the controller while-loop
            for (std::size_t i = 0; i < urx::DOF; ++i){
                w[i] = 0.0;
            }
        } 

        //std::cout << "setting speed " << cout++ << std::endl;
        robot.update_w(w);
    }

    std::cout << "Position reached, terminating program." << std::endl;

    robot.stop();
    return 0;
}
