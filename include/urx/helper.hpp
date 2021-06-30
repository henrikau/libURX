/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#ifndef RTDE_HELPER_HPP
#define RTDE_HELPER_HPP
#include <string>
#include <vector>

namespace urx
{
std::string& trim(std::string& str)
{
    str.erase(str.find_last_not_of("\t\n\v\f\r ") + 1);
    str.erase(str.find_last_not_of(0x03) + 1); // ETX
    str.erase(0, str.find_first_not_of("\t\n\v\f\r "));
    str.erase(0, str.find_first_not_of(0x03)); // ETX
    return str;
}

const char * trim_to_char(std::string &str)
{
    return trim(str).c_str();
}

// This is a clumsy way of convertnig the units, but casting to uint64_t
// and passing to be64toh and back allows the compiler to do 'clever
// things', which totally breaks the convertion.
union converter {
    uint64_t raw;
    double val;
};

// read a 64 bit double in BE encoding and return host-valid
double double_be(volatile double in)
{
    union converter c1, c2;
    c1.val = in;
    c2.raw = be64toh(c1.raw);
    return c2.val;
}

// take a host-enocded 64-bit double and return it as big-endian (network-order)
double double_h(volatile double in)
{
    union converter c1, c2;
    c1.val = in;
    c2.raw = htobe64(c1.raw);
    return c2.val;
}

enum joint_n {
    BASE = 0,
    SHOULDER,
    ELBOW,
    W1,
    W2,
    W3,
    END,                        // not used, marker only
};

enum tcp_pose_n {
    TCP_x = 0,
    TCP_y,
    TCP_z,
    TCP_rx,
    TCP_ry,
    TCP_rz,
};

const double PI = 3.141592653589793;
double deg_to_rad(const double deg)
{
    return deg * PI / 180.0;
}

}

static inline double absf(const double val)
{
    unsigned char *ptr = (unsigned char *)&val;
    ptr[7] &= 0x7f;
    return val;
}

bool close_vec(std::vector<double> actual, std::vector<double> target, const double epsilon)
{
    if (actual.size() != target.size())
        return false;
    for (size_t c = 0; c<actual.size() ;c++) {
        if (absf(actual[c] - target[c]) > epsilon)
            return false;
    }
    return true;
}

bool close_vec_unsafe(double *actual, std::vector<double> target, const double epsilon)
{
    for (size_t c = 0; c < target.size() ; ++c) {
        if (absf(actual[c] - target[c]) > epsilon)
            return false;
    }
    return true;
}

#endif  // RTDE_HELPER_HPP
