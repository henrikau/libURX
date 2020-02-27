/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#include <urx/rtde_handler.hpp>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

bool urx::RTDE_Handler::set_version()
{
    // Construct frame for library default version
    struct rtde_prot prot;
    if (!rtde_mkreq_prot_version(&prot)) {
        std::cout << "Creating version request FAILED." << std::endl;
        return false;
    }

    if (con_->do_send_recv(&prot, ntohs(prot.hdr.size), buffer_, 1024) < 0) {
        std::cout << "do_sending request FAILED" << std::endl;
        return false;
    }
    return rtde_parse_prot_version((struct rtde_prot *)buffer_);
}

bool urx::RTDE_Handler::get_ur_version(struct rtde_ur_ver_payload& urver)
{
    struct rtde_header header;

    if (!rtde_mkreq_urctrl_version(&header)) {
        std::cout << "Creating URControl version query FAILED." << std::endl;
        return false;
    }
    if (con_->do_send_recv(&header, ntohs(header.size), buffer_, 1024) < 0) {
        std::cout << "do_sending request FAILED" << std::endl;
        return false;
    }

    if (!rtde_parse_urctrl_resp((struct rtde_ur_ver *)buffer_, &urver)) {
        std::cout << "Parsing returned UR-version FAILED" << std::endl;
        return false;
    }

    return (urver.major > 0 && urver.major < 7);
}

bool  urx::RTDE_Handler::send_emerg_msg(const std::string& msg)
{
    struct rtde_msg *rtde = rtde_msg_get(); // should be RAII
    if (!rtde)
        return false;

    const char *src = "the World";
    if (!rtde_mkreq_msg_out(rtde, msg.c_str(), strlen(msg.c_str())+1, src, strlen(src)+1))
        return false;

    int res = con_->do_send(rtde, ntohs(rtde->hdr.size));
    rtde_msg_put(rtde);
    return res > 0;
}

bool urx::RTDE_Handler::register_recipe(urx::RTDE_Recipe *r)
{
    if (!r)
        return false;

    struct rtde_header *hdr = r->GetMsg();
    if (!hdr)
        return false;

    auto sendcode = con_->do_send_recv(hdr, ntohs(hdr->size), buffer_, 2048);
    if (sendcode < 0) {
        std::cout << "do_sending ControlPackage FAILED" << std::endl;
        return false;
    }
    if (!r->register_response((struct rtde_control_package_resp *)buffer_)) {
        std::cout << __func__ << "() FAILED registering Recipe" << std::endl;
        return false;
    }

    if (r->dir_out()) {
        if (out != nullptr)
            std::cout << "WARNING: adding another Out-recipe (we can only have *one* outgoing Recipe!)" << std::endl;
        out = r;
    } else {
        recipes_in.insert({r->recipe_id(), r});
    }
    return true;
}

bool
urx::RTDE_Handler::start()
{
    struct rtde_header cp;
    rtde_control_package_start(&cp);
    struct rtde_control_package_sp_resp *resp = (struct rtde_control_package_sp_resp *)buffer_;
    int sendcode = con_->do_send_recv(&cp, ntohs(cp.size), (void *)resp, (int)sizeof(*resp));

    if (sendcode < 0)
        return false;
    else if (!rtde_control_package_sp_resp_validate(resp, true))
        return false;

    return resp->accepted;
}
bool
urx::RTDE_Handler::stop()
{
    struct rtde_header cp;
    rtde_control_package_stop(&cp);
    struct rtde_control_package_sp_resp *resp = (struct rtde_control_package_sp_resp *)buffer_;
    int sendcode = con_->do_send_recv(&cp, ntohs(cp.size), (void *)resp, (int)sizeof(*resp));

    if (sendcode < 0)
        return false;
    else if (!rtde_control_package_sp_resp_validate(resp, true))
        return false;
    return resp->accepted;
}

bool
urx::RTDE_Handler::parse_incoming_data(struct rtde_data_package* data)
{
    if (!rtde_parse_data_package(data))
        return false;

    int rid = data->recipe_id;
    if (out->recipe_id() != rid)
        return false;

    // expected size of buffer?
    int datasize = ntohs(data->hdr.size) - sizeof(struct rtde_data_package) + sizeof(unsigned char);
    if (out->expected_bytes() != datasize) {
        std::cout << "mismatch between expected bytes " << out->expected_bytes()<< " and actual "<< datasize << std::endl;
        return false;
    }
    return out->parse(&data->data);
}

//              FIXME
// this assumes all incoming data is data-pacakges that we can
// parse, where as we can, in fact, receive other data as well.
// Once fixed, update doc in .hpp
bool urx::RTDE_Handler::recv()
{
    int rcode = con_->do_recv(buffer_, 2048);
    if (rcode < 0)
        return false;

    return parse_incoming_data((struct rtde_data_package *)buffer_);
}

bool urx::RTDE_Handler::send(int recipe_id)
{
    for (auto& e : recipes_in) {
        urx::RTDE_Recipe *r = e.second;
        if (r->recipe_id() == recipe_id) {
            // create datamsg;
            struct rtde_data_package *dp = r->get_dp();
            r->store(dp);
            int sendcode = con_->do_send(dp, ntohs(dp->hdr.size));
            r->put_dp(dp);
            return (sendcode > 0);
        }
    }
    return false;
}
