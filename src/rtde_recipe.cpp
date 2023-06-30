/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#include <urx/rtde_recipe.hpp>
#include <urx/rtde_recipe_token.hpp>
#include <urx/magic.h>
#include <string>
#include <sstream>

bool
urx::RTDE_Recipe::add_field(std::string name, void *storage)
{
    if (!storage)
        return false;

    urx::RTDE_Recipe_Token *token;
    int idx;

    if (dir_out_) {
        if ((idx = output_name_to_idx(name.c_str())) < 0)
            return false;
        token = new urx::RTDE_Recipe_Token_Out(name, idx, storage);
    } else {
        if ((idx = input_name_to_idx(name.c_str())) < 0)
            return false;
        token = new urx::RTDE_Recipe_Token_In(name, idx, storage);
    }

    token->set_offset(bytes);
    bytes += token->num_bytes();
    fields.push_back(token);
    return true;
}
bool
urx::RTDE_Recipe::track_ts_ns(unsigned long *ts_ns)
{
    if (!ts_ns)
        return false;
    ts_ns_ = ts_ns;

    // Reset value to have a known base
    *ts_ns_ = 0;
    return true;
}

std::string
urx::RTDE_Recipe::get_fields()
{
    std::string res="";
    for (auto &v : fields)
        res += v->name + ",";
    res.pop_back();
    return res;
}

void urx::RTDE_Recipe::clear_fields()
{
    fields.clear();
}

struct rtde_control_package_out * urx::RTDE_Recipe::msg_out_()
{
    if (!dir_out_)
        return NULL;

    rtde_control_msg_out(cpo, get_fields().c_str());
    return cpo;
}

struct rtde_control_package_in * urx::RTDE_Recipe::msg_in_()
{
    if (dir_out_)
        return NULL;

    rtde_control_msg_in(cpi, get_fields().c_str());
    return cpi;
}
// FIXME: This is screaming refactoring
struct rtde_header *urx::RTDE_Recipe::GetMsg()
{
    if (dir_out_)
        return (struct rtde_header *) msg_out_();
    return (struct rtde_header *) msg_in_();
}

bool
urx::RTDE_Recipe::register_response(struct rtde_control_package_resp* resp)
{
    if (!rtde_control_package_resp_validate(resp))
        return false;

    // unpack response and match to each Recipe_Token
    std::string variables = std::string(reinterpret_cast<const char*>(rtde_control_package_resp_get_payload(resp)));
    std::stringstream ss(variables);
    std::string item;
    std::vector<std::string> varlist;

    while (std::getline(ss, item, ','))
       varlist.push_back(item);

    // number of fields and token mismach
    if (varlist.size() != fields.size())
        return false;

    int i = 0;
    for (auto &t : fields) {
        if (!t->set_response_type(varlist[i])) {
            return false;
        }
        i++;
    }
    recipe_id_ = resp->recipe_id;
    active_ = true;
    return true;
}

bool
urx::RTDE_Recipe::parse(unsigned char *buf, unsigned long ts)
{
    if (!buf || !dir_out_)
        return false;

    for (auto &t : fields)
        if (!t->parse(buf))
            return false;

    // Update timestamp if it's being tracked
    if (ts_ns_)
        *ts_ns_ = ts;
    return true;
}

bool
urx::RTDE_Recipe::store(struct rtde_data_package *dp)

{
    if (!dp || dir_out_)
        return false;

    rtde_data_package_init(dp, recipe_id_, expected_bytes());
    unsigned char *data = rtde_data_package_get_payload(dp);

    for (auto &t : fields)
        if (!t->store(data))
            return false;
    return true;
}

// private
bool urx::RTDE_Recipe::set_dir(bool dir_out)
{
    if (dir_out_ == dir_out)
        return true;
    if (fields.size() > 0)
        return false;
    dir_out_ = dir_out;
    if (!dir_out_ && !dp_)
        dp_ = rtde_data_package_get();
    return true;
}
