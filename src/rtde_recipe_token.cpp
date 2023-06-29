/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#include <urx/rtde_recipe_token.hpp>
#include <urx/helper.hpp>
#include <iostream>

urx::RTDE_Recipe_Token_In::RTDE_Recipe_Token_In(std::string n, int idx, void *t) :
    RTDE_Recipe_Token(n,idx)
{
    type = rtde_input_names[idx].type;
    if (!t)
        type = NOT_FOUND;

    datasize = rtde_datasize[type];
    tokenStore = [](unsigned char *) { ; };
    switch(type) {
    case BOOL:
        // Fallthrough (same as for UINT8)
    case UINT8:
        tokenStore = [this, t](unsigned char *buf) { *(uint8_t *)(buf + offset) = *(uint8_t*)t; };
        break;
    case UINT32:
        tokenStore = [this, t](unsigned char *buf) { *(uint32_t *)(buf + offset) = htobe32(*(uint32_t *)t); };
        break;
        // no input case for UINT64
    case INT32:
        tokenStore = [this, t](unsigned char *buf) { *(int32_t *)(buf + offset) = htobe32(*(int32_t *)t); };
        break;
    case DOUBLE:
        tokenStore = [this, t](unsigned char *buf) { *(double *)(buf + offset) = double_h(*(double *)t); };
        break;
        // no input case for VECTOR3D
        // no input case for VECTOR6D
        // no input case for VECTOR6INT32
        // no input case for VECTOR6INT64
        // no input case for STRING
    case NOT_FOUND:
        datasize = 0;
        std::cout << "Type for " << n << " not known" << std::endl;
        break;
    default:
        break;
    }
}
bool urx::RTDE_Recipe_Token_In::store(unsigned char *buf)
{
    if (!buf)
        return false;
    tokenStore(buf);
    return true;
}


urx::RTDE_Recipe_Token_Out::RTDE_Recipe_Token_Out(std::string n, int idx, void *t) :
    RTDE_Recipe_Token(n,idx)
{
    type = rtde_output_names[idx].type;
    datasize =rtde_datasize[type];
    // FIXME: Determine if the variables are encodec in BE or LE
    // (probably BE, so must be changed...
    switch (type) {
    case BOOL:
        tokenParser = [this, t](unsigned char *buf) { *(bool *)t = (*(buf+offset) & 0xff) != 0x00; };
        break;
    case UINT8:
        tokenParser = [this, t](unsigned char *buf) { *(uint8_t *)t = *(uint8_t *)(buf+ offset); };
        break;
    case UINT32:
        tokenParser = [this, t](unsigned char *buf) { *(uint32_t *)t = be32toh(*(uint32_t *)(buf+offset)); };
        break;
    case UINT64:
        tokenParser = [this, t](unsigned char *buf) { *(uint64_t *)t = be64toh(*(uint64_t *)(buf+ offset)); };
        break;
    case INT32:
        tokenParser = [this, t](unsigned char *buf) { *(int32_t *)t = (int32_t)be32toh(*(uint32_t *)(buf+offset)); };
        break;
    case DOUBLE:
        tokenParser = [this, t](unsigned char *buf) {
            *(double *)t = double_be(*(double *)(buf + offset));
        };
        break;
    case VECTOR3D:
        tokenParser = [this, t](unsigned char *buf) {
            for (int c = 0; c < 3; ++c)
                ((double *)t)[c] = double_be(((double *)(buf + offset))[c]);
        };
        break;
    case VECTOR6D:
        tokenParser = [this, t](unsigned char *buf) {
            for (int c = 0; c < 6; ++c)
                ((double *)t)[c] = double_be(((double *)(buf + offset))[c]);
        };
        break;
    case VECTOR6INT32:
        tokenParser = [this, t](unsigned char *buf) {
            for (int c = 0; c < 6; ++c)
                ((int32_t *)t)[c] = (int32_t)be32toh(((uint32_t *)(buf+offset))[c]);
        };
        break;
    case VECTOR6UINT32:
        //                 !! NOTE !!
        // No listed value from URControl actually uses this datatype
        // add for 6 array, not for first only
        tokenParser = [this, t](unsigned char *buf) {
            for (int c = 0; c < 6; ++c)
                ((uint32_t *)t)[c] = be32toh(((uint32_t *)(buf+offset))[c]);
        };
        break;
    case STRING:
        // FIXME: add for 3 array, not for first only
        tokenParser = [this, t](unsigned char *buf) { strncpy((char *)buf, (char *)t, 32); };
        break;
    case NOT_FOUND:
    default:
        tokenParser = [](unsigned char *) { ; };
        std::cout << "Not implemented yet (" << name << ")"<< std::endl;
    }
}

bool urx::RTDE_Recipe_Token_Out::parse(unsigned char *buf)
{
    if (!buf)
        return false;
    tokenParser(buf);
    return true;
}

bool urx::RTDE_Recipe_Token::set_response_type(std::string resp_type)
{
    const char *resp_str = urx::trim_to_char(resp_type);
    return (type == data_name_to_type(resp_str, strlen(resp_str)));
}
