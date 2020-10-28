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
    if (tsn_mode)
        proxy_running = false;
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
    if (tsn_mode)
        return false;

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


bool urx::RTDE_Handler::enable_tsn_proxy(const std::string& ifname,
                                         int prio,
                                         const std::string& mac,
                                         uint64_t stream_id)

{
    try {
        socket_out = tsn::TSN_Talker::CreateTalker(prio, ifname, mac);
        stream_out = tsn::TSN_Stream::CreateTalker(socket_out, stream_id, true);
    } catch (std::runtime_error) {
        std::cerr << __func__  << "() Creating socket and talker failed" << std::endl;
        return false;
    }

    // FIXME: get size of recipe and notify talker

    tsn_mode = true;
    return true;
}

bool urx::RTDE_Handler::start_tsn_proxy()
{
    if (!tsn_mode)
        return false;

    if (!socket_out->ready()) {
        std::cerr << __func__ << "() talker not ready, cannot start TSN Proxy" << std::endl;
        return false;
    }

    proxy = std::thread(&urx::RTDE_Handler::tsn_proxy_worker, this);
    {
        std::unique_lock<std::mutex> lk(bottleneck);
        auto timeout = std::chrono::system_clock::now() + std::chrono::milliseconds(100);
        if (!tsn_cv.wait_until(lk, timeout, [&] { return proxy_running; })) {
            std::cerr << __func__  << "() Creating proxy-worker FAILED" << std::endl;
            return false;
        }
    }
    return true;
}

void urx::RTDE_Handler::tsn_proxy_worker()
{
    if (!socket_out->ready()) {
        tsn_cv.notify_all();
        return;
    }

    unsigned int flags = 0;
    struct sched_attr attr;
    attr.size = sizeof(struct sched_attr);
    attr.sched_flags	= 0;
    attr.sched_nice	= 0;
    attr.sched_priority	= 0;

    attr.sched_policy	= 6; // SCHED_DEADLINE
    attr.sched_runtime  =      500 * 1000; // 500us
    attr.sched_deadline = 2 * 1000 * 1000;
    attr.sched_period   = 2 * 1000 * 1000;
    int err = syscall(314, 0, &attr, flags);
    if (err != 0) {
        std::cerr << __func__ << "() Failed setting deadline scheduler" << std::endl;
        tsn_cv.notify_all();
        return;
    }

    proxy_running = true;
    tsn_cv.notify_all();

    // signal robot controller to start stream
    start();

    // start worker, grab data and forward
    while (proxy_running) {
        int rcode = con_->do_recv(buffer_, 2048);
        if (rcode < 0) {
            proxy_running = false;
            break;
        }

        stream_out->add_data(buffer_, rcode);
        stream_out->send();
    }
    std::cout << __func__ << "() worker stopped" << std::endl;
}
