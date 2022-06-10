/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#ifndef RTDE_RECIPE_HPP
#define RTDE_RECIPE_HPP

#include <urx/magic.h>
#include <urx/header.hpp>
#include <urx/rtde_recipe_token.hpp>
#include <string>
#include <vector>
#include <exception>

namespace urx
{

    class RTDE_Recipe
    {
    public:
        RTDE_Recipe() :
            active_(false),
            recipe_id_(-1),
            bytes(0),
            dir_out_(true)
        {
            ts_ns_ = nullptr;
            cpo = rtde_control_msg_get(125.0);
            cpi = rtde_control_get_in();
            dp_ = rtde_data_package_get();
        };

        virtual ~RTDE_Recipe()
        {
            ts_ns_ = nullptr;
            for (auto &t: fields)
                delete t;
            rtde_control_msg_put(cpo);
            rtde_control_put_in(cpi);
            cpi = nullptr;
            cpo = nullptr;
        }

        bool add_field(std::string name, void *storage);
        std::string get_fields();

        bool track_ts_ns(unsigned long *ts_ns);

        /**
         * \brief Clear all fields stored in the recipe
         */
        void clear_fields();

        struct rtde_header * GetMsg();

        bool register_response(struct rtde_control_package_resp* resp);

        bool parse(unsigned char *buf) { return parse(buf, 0); };
        bool parse(unsigned char *buf, unsigned long ts);
        bool store(struct rtde_data_package *dp);

        bool active() { return active_; }
        int recipe_id() { return recipe_id_; }

        // Used mostly for testing to make sure we add recipe-tokens as
        // expected.
        int num_fields() { return fields.size(); }
        int expected_bytes() { return bytes; }

        /**
         * \brief set direction, default output (i.e. controller sends)
         *
         * Will fail if it alreay has tokens registered.
         */
        bool dir_output() { return set_dir(true); };
        bool dir_input() { return set_dir(false); };
        bool dir_out() { return dir_out_; }

        // need guarding for simultaneous access if we ever start
        // threading RTDE_Handler. This is a *very* naive
        // resource-guarding setup
        struct rtde_data_package * get_dp() { return dp_; }
        bool put_dp(struct rtde_data_package *rdp) { return rdp == dp_; }

    private:
        bool active_;
        int recipe_id_;

        // Timestamp for last complete update, either incoming or outgoing
        unsigned long *ts_ns_;

        std::vector<RTDE_Recipe_Token *> fields;
        struct rtde_control_package_out *cpo;
        struct rtde_control_package_in *cpi;
        int bytes;
        bool dir_out_;
        struct rtde_data_package *dp_;

        bool set_dir(bool dir_out);
        struct rtde_control_package_out * msg_out_();
        struct rtde_control_package_in * msg_in_();
    };
}

#endif  // RTDE_RECIPE_HPP
