/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#define BOOST_TEST_MODULE robot
#define BOOST_TEST_DYN_LINK
#include <iostream>

#include <boost/test/unit_test.hpp>
#include <urx/urx_handler.hpp>
#include <urx/rtde_handler.hpp>

#include <urx/rtde_recipe.hpp>
#include <urx/helper.hpp>
#include "mocks/mock_con.hpp"
#include "test_helpers.hpp"
#include <urx/robot.hpp>
#include <urx/helper.hpp>

struct F
{
    F()
    {
        umock = new urx::Con_mock();
        rmock = new urx::Con_mock();
        urxh = new urx::URX_Handler(umock);
        rtdeh = new urx::RTDE_Handler(rmock);
        robot = new urx::Robot(urxh, rtdeh);
        robot->set_dut();
        BOOST_TEST_MESSAGE("setup fixture");
    }
    ~F()
    {
        delete robot;
        robot = nullptr;
        BOOST_TEST_MESSAGE( "teardown fixture" );
    }

    void setup_output_recipe()
    {
        memset(buffer, 0, 2048);
        resp = (struct rtde_control_package_resp *)buffer;
        _set_recipe_resp(resp, "INT32,DOUBLE,VECTOR6D,VECTOR6D,VECTOR6D,VECTOR6D", recipe_id);
        rmock->set_recvBuf((unsigned char *)resp, (int)ntohs(resp->hdr.size));
    }

    void setup_input_recipe()
    {
        memset(buffer, 0, 2048);
        resp = (struct rtde_control_package_resp *)buffer;
        _set_recipe_resp(resp, "INT32,INT32,DOUBLE,DOUBLE,DOUBLE,DOUBLE,DOUBLE,DOUBLE", recipe_id);
        rmock->set_recvBuf((unsigned char *)resp, (int)ntohs(resp->hdr.size));
        rmock->set_sendCode(recipe_id);
    }

    void setup_rtde_vals()
    {
        memset(buffer, 0, 2048);
        dp = (struct rtde_data_package *)buffer;
        rtde_data_package_init(dp, recipe_id, (4 + 8 + 6*8*4));
        dbuf = rtde_data_package_get_payload(dp);
        seqnr = (int32_t *)&dbuf[0];
        ts  = (double *)&dbuf[4];
        target_q = (double *)&dbuf[4+8];
        target_qd = (double *)&dbuf[4+8 + 8*6];
        target_qdd = (double *)&dbuf[4+8 + 8*6*2];
        target_moment = (double *)&dbuf[4+8 + 8*6*3];
    }

    void push_rtde_vals()
    {
        rmock->set_recvBuf(buffer, (int)ntohs(dp->hdr.size));

        //make sure  we have a context-switch so recv-thread will have a
        //chance to run (we are probably calling into state() with sync=false))
        usleep(2000);
    }

    void ready_data_buffer(int32_t s=1337)
    {
        *seqnr = htobe32(s);
        *ts = urx::double_be(42.0);
        for (int i = 0; i < urx::END; i++) {
            target_q[i] = urx::double_be((double)i);
            target_qd[i] = urx::double_be((double)i*2+1);
            target_qdd[i] = urx::double_be((double)i*3+17);
            target_moment[i] = urx::double_be((double)i);
        }
        push_rtde_vals();
    }

    void setup_robot_output_defaults()
    {
        setup_output_recipe();
        rmock->set_sendCode(recipe_id);
        rmock->set_sendBuffer(send_buffer, send_sz);

        if (!robot->init_output())
            printf("%s: robot->init_output() FAILED\n", __func__);

        setup_rtde_vals();
        ready_data_buffer();
    }

    void setup_robot_input_defaults()
    {
        setup_input_recipe();
        rmock->set_sendCode(recipe_id);
        rmock->set_sendBuffer(send_buffer, send_sz);
        if (!robot->init_input())
            printf("%s: robot->init_input() FAILED\n", __func__);
    }

    void setup_urxh()
    {
        umock->set_sendCode(42);
        umock->set_sendBuffer(urx_send_buffer, urx_send_sz);
    }

    // For RTDE setup
    const int recipe_id = 42;
    struct rtde_control_package_resp * resp;
    struct rtde_data_package * dp;
    unsigned char buffer[2048];
    unsigned char *dbuf;
    int32_t * seqnr;
    double *ts;
    double *target_q;
    double *target_qd;
    double *target_qdd;
    double *target_moment;

    unsigned char send_buffer[2048];
    const int send_sz = 2048;
    unsigned char urx_send_buffer[2048];
    const int urx_send_sz = 2048;

    //
    urx::Robot *robot;
    urx::Con_mock* umock;
    urx::Con_mock* rmock;
    urx::URX_Handler *urxh;
    urx::RTDE_Handler *rtdeh;

};

BOOST_FIXTURE_TEST_SUITE(s, F)


BOOST_AUTO_TEST_CASE(test_send_script_noexists)
{
    BOOST_CHECK(umock->is_connected());
    BOOST_CHECK(rmock->is_connected());
}

BOOST_AUTO_TEST_CASE(test_robot_rtde_setup)
{
    umock->disconnect();
    rmock->disconnect();

    BOOST_CHECK(!robot->init_output());

    umock->do_connect();
    rmock->do_connect();

    // Need to set up send/receive for control package
    rmock->set_sendCode(1);
    struct rtde_control_package_resp *resp = (struct rtde_control_package_resp *)buffer;
    _set_recipe_resp(resp, "DOUBLE", recipe_id);
    rmock->set_recvBuf((unsigned char *)resp, (int)ntohs(resp->hdr.size));
    rmock->set_sendCode(42);      // random size, > 0

    BOOST_CHECK(!robot->init_output());

    // Use RecipID from remote
    setup_output_recipe();

    BOOST_CHECK(robot->init_output());
}

BOOST_AUTO_TEST_CASE(test_state_copies)
{
    urx::Robot_State s1 = robot->state(false);
    urx::Robot_State s2 = robot->state(false);

    BOOST_CHECK(s1.local_ts_us == s2.local_ts_us);
    s1.ur_ts = 1.0;
    s2.ur_ts = 2.0;
    BOOST_CHECK_CLOSE((s2.ur_ts - s1.ur_ts), 1.0, 1e-9);
    s1.jqd[3] = 132;
    s2.jqd[3] = 133;
    BOOST_CHECK_CLOSE((s2.jqd[3] - s1.jqd[3]), 1.0, 1e-9);
}


// if version is not set, UR Controller will just return NOT_FOUND for
// the first variable in output-recipe.
BOOST_AUTO_TEST_CASE(test_robot_sets_version)
{
    memset(send_buffer, 0, send_sz);
    urx::Con_mock *u = new urx::Con_mock();
    urx::Con_mock *r = new urx::Con_mock();
    r->set_sendCode(recipe_id);
    r->set_sendBuffer(send_buffer, send_sz);

    urx::URX_Handler  * uh = new urx::URX_Handler(u);
    urx::RTDE_Handler * rh = new urx::RTDE_Handler(r);
    urx::Robot *rob = new urx::Robot(uh, rh);

    // we should now have a set_version message for v2 in send-buffer
    struct rtde_prot *header = (struct rtde_prot *)send_buffer;
    BOOST_CHECK(header->hdr.type == RTDE_REQUEST_PROTOCOL_VERSION);
    BOOST_CHECK(header->payload.version == ntohs(2));
    delete rob;
}

BOOST_AUTO_TEST_CASE(test_read_output_rtde)
{
    setup_robot_output_defaults();

    // receive data, only call recv() if robot is not running
    BOOST_ASSERT(!robot->running());
    BOOST_CHECK(robot->recv());

    urx::Robot_State state = robot->state(false);
    BOOST_CHECK(state.dof == urx::DOF);
    BOOST_CHECK_MESSAGE(state.seqnr == 1337, "Seqnr not as expected");
    BOOST_CHECK_CLOSE(state.ur_ts, 42.0, 1e-9);
    for (int i = 0; i < 6; i++) {
        BOOST_CHECK_CLOSE(state.jq[i],   1.0*i, 1e-9);
        BOOST_CHECK_CLOSE(state.jqd[i],  2.0*i + 1, 1e-9);
        BOOST_CHECK_CLOSE(state.jqdd[i], 3.0*i + 17, 1e-9);
        BOOST_CHECK_CLOSE(state.jt[i],   1.0*i, 1e-9);
    }
}

BOOST_AUTO_TEST_CASE(test_recv_updates_new_state)
{
    BOOST_CHECK(!robot->updated_state());
    BOOST_CHECK(!robot->recv());

    setup_robot_output_defaults();

    // No state has been received
    BOOST_CHECK(!robot->updated_state());
    BOOST_CHECK(robot->recv());
    BOOST_CHECK(robot->updated_state());
    urx::Robot_State s = robot->state(false);
    BOOST_CHECK(s.seqnr == 1337);

    // we've gotten the last state
    BOOST_CHECK(!robot->updated_state());
}

BOOST_AUTO_TEST_CASE(test_robot_start)
{
    setup_robot_output_defaults();

    // cannot stop a stopped thread
    BOOST_CHECK(!robot->stop());
    BOOST_ASSERT(robot->start());

    // make sure we actually get a start-message:
    struct rtde_header *start = (struct rtde_header *)send_buffer;
    BOOST_CHECK(start->type == RTDE_CONTROL_PACKAGE_START);

    std::cout << "Thread spawned, waiting before killing it" << std::endl;
    usleep(1000);
    BOOST_ASSERT(robot->running());

    ready_data_buffer(1337);

    urx::Robot_State s1 = robot->state(false);
    std::cout << "state: " << s1.seqnr << std::endl;
    BOOST_CHECK(s1.seqnr == 1337);

    // manipulate the memory where robot->rtdeh->con_mock will retreive
    // the data
    *seqnr = htobe32(1338);
    for (int i = 0; i < 6; i++)
        target_q[i] = urx::double_be((double)i+3.0);
    push_rtde_vals();

    // verify updated content
    urx::Robot_State s2 = robot->state(false);
    BOOST_CHECK(s2.seqnr == 1338);
    for (int i = 0; i < urx::END; i++)
        BOOST_CHECK_CLOSE(s2.jq[i], 1.0*i + 3.0, 1e-9);

    memset(send_buffer, 0, send_sz);
    BOOST_CHECK(robot->stop());
    BOOST_CHECK(start->type == RTDE_CONTROL_PACKAGE_PAUSE);
}

BOOST_AUTO_TEST_CASE(test_robot_init_input)
{
    setup_input_recipe();
    rmock->set_sendBuffer(send_buffer, send_sz);

    BOOST_CHECK(robot->init_input());

}
BOOST_AUTO_TEST_CASE(test_robot_init_vals)
{
    setup_robot_output_defaults();
    setup_robot_input_defaults();

    // setup input variables
    std::vector<double> w;

    // too small vector
    BOOST_CHECK(!robot->update_w(w));

    for (std::size_t i = 0; i < urx::DOF; ++i)
        w.push_back(0);

    BOOST_CHECK(robot->update_w(w));

    // Verify sent data via robot->rtde_handler->con
    struct rtde_data_package *data = (struct rtde_data_package *)send_buffer;
    BOOST_CHECK(data->hdr.type == RTDE_DATA_PACKAGE);

    unsigned char *arr = rtde_data_package_get_payload(data);
    int32_t *seqnr = (int32_t *)(arr);
    int32_t *cmd   = (int32_t *)(arr + sizeof(int32_t));
    double  *w_in  = ( double *)(arr + 2 * sizeof(int32_t));

    BOOST_CHECK(ntohl(*seqnr) == 1);
    BOOST_CHECK(ntohl(*cmd) == 2);
    for (std::size_t i = 0; i < urx::DOF; ++i) {
        BOOST_CHECK_CLOSE(urx::double_h(w_in[i]), 0, 1e-9);
    }

    w.clear();
    for (std::size_t i = 0; i < urx::DOF; ++i)
        w.push_back(i);
    memset(send_buffer, 0, send_sz);
    BOOST_CHECK(robot->update_w(w));
    for (std::size_t i = 0; i < urx::DOF; ++i) {
        w_in = (double *)(arr + 2 * sizeof(int32_t) + i*sizeof(double));
        BOOST_CHECK_CLOSE(urx::double_h(*w_in), 1.0*i, 1e-9);
    }

    w[3] = 99.9;
    memset(send_buffer, 0, send_sz);
    BOOST_CHECK(robot->update_w(w));
    for (std::size_t i = 0; i < urx::DOF; ++i) {
        w_in = (double *)(arr + 2 * sizeof(int32_t) + i*sizeof(double));
        if (i != 3)
            BOOST_CHECK_CLOSE(urx::double_h(*w_in), 1.0*i, 1e-9);
        else
            BOOST_CHECK_CLOSE(urx::double_h(*w_in), 99.9, 1e-9);
    }
}

BOOST_AUTO_TEST_CASE(test_robot_script_upload)
{
    setup_urxh();
    BOOST_CHECK(robot->upload_script("simple_movej.script"));
    BOOST_CHECK(strncmp((const char *)urx_send_buffer, "def rtde_prog()", strlen("def rtde_prog()")) == 0);
}

BOOST_AUTO_TEST_CASE(test_robot_sync_state)
{
    setup_robot_output_defaults();
    setup_robot_input_defaults();

    constexpr int loop = 10;
    std::chrono::microseconds prev;

    prev = std::chrono::duration_cast<std::chrono::microseconds >(std::chrono::system_clock::now().time_since_epoch());

    robot->start();
    usleep(1000);

    // thread that sends  10 frames, every 5000us
    std::thread t = std::thread([&] {
            std::cout << "Robot started, setting up vector" << std::endl;
            std::vector<double> w;
            for (std::size_t i = 0; i < urx::DOF; ++i)
                w.push_back(0);

            std::cout << "Robot running, pushing vector for " << loop << " iterations." << std::endl;
            for (int i = 0; i < loop; ++i) {
                w[urx::ELBOW] = i*1.0;
                std::cout << "w[urx::ELBOW]=" <<w[urx::ELBOW]<<std::endl;
                BOOST_CHECK(robot->update_w(w));
                usleep(5000);
            }
            std::cout << "stopping robot" << std::endl;
            robot->stop();
            std::cout << "robot stopped" << std::endl;
        });
#if 0


    // start polling data, call state() synchronous, so we should block
    std::chrono::microseconds current;
    struct rtde_data_package *data = (struct rtde_data_package *)send_buffer;
    int32_t seqnr = 0;
    for (int i = 0; i < loop; ++i) {
        urx::Robot_State s = robot->state();

        BOOST_CHECK(data->hdr.type == RTDE_DATA_PACKAGE);
        current  = std::chrono::duration_cast<std::chrono::microseconds >(std::chrono::system_clock::now().time_since_epoch());
        std::cout << "Diff: " << (current - prev).count() << " us" << std::endl;
        BOOST_CHECK((current - prev).count() < 10000);

        std::cout << "s.jq[urx::ELBOW]=" << s.jq[urx::ELBOW] << std::endl;
        BOOST_CHECK_CLOSE(s.jq[urx::ELBOW], 1.0*i, 1e-9);
    }
#endif

    t.join();

}
BOOST_AUTO_TEST_SUITE_END()
