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
#include <sched.h>
#include <linux/sched.h>

struct sched_attr {
        uint32_t size;
        uint32_t sched_policy;
        uint64_t sched_flags;

        /* SCHED_NORMAL, SCHED_BATCH */
        int32_t sched_nice;
        /* SCHED_FIFO, SCHED_RR */
        uint32_t sched_priority;
        /* SCHED_DEADLINE */
        uint64_t sched_runtime;
        uint64_t sched_deadline;
        uint64_t sched_period;
};

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

        // Clearing input registers on robot controller, assuming variables in recipe are initialized properly. If not done, it may read stuff from previous time robot was run.
        if (!send(r->recipe_id())) {
            std::cout << "Initial sending to handler using " << r->recipe_id() << " failed" << std::endl;
            return false;
        }
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

    if (sendcode < 0) {
        std::cerr << __func__ << "() do_send() failed" << std::endl;
        return false;
    }
    if (!rtde_control_package_sp_resp_validate(resp, true)) {
        std::cerr << __func__ << "() invalid response received, won't start." << std::endl;
        return false;
    }

    return resp->accepted;
}

bool
urx::RTDE_Handler::stop()
{
    usleep(10000);

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
urx::RTDE_Handler::parse_incoming_data(struct rtde_data_package* data, unsigned long rx_ts)
{
    if (!rtde_parse_data_package(data))
        return false;

    int rid = data->recipe_id;
    if (out->recipe_id() != rid)
        return false;

    // expected size of buffer?
    int datasize = ntohs(data->hdr.size) - sizeof(struct rtde_data_package) + sizeof(unsigned char);
    if (out->expected_bytes() != datasize) {
        // std::cout << "mismatch between expected bytes " << out->expected_bytes()<< " and actual "<< datasize << std::endl;
        return false;
    }
    return out->parse(&data->data, rx_ts);
}

//              FIXME
// this assumes all incoming data is data-pacakges that we can
// parse, where as we can, in fact, receive other data as well.
// Once fixed, update doc in .hpp
bool urx::RTDE_Handler::recv()
{
    unsigned long ts;
    int rcode = con_->do_recv(buffer_, 2048, &ts);
    if (rcode < 0) {
        std::cout << "do_recv() failed, code=" << rcode << ", buffer_=" << buffer_ << std::endl;
        return false;
    }

    return parse_incoming_data((struct rtde_data_package *)buffer_, ts);
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

#define US_IN_NS 1000
#define MS_IN_NS (1000*US_IN_NS)

bool _set_dl(uint64_t runtime_ns, uint64_t dl_ns, uint64_t period_ns)
{
    unsigned int flags = 0;
    struct sched_attr attr;
    attr.size = sizeof(struct sched_attr);
    attr.sched_flags	= 0;
    attr.sched_nice	= 0;
    attr.sched_priority	= 0;
    attr.sched_policy	= 6; // SCHED_DEADLINE
    attr.sched_runtime  = runtime_ns;
    attr.sched_deadline = dl_ns;
    attr.sched_period   = period_ns;
    return syscall(314, 0, &attr, flags) == 0;
}
