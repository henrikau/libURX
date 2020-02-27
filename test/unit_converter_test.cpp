/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#define BOOST_TEST_MODULE unit_converter_test
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <urx/helper.hpp>

BOOST_AUTO_TEST_SUITE(unit_converter_test)

BOOST_AUTO_TEST_CASE(test_unit_double)
{
    double val = 1.0;
    BOOST_CHECK_CLOSE(val, urx::double_h(urx::double_be(val)), 0.00001);
    val = -2.0;
    BOOST_CHECK_CLOSE(val, urx::double_h(urx::double_be(val)), 0.00001);
    val = 1337.42;
    BOOST_CHECK_CLOSE(val, urx::double_h(urx::double_be(val)), 0.00001);
}

BOOST_AUTO_TEST_SUITE_END()
