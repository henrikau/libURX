/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#ifndef CON_HPP
#define CON_HPP
#include <iostream>
#include <string>
#include <stdbool.h>
#include <tsn/tsn_stream.hpp>

namespace urx {
    constexpr int RTDE_PORT = 30004;
    constexpr int URX_PORT = 30002;
    class Con
    {
    public:
        Con(const std::string remote, int port) :
            remote_(remote),
            port_(port),
            sock_(-1),
            connected_(false)
        {};

        virtual ~Con() { disconnect(); }

        // Avoid libc-names like 'connect', 'send' etc
        virtual void disconnect();
        virtual bool do_connect(bool nodelay = false);

        virtual int  do_send(void *sbuf, int ssz);
        virtual int  do_recv(void *rbuf, int rsz);
        virtual int  do_send_recv(void *sbuf, int ssz, void *rbuf, int rsz);

        bool is_connected() { return connected_; }
    protected:
        const std::string remote_;
        int port_;
        int sock_;
        bool connected_;
    };

    class TSNCon : public Con
    {
    public:
        TSNCon(std::shared_ptr<tsn::TSN_Stream> talker,
               std::shared_ptr<tsn::TSN_Stream> listener) :
            Con("127.0.0.1", 30002),
            talker(talker),
            listener(listener)
        {
            pdu = (struct avtp_stream_pdu *)recv_buf;
        };

        virtual void disconnect();
        virtual bool do_connect(bool nodelay);
        virtual int  do_send(void *sbuf, int ssz);
        virtual int  do_recv(void *rbuf, int rsz);
        virtual int  do_send_recv(void *sbuf, int ssz, void *rbuf, int rsz);

    private:
        std::shared_ptr<tsn::TSN_Stream> talker;
        std::shared_ptr<tsn::TSN_Stream> listener;
        uint8_t recv_buf[2048];
        struct avtp_stream_pdu *pdu;
    };
}

#endif
