/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#define BOOST_TEST_MODULE urx_script
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <urx/urx_script.hpp>
#include "mocks/mock_urx_script.hpp"

struct F
{
    F() : script("ur_script_movej.script")
    {
        BOOST_TEST_MESSAGE("setup fixture");
    }
  ~F()
  {
      BOOST_TEST_MESSAGE( "teardown fixture" );
  }
    urx::URX_Script script;
};

BOOST_FIXTURE_TEST_SUITE(s, F)

BOOST_AUTO_TEST_CASE(test_script_unknown_file)
{
    urx::URX_Script urxs = urx::URX_Script("foobar");
    BOOST_CHECK_THROW(!urxs.read_file(), std::exception);
}

BOOST_AUTO_TEST_CASE(test_script_valid_file)
{
    urx::URX_Script urxs = urx::URX_Script("ur_script_movej.script");
    BOOST_CHECK(urxs.read_file());
}

BOOST_AUTO_TEST_CASE(test_script_get_empty_fails)
{
    BOOST_CHECK_THROW(script.get_script(), std::exception);
    BOOST_CHECK(script.read_file());
    std::string st = script.get_script();
    BOOST_CHECK(st.size() > 0);
}

BOOST_AUTO_TEST_CASE(test_script_proper_formatting)
{
    urx::URX_Script_mock ms = urx::URX_Script_mock();

    BOOST_CHECK(script.read_file());
    ms.set_script("first");
    BOOST_CHECK(ms.get_script() == "first");
    ms.set_script("first\r\n");
    BOOST_CHECK(ms.get_script() == "first\n");
}
BOOST_AUTO_TEST_CASE(test_script_parse_fail_on_empty_script)
{
    urx::URX_Script sc = urx::URX_Script();
    BOOST_CHECK_THROW(sc.read_file(), std::exception);
}

BOOST_AUTO_TEST_CASE(test_script_all_in_one)
{
    urx::URX_Script sc = urx::URX_Script();
    std::string res = sc.read_script("ur_script_movej.script");
    BOOST_CHECK(res.size() > 0);
}

BOOST_AUTO_TEST_CASE(test_script_set_new)
{
    std::string s_;
    BOOST_CHECK(script.read_file());

    // invalid file should fail and we should read old script
    BOOST_CHECK(!script.set_script("/dev/foo/bar"));
    s_ = script.get_script();
    BOOST_CHECK(s_.size() > 0);

    BOOST_CHECK(script.set_script("ur_script_movej.script"));
    BOOST_CHECK_THROW(script.get_script(), std::exception);
    BOOST_CHECK(script.read_file());
    s_ = script.get_script();
    BOOST_CHECK(s_.size() > 0);
}

BOOST_AUTO_TEST_SUITE_END()
