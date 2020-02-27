#define BOOST_TEST_MODULE urx_handler
#define BOOST_TEST_DYN_LINK
#include <iostream>

#include <boost/test/unit_test.hpp>
#include <urx/urx_handler.hpp>
#include "mocks/mock_con.hpp"

struct F
{
    F()
    {
        BOOST_TEST_MESSAGE("setup fixture");
        mock = new urx::Con_mock();
        h = new urx::URX_Handler(mock);
        memset(buffer_, 0, 2048);
        mock->set_sendBuffer(buffer_, 2048);
        mock->set_sendCode(1);
    }
  ~F()
  {
      BOOST_TEST_MESSAGE( "teardown fixture" );
  }
    unsigned char buffer_[2048];
    urx::Con_mock* mock;
    urx::URX_Handler* h;
};

BOOST_FIXTURE_TEST_SUITE(s, F)

BOOST_AUTO_TEST_CASE(test_send_script_noexists)
{
    BOOST_CHECK(!h->upload_script("/dev/foo/bar"));
}

BOOST_AUTO_TEST_CASE(test_send_script)
{
    BOOST_CHECK(h->upload_script("ur_script_movej.script"));
    BOOST_CHECK(strncmp((const char *)buffer_, "# Summary:", 10) == 0);
}

BOOST_AUTO_TEST_SUITE_END()
