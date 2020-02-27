/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#ifndef TEST_HELPERS_HPP
#define TEST_HELPERS_HPP

#include <stdlib.h>
#include <urx/header.hpp>

static inline struct rtde_control_package_resp * create_cp_resp()
{
    return (struct rtde_control_package_resp *) calloc(2048, 1);
}

static inline void _set_recipe_resp(struct rtde_control_package_resp *resp, std::string variables, int recipe_id)
{
    memset(resp, 0, 2048);
    resp->hdr.type = RTDE_CONTROL_PACKAGE_SETUP_OUTPUTS;
    resp->hdr.size = htons(sizeof(struct rtde_control_package_resp) + variables.length());
    resp->recipe_id = recipe_id & 0xff;
    char *dst = (char *)resp + sizeof(struct rtde_header) + sizeof(uint8_t);
    strncpy(dst, variables.c_str(), variables.length());
}

#endif  // TEST_HELPERS_HPP
