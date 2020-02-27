/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#define BOOST_TEST_MODULE urx_con
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <urx/con.hpp>
#include "test_server.hpp"

struct F
{
    F() :
        server_("127.0.0.1", urx::URX_PORT)
    {
        server_.start();
        usleep(50000);
        BOOST_TEST_MESSAGE( "setup fixture" );
    }
    ~F()
    {
        BOOST_TEST_MESSAGE( "teardown fixture" );
        server_.stop();
        usleep(10000);
    }
    test::TestServer server_;
};

BOOST_FIXTURE_TEST_SUITE(s, F)


BOOST_AUTO_TEST_CASE(test_urx_con_first)
{
    urx::Con con("127.0.0.1", urx::URX_PORT);
    const char *str = "foobar";

    BOOST_CHECK(con.do_send((void *)str, strlen(str)) == -1); // not connected
    BOOST_CHECK(con.do_connect());
    BOOST_CHECK(con.do_send((void *)str, strlen(str)));
    BOOST_CHECK(server_.get_last_tx_buf_sz() == strlen(str));
    BOOST_CHECK(server_.get_last_tx_buf() != NULL);
    BOOST_CHECK(strncmp(server_.get_last_tx_buf(), str, strlen(str)) == 0);
    usleep(10000);
}

BOOST_AUTO_TEST_CASE(test_con_disconnect)
{
    urx::Con con("127.0.0.1", urx::URX_PORT);

    BOOST_CHECK(!con.is_connected());
    BOOST_CHECK(con.do_connect());
    BOOST_CHECK(con.is_connected());
    con.disconnect();
}

BOOST_AUTO_TEST_SUITE_END()
