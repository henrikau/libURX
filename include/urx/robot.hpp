/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#ifndef URX_ROBOT_HPP
#define URX_ROBOT_HPP
#include <string>
#include <mutex>
#include <condition_variable>
#include <urx/urx_handler.hpp>
#include <urx/rtde_handler.hpp>
#include <chrono>
#include <thread>
#include <atomic>

namespace urx {
    constexpr std::size_t DOF = 6;
    enum {
        NO_COMMAND = 0,
        STOP_COMMAND,
        NEW_CONTROL_INPUT_COMMAND,
        NEW_TCP_POSE_REF_COMMAND
    };

    /**
     * \brief bucket of internal state data for the robot
     */
    struct Robot_State {
        explicit Robot_State() :
            dof(DOF),
            ur_ts(0),
            seqnr(0),
            jq_ref(DOF),
            jq(DOF),
            jqd(DOF),
            jqdd(DOF),
            jt(DOF),
            tcp_pose(DOF)
        {
        };

        // Copy constructor
        // this <-- a
        Robot_State(const Robot_State& a) :
            dof(DOF),
            ur_ts(a.ur_ts),
            seqnr(a.seqnr)
        {
            jq_ref = a.jq_ref;
            jq     = a.jq;
            jqd    = a.jqd;
            jqdd   = a.jqdd;
            jt     = a.jt;

            tcp_pose = a.tcp_pose;
        };

        // Copy constructor
        // this <-- a
        Robot_State& operator=(const Robot_State& a)
        {
            local_ts_us = a.local_ts_us;
            ur_ts = a.ur_ts;
            seqnr = a.seqnr;
            for (std::size_t i = 0; i < DOF; i++) {
                jq_ref[i] = a.jq_ref[i];
                jq[i]     = a.jq[i];
                jqd[i]    = a.jqd[i];
                jqdd[i]   = a.jqdd[i];
                jt[i]     = a.jt[i];

                tcp_pose[i] = a.tcp_pose[i];
            }
            return *this;
        }

        // Fast copy of a's state into this state
        // this <-- a
        void copy_from(Robot_State& a)
        {
            local_ts_us = a.local_ts_us;
            ur_ts = a.ur_ts;
            seqnr = a.seqnr;
            for (std::size_t i = 0; i < DOF; i++) {
                jq_ref[i] = a.jq_ref[i];
                jq[i]     = a.jq[i];
                jqd[i]    = a.jqd[i];
                jqdd[i]   = a.jqdd[i];
                jt[i]     = a.jt[i];

                tcp_pose[i] = a.tcp_pose[i];
            }
        }

        // Degrees of freedom for this arm
        const std::size_t dof;

        // local time when the *latest* set of values were received
        std::chrono::microseconds local_ts_us;

        // Timestamp at UR-controller for when the frame was
        // sent. Relative to boot for controller.
        double ur_ts;
        int32_t seqnr;

        // angle for each joint
        std::vector<double> jq_ref;
        std::vector<double> jq;
        std::vector<double> jqd;
        std::vector<double> jqdd;
        std::vector<double> jt;         // joint torque

        // Tool Center Point pose
        std::vector<double> tcp_pose;
    };


    class Robot {
    public:
        Robot(URX_Handler *urxh, RTDE_Handler *rtdeh) :
            urxh_(urxh),
            rtdeh_(rtdeh),
            out_initialized_(false),
            in_initialized_(false),
            running_(false),
            dut_(false),
            updated_state_(false),
            q_ref_initialized_(false),
            last_seqnr(0),
            q_ref_out_seqnr(0),
            in_seqnr(1),
            cmd(NO_COMMAND),
            ur_state(Robot_State()),
            ts_log_debug(false)

        {
            out = new urx::RTDE_Recipe();
            in = new urx::RTDE_Recipe();
            in->dir_input();
            rtdeh_->set_version();
            ts_log_fd = fopen("ts_receive.csv", "w");
            if (ts_log_fd) {
                fprintf(ts_log_fd, "ts_loc,ts_ur\n");
                fflush(ts_log_fd);
            }
        };

        Robot(const std::string remote) :
            Robot(new URX_Handler(remote),
                  new RTDE_Handler(remote))
        {};

        ~Robot()
        {
            if (ts_log_fd) {
                fflush(ts_log_fd);
                fclose(ts_log_fd);
                ts_log_fd = NULL;
            }

            delete urxh_;
            delete rtdeh_;
            delete out;
            delete in;
            out_initialized_ = false;
            in_initialized_ = false;
        }


        /**
         * return internal state of robot
         *
         * It is expected that recv() has been called and some data has
         * found its way into our cold embrace.
         *
         * If synchronous is set, it will block until a *new* set of
         * data has been received from the remote controller. Note that
         * if new state is present since last call (i.e. updated_state()
         * is true), it will return immediately.
         *
         * Note: synchronous is expsed as an option to facilitate
         * testing and is planned to be phased out.
         *
         * Can also block for a short period of time waiting for the
         * bottleneck to be released by the receiver thread.
         *
         * \params synchronous indicate if call should be blocking
         * \returns Robot_State with latest state from robot.
         */
        Robot_State state(bool synchronous=true);

        /**
         * \brief see if robot has a state update
         *
         * Indicates whether a new (and valid) RTDE data pacakge has been received,
         * parsed and unpacked since the last call to state().
         *
         * \return true if updated state is available
         */
        bool updated_state();
        
        /**
         * \brief Asynchronously sets new reference for Tool Center Point pose. 
         * 
         * Triggers inverse kinematic calculations in URScript for finding corresponding joint
         * angle references. q_ref is updated asynchronously through recv() and can be accessed 
         * through state(). If q_ref is initialized by a previous call to calculate_q_ref() then 
         * wait_for_q_ref() is run asynchronously, if not it blocks.
         *
         * \return true if valid and successfully sent to remote 
         * (q_ref is not necessarily successfully received yet).
         */
        bool calculate_q_ref(std::vector<double>& new_pose);

        /**
         * \brief set new target joint speed (angular)
         *
         * \return true if valid and successfully sent to remote.
         */
        bool update_w(std::vector<double>& new_w);

        /**
         * \brief initalize robot, create recipes and prepare for start
         * 
         * \pre The variables connected to the input recipe must be properly initialized before init_input() is called,
         * such that the UR script does not perform unexpected actions due to poorly initialized input registers. Also, for the
         * same reason init_input() should be called before upload_script().
         *
         * \return true if all worked as expected
         */
        bool init_output();
        bool init_input();
        bool init() { return init_output() && init_input(); }

        /**
         * \brief initalize robot, create recipes for controlling Tool Center Point pose and prepare for start
         *
         * \pre The variables connected to the input recipe must be properly initialized before init_input_TCP_pose() is called,
         * such that the UR script does not perform unexpected actions due to poorly initialized input registers. Also, for the
         * same reason init_input_TCP_pose() should be called before upload_script().
         * 
         * As TCP pose is a part of the robot state then init_output_TCP_pose() should
         * probably not be a separate function. Instead, init_output() should handle updating the 
         * entire robot state, incl TCP pose: But this gave a test error during build.
         * 
         * \return true if all worked as expected
         */
        bool init_output_TCP_pose();
        bool init_input_TCP_pose();

        /**
         * \brief Upload new script to URController
         * 
         * \pre init_input() must be called before this function to initialize the input registers on the URController
         * 
         * \return true on success.
         */
        bool upload_script(const std::string& script);

        /**
         * \brief Wait for a new RTDE dataframe
         *
         * This is a *blocking* call, it will wait for a new dataframe
         * from UR and update the internal state
         */
        bool recv();

        /**
         * \brief start the receiver thread
         */
        template<typename Func>
        bool start_(Func f);
        bool start();
        bool start_rr(int pri);

        /**
         * \brief stop the receiver thread
         */
        bool stop();

        /**
         * \brief query if recv-thread is running
         *
         * \returns true if receiver is running and parsing RTDE output from the controller.
         */
        bool running();

        /**
         * \brief: tag Robot as being under test.
         *
         * This will add delays where we would normally block from OS
         * (i.e. on recv() etc)
        */
        void set_dut() { dut_ = true; };

        // Returns the value of q_ref_initialized_
        bool q_ref_initialized() const {return q_ref_initialized_; };

    private:
        /**
         * \brief mainloop for reciever thread
         *
         * This is the worker spawned by start() and will *not* by
         * default run in it's own thread, nor will it attempt to move
         * to another scheduling policy or priority. It expects this to
         * be done *prior* to run().
         */
        void run();

        /**
         * \brief internal state function.
         *
         * Expects to be called with locks held, will happily update
         * internal state.
         *
         * \returns copy of Robot_State
         */
        Robot_State loc_state();

        /**
         * \brief Monitors whether the results from inverse cinematic calculations in URScript are received in time.
         * 
         * This function is called asynchronously in calculate_q_ref(),
         * unless it is the first time it is called, then the waiting 
         * is synchronous. Writes to log if q_ref is not received in time.
         */
        void wait_for_q_ref(uint32_t q_ref_in_seqnr);        

        URX_Handler *urxh_;
        RTDE_Handler *rtdeh_;
        bool out_initialized_;
        bool in_initialized_;
        urx::RTDE_Recipe * out;
        urx::RTDE_Recipe * in;
        std::atomic<bool> running_;
        // Device under Test, if true, add delays where we would
        // normally block
        bool dut_;

        std::thread receiver;

        std::mutex bottleneck;
        std::condition_variable cv;
        std::condition_variable start_cv;

        bool updated_state_; // new state arrived since last state()
        std::atomic<bool>  q_ref_initialized_;

        uint32_t last_seqnr;

        // Robot output, registred with handler through input/output recipes
        uint32_t out_seqnr;
        double timestamp;
        double target_q[DOF];
        double target_qd[DOF];
        double target_qdd[DOF];
        double target_moment[DOF];
        double target_TCP_pose[DOF];
        double q_ref_buf[DOF];          // To avoid race conditions
        uint32_t q_ref_out_seqnr;
        uint32_t q_ref_out_seqnr_buf;   // To avoid race conditions

        // Robot input
        uint32_t in_seqnr;
        int32_t cmd;
        double set_qd[DOF];
        double TCP_pose_ref[DOF];     

        Robot_State ur_state;
        std::vector<std::tuple<std::chrono::microseconds, double>> ts_log;
        FILE *ts_log_fd;
        bool ts_log_debug;
    };
}
#endif  // URX_ROBOT_HPP
