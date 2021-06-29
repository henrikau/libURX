#include <urx/rtde_handler.hpp>

bool urx::RTDE_Handler::enable_tsn_proxy(const std::string& ifname,
                                         int prio,
                                         const std::string& dst_mac,
                                         const std::string& src_mac,
                                         uint64_t sid_out,
                                         uint64_t sid_in)

{
    try {
        socket_out = tsn::TSN_Talker::CreateTalker(prio, ifname, dst_mac);
        stream_out = tsn::TSN_Stream::CreateTalker(socket_out, sid_out, true);

        socket_in  = tsn::TSN_Listener::Create(ifname, src_mac);
        stream_in  = tsn::TSN_Stream::CreateListener(socket_in, sid_in);

    } catch (const std::runtime_error& e) {
        std::cerr << __func__  << "() Creating socket and talker failed" << std::endl;
        return false;
    }

    // FIXME: get size of recipe and notify talker
    tsn_mode = true;
    socket_in->set_ready();
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

    tsn_proxy = std::thread(&urx::RTDE_Handler::tsn_worker, this);
    rtde_proxy = std::thread(&urx::RTDE_Handler::rtde_worker, this);

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

void urx::RTDE_Handler::tsn_worker()
{
    // Grab incoming TSN frames, extract from TSN-header and forward
    // directly to robot via rtde
    {
        std::unique_lock<std::mutex> lk(bottleneck);
        auto timeout = std::chrono::system_clock::now() + std::chrono::milliseconds(100);
        if (!tsn_cv.wait_until(lk, timeout, [&] { return proxy_running; })) {
            std::cerr << __func__  << "() Creating rtde_worker FAILED, tsn_worker will abort" << std::endl;
            return ;
        }
    }

    if (!_set_dl(100 * US_IN_NS, 1 * MS_IN_NS, 2 * MS_IN_NS)) {
        std::cerr << __func__ << "() Failed setting deadline scheduler" << std::endl;
        proxy_running = false;
        return;
    }

    unsigned char buf[2048];
    memset(buf, 0, 2048);

    struct avtp_stream_pdu *pdu = (struct avtp_stream_pdu *)buf;
    while (proxy_running) {

        // wait for incoming tsn-frame, data will point into pdu
        uint8_t *data;
        int rsz = stream_in->recv(pdu, &data);

        // Very simple sanity-check, if header-size matches actual size,
        // then we're on to something and can pass that along
        struct rtde_data_package *dp = (struct rtde_data_package *)data;
        if (rsz == ntohs(dp->hdr.size)) {
            // This frame should be formatted already, so we don't have
            // to do any of the fiddling to get it ready
            int sendcode = con_->do_send(data, ntohs(dp->hdr.size));
            if (sendcode <= 0)
                proxy_running = false;
        }
        memset(buf, 0, rsz);
    }
}
