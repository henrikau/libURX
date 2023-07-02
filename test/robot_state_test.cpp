/*
 * Copyright 2023 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */

#define BOOST_TEST_MODULE robot_state
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <urx/robot.hpp>

struct F
{
    F()
    {
    state.seqnr = 42;
    state.ur_ts = 13.37;
    for (size_t i = 0; i < urx::DOF; i++) {
        state.jq_ref[i] = i;
        state.jq[i] = i;
        state.jqd[i] = i;
        state.jqdd[i] = i;
        state.jt[i] = i;
        state.tcp_pose[i] = i;
    }
    state.local_ts_us = std::chrono::microseconds(11);
    }
    ~F()
    {

    }
    urx::Robot_State state;
};

BOOST_FIXTURE_TEST_SUITE(s, F)

BOOST_AUTO_TEST_CASE(test_robot_state_check_init)
{
    urx::Robot_State state;

    BOOST_CHECK(state.dof == urx::DOF);
    for (size_t i = 0; i < urx::DOF; i++) {
        BOOST_CHECK_CLOSE(state.jq_ref[i], 0, 0.0001);
        BOOST_CHECK_CLOSE(state.jq[i], 0, 0.0001);
        BOOST_CHECK_CLOSE(state.jqd[i], 0, 0.0001);
        BOOST_CHECK_CLOSE(state.jqdd[i], 0, 0.0001);
        BOOST_CHECK_CLOSE(state.jt[i], 0, 0.0001);
        BOOST_CHECK_CLOSE(state.tcp_pose[i], 0, 0.0001);
    }
    BOOST_CHECK(state.seqnr == 0);
    BOOST_CHECK_CLOSE(state.ur_ts, 0.0, 0.0001);
}
BOOST_AUTO_TEST_CASE(test_robot_state_update)
{
    urx::Robot_State state;

    state.seqnr = 42;
    state.ur_ts = 13.37;
    for (size_t i = 0; i < urx::DOF; i++) {
        state.jq_ref[i] = i;
        state.jq[i] = i;
        state.jqd[i] = i;
        state.jqdd[i] = i;
        state.jt[i] = i;
        state.tcp_pose[i] = i;
    }
    state.local_ts_us = std::chrono::microseconds(11);

    BOOST_CHECK(state.seqnr == 42);
    BOOST_CHECK_CLOSE(state.ur_ts, 13.37, 0.0001);
    for (size_t i = 0; i < urx::DOF; i++) {
        BOOST_CHECK_CLOSE(state.jq_ref[i], i, 0.0001);
        BOOST_CHECK_CLOSE(state.jq[i], i, 0.0001);
        BOOST_CHECK_CLOSE(state.jqd[i], i, 0.0001);
        BOOST_CHECK_CLOSE(state.jqdd[i], i, 0.0001);
        BOOST_CHECK_CLOSE(state.jt[i], i, 0.0001);
        BOOST_CHECK_CLOSE(state.tcp_pose[i], i, 0.0001);
    }
}

BOOST_AUTO_TEST_CASE(test_robot_state_copy)
{
    BOOST_CHECK(state.seqnr == 42);
    urx::Robot_State s2(state);
    BOOST_CHECK(s2.seqnr == 42);
    state.seqnr = 1;
    BOOST_CHECK(s2.seqnr == 42);
    BOOST_CHECK_CLOSE(s2.ur_ts, 13.37, 0.0001);
    for (size_t i = 0; i < urx::DOF; i++) {
        BOOST_CHECK_CLOSE(s2.jq_ref[i], i, 0.0001);
        BOOST_CHECK_CLOSE(s2.jq[i], i, 0.0001);
        BOOST_CHECK_CLOSE(s2.jqd[i], i, 0.0001);
        BOOST_CHECK_CLOSE(s2.jqdd[i], i, 0.0001);
        BOOST_CHECK_CLOSE(s2.jt[i], i, 0.0001);
        BOOST_CHECK_CLOSE(s2.tcp_pose[i], i, 0.0001);
    }
    urx::Robot_State s3 = state;
    BOOST_CHECK_CLOSE(s3.ur_ts, 13.37, 0.0001);
    state.ur_ts = 42.42;
    BOOST_CHECK_CLOSE(state.ur_ts, 42.42, 0.0001);
    BOOST_CHECK_CLOSE(s3.ur_ts, 13.37, 0.0001);
}

BOOST_AUTO_TEST_CASE(test_robot_state_copy_contents)
{
    urx::Robot_State *s = new urx::Robot_State();
    BOOST_CHECK(s->seqnr == 0);
    BOOST_CHECK_CLOSE(s->ur_ts, 0, 0.0001);

    void *p = (void *)s;
    s->seqnr = 1337;
    printf("state: %p\n", (void*)s);
    s->copy_from(state);
    BOOST_CHECK(s==p);
    BOOST_CHECK(s->seqnr == state.seqnr);
    state.seqnr = 123;
    BOOST_CHECK(state.seqnr == 123);
    BOOST_CHECK(s->seqnr != state.seqnr);
}

BOOST_AUTO_TEST_SUITE_END()
