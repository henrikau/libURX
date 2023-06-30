/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#define BOOST_TEST_MODULE rtde_handler_test
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <arpa/inet.h>

#include <urx/rtde_handler.hpp>
#include <urx/con.hpp>
#include <urx/rtde_recipe.hpp>
#include <urx/helper.hpp>

#include "mocks/mock_con.hpp"
#include "test_helpers.hpp"

struct F
{
    F()
    {
      mock = new urx::Con_mock();
      h = new urx::RTDE_Handler(mock);

      BOOST_TEST_MESSAGE("setup fixture");
      header = (struct rtde_prot *)&buf_[0];

      // make version-reply be the default
      set_version();
  }

  ~F()
  {
      delete h;
      BOOST_TEST_MESSAGE( "teardown fixture" );
  }

    void set_version() {
        header->hdr.size = 0x0400;
        header->hdr.type = RTDE_REQUEST_PROTOCOL_VERSION;
        header->payload.accepted = true;
        mock->set_recvBuf(buf_, 4);
        mock->set_sendCode(4);

    }

    void set_ur_version()
    {
        urver = (struct rtde_ur_ver *)buf_;
        urver->hdr.size = 0x1300;
        urver->hdr.type = RTDE_GET_URCONTROL_VERSION;
        urver->payload.major = htonl(5);
        urver->payload.minor = htonl(3);
        urver->payload.bugfix = htonl(0);
        urver->payload.build = htonl(1337);
        mock->set_recvBuf(buf_, sizeof(*urver));
        mock->set_sendCode(sizeof(*urver));
    }

    struct rtde_prot *header;
    struct rtde_ur_ver *urver;
    urx::RTDE_Handler* h;
    urx::Con_mock* mock;

  unsigned char buf_[128];
};

BOOST_FIXTURE_TEST_SUITE(s, F)


BOOST_AUTO_TEST_CASE(test_creating_own_handler)
{
    BOOST_CHECK(mock != nullptr);
    mock->disconnect();

    BOOST_CHECK(!mock->is_connected());
    BOOST_CHECK(h->connect_ur());
    BOOST_CHECK(mock->is_connected());
    BOOST_CHECK(h->is_connected() == mock->is_connected());

    // Make sure handler calls do_connect properly
    mock->disconnect();
    BOOST_CHECK(!h->is_connected());
    BOOST_CHECK(h->connect_ur());
    BOOST_CHECK(h->is_connected());
    BOOST_CHECK(mock->is_connected());

    // General failure when sending
    mock->set_sendCode(-1);
    BOOST_CHECK(!h->send_emerg_msg("Foobar"));

    mock->set_sendCode(1);
    BOOST_CHECK(h->send_emerg_msg("Foobar"));
}

BOOST_AUTO_TEST_CASE(test_handler_server)
{
    BOOST_CHECK(h->is_connected());
    BOOST_CHECK(h->set_version());
}

BOOST_AUTO_TEST_CASE(test_error_version)
{
    header->payload.accepted = false;
    mock->set_recvBuf(buf_, 4);
    BOOST_CHECK(!h->set_version());
}

BOOST_AUTO_TEST_CASE(test_urversion)
{
    set_ur_version();

    struct rtde_ur_ver_payload urver;
    BOOST_CHECK(h->get_ur_version(urver));
    BOOST_CHECK(urver.major == 5);
    BOOST_CHECK(urver.minor == 3);
    BOOST_CHECK(urver.bugfix == 0);
    BOOST_CHECK(urver.build == 1337);
}

BOOST_AUTO_TEST_CASE(test_urversion_invalid_major)
{
    set_ur_version();
    urver->payload.major = htonl(19);
    mock->set_recvBuf(buf_, sizeof(*urver));

    struct rtde_ur_ver_payload urver;
    BOOST_CHECK(!h->get_ur_version(urver));

}
BOOST_AUTO_TEST_CASE(test_urversion_emerg_msg)
{
    mock->set_sendCode(99);
    BOOST_CHECK(h->send_emerg_msg("This is a test"));
}

BOOST_AUTO_TEST_CASE(test_handler_recipe)
{
    urx::RTDE_Recipe *r = new urx::RTDE_Recipe();

    BOOST_CHECK(!h->register_recipe(NULL));

    double ts;
    BOOST_CHECK(r->add_field("timestamp", &ts));

    mock->set_sendCode(-1);
    BOOST_CHECK(!h->register_recipe(r));

    struct rtde_control_package_resp *resp = create_cp_resp();
    _set_recipe_resp(resp, "DOUBLE", 42);
    mock->set_recvBuf((unsigned char *)resp, (int)ntohs(resp->hdr.size));

    mock->set_sendCode(42);      // random size, > 0

    BOOST_CHECK(h->register_recipe(r));
    free(resp);
}

// test path from creating recipe to receivng data from UR
BOOST_AUTO_TEST_CASE(test_handler_full_data_path)
{
    struct rtde_control_package_resp *resp = create_cp_resp();
    _set_recipe_resp(resp, "DOUBLE", 42);
    mock->set_recvBuf((unsigned char *)resp, (int)ntohs(resp->hdr.size));
    mock->set_sendCode(42);

    urx::RTDE_Recipe *r = new urx::RTDE_Recipe();
    double ts;
    r->add_field("timestamp", &ts);
    BOOST_CHECK(h->register_recipe(r));

    // Parse incoming data with the active input Recipes
    struct rtde_data_package dp;
    memset(&dp, 0, sizeof(struct rtde_data_package));

    // unknown recpie_id should fail
    BOOST_CHECK(!h->parse_incoming_data(NULL));
    BOOST_CHECK(!h->parse_incoming_data(&dp));

    dp.recipe_id= 42;

    // Wrong datatype should fails
    BOOST_CHECK(!h->parse_incoming_data(&dp));

    dp.hdr.type = RTDE_DATA_PACKAGE;

    // 0 size  should fail
    BOOST_CHECK(!h->parse_incoming_data(&dp));

    // invalid size should fail (missing 7 bytes of storage for the double
    dp.hdr.size = htons(sizeof(struct rtde_data_package));
    BOOST_CHECK(!h->parse_incoming_data(&dp));

    dp.hdr.size = htons(sizeof(struct rtde_data_package) - sizeof(unsigned char) + sizeof(double));
    *(double *)&dp.data = urx::double_h(42.1337);

    // ok, valid package, should be ok
    BOOST_CHECK(h->parse_incoming_data(&dp));

    // Finally, retrieve the expected value:

    //FIXME: final testing of getting this field.
    BOOST_CHECK_CLOSE(ts, 42.1337, 0.000001);
    // BOOST_CHECK(h->getField<double>("timestamp") == 42.1337);
    free(resp);
}

BOOST_AUTO_TEST_CASE(test_handler_do_start)
{
    struct rtde_control_package_resp *resp = create_cp_resp();
    _set_recipe_resp(resp, "DOUBLE", 42);
    urx::RTDE_Recipe *r = new urx::RTDE_Recipe();
    double ts;

    r->add_field("timestamp", &ts);
    h->register_recipe(r);

    memset(resp, 0, 2048);

    // Invalid response from remote
    BOOST_CHECK(!h->start());

    struct rtde_control_package_sp_resp cpr;
    rtde_control_package_start((struct rtde_header *)&cpr);
    cpr.hdr.size = htons(4);
    cpr.accepted = false;
    mock->set_recvBuf((unsigned char *)&cpr, ntohs(cpr.hdr.size));

    BOOST_CHECK(!h->start());

    cpr.accepted = true;
    mock->set_recvBuf((unsigned char *)&cpr, ntohs(cpr.hdr.size));
    BOOST_CHECK(h->start());
    free(resp);
}

BOOST_AUTO_TEST_CASE(test_handler_full_large_data)
{
    // -----------------------------------------------------
    // Test Setup
    struct rtde_data_package *dp = (struct rtde_data_package *)buf_;
    dp->hdr.type = RTDE_DATA_PACKAGE;
    dp->recipe_id = 1;

    unsigned char *arr = rtde_data_package_get_payload(dp);
    double *ts_ptr = (double *)arr;
    int32_t jm_val[6];
    int32_t *jm_ptr = (int32_t *)(arr + sizeof(double));
    int32_t *rm_ptr = (int32_t *)(arr + sizeof(double)+ 6*sizeof(int32_t));
    double *jt_ptr = (double *)(arr + sizeof(double) + 6*sizeof(int32_t) + sizeof(int32_t));
    double jt_val[6];
    double ts_val = 42.1337;

    *ts_ptr = urx::double_h(ts_val);
    for (int c = 0; c < 6; ++c) {
        jm_val[c] = c+2;
        jm_ptr[c] = htobe32(jm_val[c]);
    }
    int32_t rm_val = 1;
    rm_ptr[0] = htobe32(rm_val);
    for (int c = 0; c < 6; ++c) {
        jt_val[c] = (c+123.456)*0.983;
        jt_ptr[c] = urx::double_h(jt_val[c]);
    }

    // -----------------------------------------------------
    // actual code

    // create recipe
    urx::RTDE_Recipe *r = new urx::RTDE_Recipe();
    double ts;
    r->add_field("timestamp", &ts);
    int32_t jm[6];
    r->add_field("joint_mode", jm);
    int32_t rm;
    r->add_field("robot_mode", &rm);
    double jt[6];
    r->add_field("joint_temperatures", jt);
    dp->hdr.size = htons(sizeof(struct rtde_data_package) + r->expected_bytes());

    struct rtde_control_package_resp *resp = create_cp_resp();
    _set_recipe_resp(resp, "DOUBLE,VECTOR6INT32,INT32,VECTOR6D", 1);
    mock->set_recvBuf((unsigned char *)resp, (int)ntohs(resp->hdr.size));
    mock->set_sendCode(11);

    // register recipe and ready parser
    h->register_recipe(r);

    // Notify UR that we a rready
    struct rtde_control_package_sp_resp cpr;
    rtde_control_package_start((struct rtde_header *)&cpr);
    cpr.hdr.size = htons(4);
    cpr.accepted = false;
    mock->set_recvBuf((unsigned char *)&cpr, ntohs(cpr.hdr.size));

    h->start();

    // Receive data, blocking call to handler, when return,

    dp->recipe_id = 42;
    mock->set_recvBuf(buf_, ntohs(cpr.hdr.size));
    BOOST_CHECK(!h->recv());

    dp->recipe_id = 1;
    mock->set_recvBuf(buf_, ntohs(dp->hdr.size));
    BOOST_CHECK(h->recv());

    BOOST_CHECK_CLOSE(ts, ts_val, 0.00001);
    for (int c = 0; c < 6; ++c)
        BOOST_CHECK(jm[c] == jm_val[c]);
    BOOST_CHECK(rm == rm_val);
    BOOST_CHECK(rm != 0);
    BOOST_CHECK(rm != 99);
    for (int c = 0; c < 6; ++c)
        BOOST_CHECK_CLOSE(jt[c], jt_val[c], 0.00001);

    // send stop
    rtde_control_package_start((struct rtde_header *)&cpr);
    cpr.hdr.size = htons(4);
    cpr.accepted = false;
    mock->set_recvBuf((unsigned char *)&cpr, ntohs(cpr.hdr.size));
    BOOST_CHECK(!h->stop());

    cpr.accepted = true;
    mock->set_recvBuf((unsigned char *)&cpr, ntohs(cpr.hdr.size));
    BOOST_CHECK(h->stop());
    free(resp);
}


BOOST_AUTO_TEST_CASE(test_register_recv_timestamp)
{
    urx::RTDE_Recipe *r = new urx::RTDE_Recipe();
    BOOST_ASSERT(!r->track_ts_ns(NULL));
    unsigned long ts = 42;
    BOOST_ASSERT(r->track_ts_ns(&ts));
    BOOST_ASSERT(ts == 0);
    delete r;
}

BOOST_AUTO_TEST_CASE(test_update_recv_timestamp)
{
    // Register for UR timestamp only
    struct rtde_control_package_resp *resp = create_cp_resp();
    _set_recipe_resp(resp, "DOUBLE", 7);
    mock->set_recvBuf((unsigned char *)resp, (int)ntohs(resp->hdr.size));
    mock->set_sendCode(42);

    // Create recipe, register for UR Ts and recv-ts
    urx::RTDE_Recipe *r = new urx::RTDE_Recipe();
    double ur_ts = 0.0;
    unsigned long ts = 42;
    r->add_field("timestamp", &ur_ts);
    r->track_ts_ns(&ts);
    BOOST_ASSERT(h->register_recipe(r));

    // Notify UR that we are ready
    struct rtde_control_package_sp_resp cpr;
    rtde_control_package_start((struct rtde_header *)&cpr);
    cpr.hdr.size = htons(4);
    cpr.accepted = true;
    mock->set_recvBuf((unsigned char *)&cpr, ntohs(cpr.hdr.size));

    BOOST_ASSERT(h->start());

    // Assemble data package
    struct rtde_data_package *dp = (struct rtde_data_package *)buf_;
    double *ts_ptr = (double *)rtde_data_package_get_payload(dp);
    *ts_ptr = urx::double_h(42.1337);

    dp->hdr.type = RTDE_DATA_PACKAGE;
    dp->recipe_id = 7;
    dp->hdr.size = htons(sizeof(struct rtde_data_package) + r->expected_bytes());

    mock->set_recvBuf(buf_, ntohs(dp->hdr.size));
    mock->set_sendCode(42);
    mock->set_recv_ts(12345);

    BOOST_CHECK(h->recv());
    BOOST_CHECK_CLOSE(ur_ts, 42.1337, 0.00001);
    BOOST_ASSERT(ts == 12345);
    h->stop();
    delete r;
}

BOOST_AUTO_TEST_CASE(test_handler_input_recipe)
{
    urx::RTDE_Recipe *in = new urx::RTDE_Recipe();
    in->dir_input();

    double slf = 1337.42;
    BOOST_CHECK(in->add_field("speed_slider_fraction", &slf));
    uint32_t slm = 0xdeadbeef;
    BOOST_CHECK(in->add_field("speed_slider_mask", &slm));

    mock->set_sendCode(-1);
    BOOST_CHECK(!h->register_recipe(in));

    // FIXME
    struct rtde_control_package_resp *resp = create_cp_resp();
    _set_recipe_resp(resp, "DOUBLE,UINT32", 42);
    mock->set_recvBuf((unsigned char *)resp, (int)ntohs(resp->hdr.size));
    mock->set_sendCode(42);      // random size, > 0
    BOOST_CHECK(h->register_recipe(in));
    free(resp);
}

BOOST_AUTO_TEST_CASE(test_handler_input_full)
{
    urx::RTDE_Recipe *in = new urx::RTDE_Recipe();
    in->dir_input();
    uint32_t slm = 0xdeadbeef;
    BOOST_CHECK(in->add_field("speed_slider_mask", &slm));
    double slf = 0.5;
    BOOST_CHECK(in->add_field("speed_slider_fraction", &slf));

    struct rtde_control_package_resp *resp = create_cp_resp();
    _set_recipe_resp(resp, "UINT32,DOUBLE", 42);
    mock->set_recvBuf((unsigned char *)resp, (int)ntohs(resp->hdr.size));
    mock->set_sendCode(42);
    BOOST_CHECK(h->register_recipe(in));
    BOOST_CHECK(in->recipe_id() == 42);

    // invalid recipe_id should fail
    BOOST_CHECK(!h->send(40));

    // should place a correct control_packge_in in the send-buffer
    unsigned char sbuf[256] = {0};
    mock->set_sendBuffer(sbuf, 256);

    BOOST_CHECK(h->send(42));
    struct rtde_data_package *data = (struct rtde_data_package *)sbuf;
    BOOST_CHECK(data != NULL);
    BOOST_CHECK(data->hdr.type == RTDE_DATA_PACKAGE);
    printf("size: %d\n", ntohs(data->hdr.size));

    unsigned char *buf = rtde_data_package_get_payload(data);

    BOOST_CHECK(ntohs(data->hdr.size) == (sizeof(*data) - sizeof(unsigned char) + 12));
    BOOST_CHECK(be32toh(*(uint32_t *)buf) == 0xdeadbeef);
    BOOST_CHECK_CLOSE(urx::double_be(*(double *)(buf+4)), 0.5, 0.00001);
    free(resp);
}

BOOST_AUTO_TEST_SUITE_END()
