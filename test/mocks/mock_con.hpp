/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#ifndef MOCK_RTDE_CON_HPP
#define MOCK_RTDE_CON_HPP
#include <iostream>

#include <string.h>
#include <string>
#include <urx/con.hpp>

namespace urx {
    class Con_mock : public Con
    {
    public:
        Con_mock() :
            Con("mock.ex.org", 1),
            send_code(-1),
            ssz_(-1),
            rsz_(-1)
        {};

        virtual ~Con_mock() =default;

        void disconnect()
        {
            connected_ = false;
        };

        bool do_connect(bool nodelay = false) { (void)nodelay; connected_ = true; return true; };

        int do_send(void *buf , int sz)
        {
            if (!connected_)
                return -1;

            if (ssz_ > 0 && sz < ssz_ && buf)
                memcpy(sbuf_, buf, sz);

            return send_code;
        };

        int do_recv(void* rbuf, int rsz, unsigned long *rx_ns)
        {
            // avoid buffer overflows in test-harness
            if (rsz_ > rsz || rsz_ < 0 || rsz < 0)
                return -1;
            memcpy(rbuf, rbuf_, rsz_);
            if (rx_ns)
                *rx_ns = rx_ns_;
            return rsz_;
        };

        int do_send_recv(void *sbuf, int sz, void *rbuf, int rsz)
        {
            if (do_send(sbuf, sz) >= 0)
                return do_recv(rbuf, rsz, NULL);
            return -1;
        };

        // Mock specific hooks to adjust the behaviour
        void set_sendCode(int code) { send_code = code; };
        void set_recvBuf(unsigned char* rbuf, int rsz)
        {
            rbuf_ = rbuf;
            rsz_ = rsz;
        };
        void set_recv_ts(int ts_ns) { rx_ns_ = ts_ns; };

        // Set a reference to a buffer into which sent frames will be stored.
        void set_sendBuffer(unsigned char *buf, int ssz)
        {
            if (!buf)
                return;
            sbuf_ = buf;
            ssz_ = ssz;
        }
    private:
        int send_code;
        int ssz_;
        int rsz_;
        unsigned long rx_ns_;
        unsigned char *sbuf_;
        unsigned char *rbuf_;
    };
}
#endif // MOCK_RTDE_CON_HPP
