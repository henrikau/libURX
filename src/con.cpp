/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#include <urx/con.hpp>
#include <arpa/inet.h>
#include <iostream>
#include <linux/tcp.h>

void urx::Con::disconnect()
{
    if (!connected_)
        return;
    connected_ = false;
    sock_ = -1;
}

bool urx::Con::do_connect(bool nodelay)
{
    if (connected_)
        return true;

    if ((sock_ = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return false;

    if (nodelay) {
        int flag = 1;
        if (setsockopt(sock_, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)))
            std::cerr << "Failed setting TCP_NODELAY on socket!" << std::endl;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_);
    if (inet_pton(AF_INET, remote_.c_str(), &serv_addr.sin_addr) <= 0)
        return false;

    if (connect(sock_, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "do_connecting to " << remote_ << " Failed!" << std::endl;
        return false;
    }

    connected_ = true;
    return connected_;

}

int urx::Con::do_send(void *sbuf, int ssz)
{
    if (!connected_)
        return -1;

    int written_sz  = send(sock_ , sbuf , ssz , 0);
    if (written_sz < ssz) {
        std::cout << "do_sending request FAILED! Expected " << ssz << " got: " << written_sz << std::endl;
        return -1;
    }
    return written_sz;
}

int urx::Con::do_recv(void *rbuf, int rsz)
{
    struct sockaddr src_addr;
    socklen_t addrlen;
    auto read_sz = recvfrom(sock_, rbuf, rsz, 0, &src_addr, &addrlen);
    if (read_sz < rsz)
        ((unsigned char *)rbuf)[read_sz] = 0x00;

    // FIXME: make sure we get from correct address..

    return read_sz;
}

int urx::Con::do_send_recv(void *sbuf, int ssz, void *rbuf, int rsz)
{
    auto sendsz = do_send(sbuf, ssz);
    if (sendsz < 0)
        return sendsz;
    return  do_recv(rbuf, rsz);
}
