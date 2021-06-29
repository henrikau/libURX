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

BOOST_AUTO_TEST_CASE(test_add_tsn_remote)
{
    struct rtde_control_package_resp *resp = create_cp_resp();
    _set_recipe_resp(resp, "DOUBLE,VECTOR6D", 42);
    mock->set_recvBuf((unsigned char *)resp, (int)ntohs(resp->hdr.size));
    mock->set_sendCode(42);

    urx::RTDE_Recipe *r = new urx::RTDE_Recipe();
    double ts;
    int32_t tq[6];
    r->add_field("timestamp", &ts);
    r->add_field("target_q", tq);

    BOOST_CHECK(h->register_recipe(r));
    BOOST_CHECK(h->enable_tsn_proxy("lo", 3, "01:00:5e:01:01:00", "01:00:5e:01:01:01", 42, 43));
}
BOOST_AUTO_TEST_SUITE_END()
