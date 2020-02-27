/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#ifndef RTDE_RECIPE_TOKEN_HPP
#define RTDE_RECIPE_TOKEN_HPP
#include <string>
#include <functional>
#include <urx/magic.h>

namespace urx
{
    struct RTDE_Recipe_Token
    {
        RTDE_Recipe_Token(std::string n, int idx) :
            name(n),
            magic_idx(idx),
            offset(0),
            ready(false)
        {};

        virtual ~RTDE_Recipe_Token() =default;

        // run the token-parser on the buffer,it will know where in the
        // buffer to look, the result will be stored in the location set
        // when Recipe_Token was created.
        virtual bool parse(unsigned char *) { return false; };

        // store the token into the out-buffer. Will do host-to-BE
        // transform and bytewise copy
        virtual bool store(unsigned char *) { return false; };

        // set the type returned from URControl, true if a valid
        // match. Will then be used for parsing data later
        bool set_response_type(std::string type);

        // Number of bytes this token use in a datastream
        uint8_t num_bytes() { return datasize; };

        void set_offset(std::size_t o) { offset = o; };

        std::string name;
        enum RTDE_DATA_TYPE type;
        int magic_idx;
        int datasize;
        size_t offset;        // offset into buffer
        bool ready;
    };

    struct RTDE_Recipe_Token_In : public RTDE_Recipe_Token
    {
        // expects magic_idx to be valid
        RTDE_Recipe_Token_In(std::string n, int idx, void* t);

        virtual ~RTDE_Recipe_Token_In() =default;

        virtual bool store(unsigned char *);
    private:
        std::function<void(unsigned char *)> tokenStore;
    };

    struct RTDE_Recipe_Token_Out : public RTDE_Recipe_Token
    {
        // expects magic_idx to be valid
        RTDE_Recipe_Token_Out(std::string n, int idx, void* t);
        virtual ~RTDE_Recipe_Token_Out() =default;

        virtual bool parse(unsigned char *buf) override;

    private:
        std::function<void(unsigned char *)> tokenParser;
    };


}

#endif
