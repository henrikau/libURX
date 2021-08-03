/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#include <urx/robot.hpp>

#include <future>   // For launching wait_for_q_ref() asynchronously

#ifndef BOOST_LOG_DYN_LINK
#define BOOST_LOG_DYN_LINK
#endif

#include <boost/log/trivial.hpp>
namespace logging = boost::log;

bool urx::Robot::init_output()
{
    std::lock_guard<std::mutex> lg(bottleneck);
    if (!out)
        return false;

    // don't run init() multiple times
    if (out_initialized_) {
        BOOST_LOG_TRIVIAL(warning) << __func__ << "() already initalized" << std::endl;
        return true;
    }

    if (!rtdeh_->is_connected()) {
        BOOST_LOG_TRIVIAL(warning) << __func__ << "() rtdeh not connected" << std::endl;
        return false;
    }

    if (out->num_fields() != 0) {
        BOOST_LOG_TRIVIAL(warning) << "() Cannot re-init a populated out-recipe " << out->get_fields();
        return false;
    }

    // register output fields
    // Note: We *could* register the fields in ur_state.directly, but then
    // we'd set ourselves up from some really awesome race conditions,
    // so do a 2-step approach.

    if (!out->add_field("output_int_register_0", &out_seqnr))
        return false;
    if (!out->add_field("timestamp", &timestamp))
        return false;
    if (!out->add_field("target_q", target_q))
        return false;
    if (!out->add_field("target_qd", target_qd))
        return false;
    if (!out->add_field("target_qdd", target_qdd))
        return false;
    if (!out->add_field("target_moment", target_moment))
        return false;

    if (!rtdeh_->register_recipe(out)) {
        out->clear_fields();
        return false;
    }

    out_initialized_ = true;
    return true;
}

bool urx::Robot::init_input()
{
    std::lock_guard<std::mutex> lg(bottleneck);
    if (!in)
        return false;

    in->add_field("input_int_register_0", &in_seqnr);
    in->add_field("input_int_register_1", &cmd);

    // FIXME: This breaks the DoF independence
    in->add_field("input_double_register_0", &set_qd[0]);
    in->add_field("input_double_register_1", &set_qd[1]);
    in->add_field("input_double_register_2", &set_qd[2]);
    in->add_field("input_double_register_3", &set_qd[3]);
    in->add_field("input_double_register_4", &set_qd[4]);
    in->add_field("input_double_register_5", &set_qd[5]);

    if (!rtdeh_->register_recipe(in)) {
        BOOST_LOG_TRIVIAL(error) << __func__ << "() Failed registering input recipe" << std::endl;
        return false;
    }

    in_initialized_ = true;
    return true;
}

bool urx::Robot::init_output_TCP_pose()
{
    std::lock_guard<std::mutex> lg(bottleneck);
    if (!out)
        return false;

    // don't run init() multiple times
    if (out_initialized_) {
        BOOST_LOG_TRIVIAL(warning) << __func__ << "() already initalized" << std::endl;
        return true;
    }

    if (!rtdeh_->is_connected()) {
        BOOST_LOG_TRIVIAL(warning) << __func__ << "() rtdeh not connected" << std::endl;
        return false;
    }

    if (out->num_fields() != 0) {
        BOOST_LOG_TRIVIAL(warning) << "() Cannot re-init a populated out-recipe " << out->get_fields();
        return false;
    }

    // register output fields
    // Note: We *could* register the fields in ur_state.directly, but then
    // we'd set ourselves up from some really awesome race conditions,
    // so do a 2-step approach.

    if (!out->add_field("output_int_register_0", &out_seqnr))
        return false;
    if (!out->add_field("timestamp", &timestamp))
        return false;
    if (!out->add_field("target_q", target_q))
        return false;
    if (!out->add_field("target_qd", target_qd))
        return false;
    if (!out->add_field("target_qdd", target_qdd))
        return false;
    if (!out->add_field("target_moment", target_moment))
        return false;
    if (!out->add_field("target_TCP_pose", target_TCP_pose))
        return false;

    // Registers for reading results of inverse kinematic calculations
    if (!out->add_field("output_int_register_1", &q_ref_out_seqnr_buf))
        return false;
    if (!out->add_field("output_double_register_0", &q_ref_buf[0]))
        return false;
    if (!out->add_field("output_double_register_1", &q_ref_buf[1]))
        return false;
    if (!out->add_field("output_double_register_2", &q_ref_buf[2]))
        return false;
    if (!out->add_field("output_double_register_3", &q_ref_buf[3]))
        return false;
    if (!out->add_field("output_double_register_4", &q_ref_buf[4]))
        return false;
    if (!out->add_field("output_double_register_5", &q_ref_buf[5]))
        return false;

    if (!rtdeh_->register_recipe(out)) {
        out->clear_fields();
        return false;
    }

    out_initialized_ = true;
    return true;
}

bool urx::Robot::init_input_TCP_pose()
{
    std::lock_guard<std::mutex> lg(bottleneck);
    if (!in)
        return false;

    in->add_field("input_int_register_0", &in_seqnr);
    in->add_field("input_int_register_1", &cmd);

    // FIXME: This breaks the DoF independence (copied from init_input())
    // Registers for sending control input
    in->add_field("input_double_register_0", &set_qd[0]);
    in->add_field("input_double_register_1", &set_qd[1]);
    in->add_field("input_double_register_2", &set_qd[2]);
    in->add_field("input_double_register_3", &set_qd[3]);
    in->add_field("input_double_register_4", &set_qd[4]);
    in->add_field("input_double_register_5", &set_qd[5]);

    // Registers 6 and 7 are ment for sending acceleration and time, not implemented yet

    // Registers for sending input to inverse kinematic calculations
    in->add_field("input_double_register_8" , &TCP_pose_ref[0]);
    in->add_field("input_double_register_9" , &TCP_pose_ref[1]);
    in->add_field("input_double_register_10", &TCP_pose_ref[2]);
    in->add_field("input_double_register_11", &TCP_pose_ref[3]);
    in->add_field("input_double_register_12", &TCP_pose_ref[4]);
    in->add_field("input_double_register_13", &TCP_pose_ref[5]);

    if (!rtdeh_->register_recipe(in)) {
        BOOST_LOG_TRIVIAL(error) << __func__ << "() Failed registering input recipe" << std::endl;
        return false;
    }

    in_initialized_ = true;
    return true;
}

bool urx::Robot::upload_script(const std::string& script)
{
    return urxh_->upload_script(script);
}

bool urx::Robot::recv()
{
    if (!rtdeh_->recv()) {
        BOOST_LOG_TRIVIAL(error) << __func__ << "() FAILED receiving incoming frame from RTDE Handler" << std::endl;
        return false;
    }

    {
        std::lock_guard<std::mutex> lg(bottleneck);
        if (!out_initialized_) {
            BOOST_LOG_TRIVIAL(warning) << __func__ << "() FAILED, not initialized" << std::endl;
            return false;
        }


        ur_state.local_ts_us = std::chrono::duration_cast<std::chrono::microseconds >(std::chrono::system_clock::now().time_since_epoch());
        ur_state.ur_ts = timestamp;
        ur_state.seqnr = out_seqnr;
        q_ref_out_seqnr    = q_ref_out_seqnr_buf;
        for (std::size_t i = 0; i < urx::DOF; i++) {
            ur_state.jq_ref[i]   = q_ref_buf[i];
            ur_state.jq[i]       = target_q[i];
            ur_state.jqd[i]      = target_qd[i];
            ur_state.jqdd[i]     = target_qdd[i];
            ur_state.jt[i]       = target_moment[i];
            ur_state.tcp_pose[i] = target_TCP_pose[i];
        }
        updated_state_ = true;
        
        ts_log.push_back( std::tuple<std::chrono::microseconds, double>(ur_state.local_ts_us, ur_state.ur_ts) );
    }

    cv.notify_all();
    return true;
}

urx::Robot_State urx::Robot::loc_state()
{
    if (updated_state_)
        updated_state_ = false;
    return ur_state;
}

urx::Robot_State urx::Robot::state(bool synchronous)
{
    Robot_State out;
    std::vector<std::tuple<std::chrono::microseconds, double>> ts_log_loc;
    if (!synchronous) {
        BOOST_LOG_TRIVIAL(error) << __func__ << "() DEPRECATED! Should not call with synchronous cleared!" << std::endl;
        bottleneck.lock();
        out = loc_state();
        bottleneck.unlock();
    } else {
        std::unique_lock<std::mutex> lk(bottleneck);
        auto timeout = std::chrono::milliseconds(100);
        if (!cv.wait_for(lk, timeout, [&] { return updated_state_; })) {
            BOOST_LOG_TRIVIAL(error) << __func__ << "() wait_for FAILED, did not receive a state-update" << std::endl;
            return out;
        }
        out = loc_state();

        if (ts_log.size() > 500) {
            ts_log_loc = std::move(ts_log);
            ts_log.clear();
        }

        if (ts_log_loc.size() > 3) {
            if (ts_log_fd) {
                for (auto& t : ts_log_loc)
                    fprintf(ts_log_fd, "%ld,%f\n", std::get<0>(t).count(), std::get<1>(t));
                fflush(ts_log_fd);
            }

            if (ts_log_debug) {
                std::chrono::microseconds first = std::get<0>(ts_log_loc[0]);
                std::chrono::microseconds last = std::get<0>(ts_log_loc[ts_log_loc.size() - 1]);
                long int diff_us = (last-first).count();

                long int prev = first.count();
                long int diff_max = 0;
                long int diff_min = prev;

                for (auto& t : ts_log_loc) {
                    auto curr = std::get<0>(t).count();
                    if (curr == prev)
                        continue;
                    auto diff = curr - prev;
                    if (diff > diff_max)
                        diff_max = diff;
                    if (diff < diff_min)
                        diff_min = diff;
                    prev = curr;
                }

                printf("samples:: %lu, span: %ld (%f sec)", ts_log_loc.size(), diff_us, 1.0 * diff_us * 1e-6);
                printf(" avg: %f", (double)diff_us / ts_log_loc.size());
                printf(" max: %lu", diff_max);
                printf(" min: %lu", diff_min);
                printf("\n");
            }
            ts_log_loc.clear();
        }
    }
    return out;
}

bool urx::Robot::updated_state()
{
    std::lock_guard<std::mutex> lg(bottleneck);
    return updated_state_;
}

bool urx::Robot::calculate_q_ref(std::vector<double>& new_pose)
{
    std::unique_lock<std::mutex> lk(bottleneck);

    if (new_pose.size() != DOF){
        std::cout << "Wrong size on new TCP pose\n";
        return false;
    }

    if (!in_initialized_) {
        BOOST_LOG_TRIVIAL(error) << __func__ << "() init_input() not completed successfully yet" << std::endl;
        return false;
    }

    // ranges are nice, but we have registred the address of the
    // elements in set_qd with the recipe, so we cannot simply swap, we
    // need a per-element copy
    for (std::size_t i = 0; i < new_pose.size(); ++i)
        TCP_pose_ref[i] = new_pose[i];

    in_seqnr = ++last_seqnr;
    cmd = NEW_TCP_POSE_REF_COMMAND;
    if (!rtdeh_->send(in->recipe_id())) {
        std::cout << "Sending to handler using " << in->recipe_id() << " failed" << std::endl;
        return false;
    }

    cmd = NO_COMMAND;

    uint32_t this_seqnr = in_seqnr;
    lk.unlock();    // No longer needed to hold lock and must not hold it when waiting in the else block

    // The first time q_ref is calculated the waiting for the results should be blocking
    if(q_ref_initialized_){
        // Perhaps simpler to run wait_for_q_ref body directly as a lambda expression in this call and remove the function
        auto ignore = std::async(std::launch::async,  &urx::Robot::wait_for_q_ref, this, this_seqnr);
    } else {

        wait_for_q_ref(this_seqnr);
        q_ref_initialized_ = true;
    }

    return true;
}

// It is possible to both throw an exception here or use a return val.
// Both can be retrieved through the return val of std::async
void urx::Robot::wait_for_q_ref(uint32_t q_ref_in_seqnr)
{
    std::unique_lock<std::mutex> lk(bottleneck);
    auto timeout = std::chrono::milliseconds(20);
    int attempts = 500; // New messages should arrive with 500Hz
    for(int i = 0; i < attempts; i++){
        if (cv.wait_for(lk, timeout, [&] { return q_ref_out_seqnr == q_ref_in_seqnr; })) {
            return;
        }
    }

    BOOST_LOG_TRIVIAL(error) << __func__ << "() never received q_ref from remote with seqnr " << q_ref_in_seqnr << "(probably too few attempts used)\n";
}

bool urx::Robot::update_w(std::vector<double>& new_w)
{   
    std::lock_guard<std::mutex> lg(bottleneck);

    if (new_w.size() != DOF){
        std::cout << "Wrong size on new w\n";
        return false;
    }

    if (!in_initialized_) {
        BOOST_LOG_TRIVIAL(error) << __func__ << "() init_input() not completed successfully yet" << std::endl;
        return false;
    }

    // ranges are nice, but we have registred the address of the
    // elements in set_qd with the recipe, so we cannot simply swap, we
    // need a per-element copy
    for (std::size_t i = 0; i < new_w.size(); ++i)
        set_qd[i] = new_w[i];

    in_seqnr = ++last_seqnr;
    cmd = NEW_CONTROL_INPUT_COMMAND;
    if (!rtdeh_->send(in->recipe_id())) {
        std::cout << "Sending to handler using " << in->recipe_id() << " failed" << std::endl;
        return false;
    }

    cmd = NO_COMMAND;

    return true;
}

bool urx::Robot::stop()
{
    if (!running_)
        return false;

    running_ = false;
    receiver.join();
    rtdeh_->stop();
    return true;
}

template<typename Func>
bool urx::Robot::start_(Func f)
{
    if (!out_initialized_)
        return false;

    if (!f())
        return false;

    // start receiving data
    rtdeh_->start();
    receiver = std::thread(&urx::Robot::run, this);

    // Wait here until receiver has started and is ready
    {
        std::unique_lock<std::mutex> lk(bottleneck);
        auto timeout = std::chrono::milliseconds(100);
        if (!start_cv.wait_for(lk, timeout, [&] { return updated_state_; })) {
            BOOST_LOG_TRIVIAL(error) << __func__ << "() Failed waiting for udpated state (timeout)" << std::endl;
            return false;
        }
    }
    return true;
}

bool urx::Robot::start()
{
    return start_([]{ return true; });
}


bool urx::Robot::start_rr(int pri)
{
    return start_([=]{
            struct sched_param param;
            param.sched_priority = pri;
            if (sched_setscheduler(0, SCHED_RR, &param)) {
                perror("Failed setting scheduler");
                return false;
            }
            return true;
        });
    return false;
}

bool urx::Robot::running()
{
    return running_;
}

void urx::Robot::run()
{
    if (!running_)
        running_ = true;

    start_cv.notify_all();

    while (running_) {
        if (dut_)
            usleep(2000);
        recv();
    }
}
