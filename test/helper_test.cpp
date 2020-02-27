/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#define BOOST_TEST_MODULE helper_test
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <urx/helper.hpp>
BOOST_AUTO_TEST_SUITE(helper_test)

BOOST_AUTO_TEST_CASE(test_helper_absf)
{
    BOOST_CHECK_CLOSE(absf(1.0), 1.0, 1e-9);
    BOOST_CHECK_CLOSE(absf(-1.0), 1.0, 1e-9);
    BOOST_CHECK_CLOSE(absf(-1993721917482901.1784910384), 1993721917482901.1784910384, 1e-9);

    double v = 3.4;
    BOOST_CHECK_CLOSE(absf(v), 3.4, 1e-9);
    BOOST_CHECK_CLOSE(v, 3.4, 1e-9);
    v = -1437.3;
    BOOST_CHECK_CLOSE(absf(v), 1437.3, 1e-9);
    BOOST_CHECK_CLOSE(v, -1437.3, 1e-9);
}

BOOST_AUTO_TEST_SUITE_END()
