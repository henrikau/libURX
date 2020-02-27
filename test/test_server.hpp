/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#ifndef TEST_SERVER_HPP
#define TEST_SERVER_HPP
#include <thread>
#include <cstddef>
#include <iostream>
#include <atomic>

namespace test
{
class TestServer
{
public:
    TestServer(const std::string addr, int port) :
        addr_(addr),
        port_(port),
        running_(false),
        server_fd_(-1),
        dsize_(0),
        in_sz_(0)
    {};
    TestServer() : TestServer("127.0.0.1", 30004) {};

    virtual ~TestServer() =default;

    // data to write back to a connecting client upon next request,
    // allows us to trigger faulty responses etc.
    void set_resp(unsigned char *data, std::size_t dsize);

    // start/stop the server
    bool start();
    void stop();
    void serve();

    // Get reference to the last tx buffer and size. Do *not* free this,
    // releas it using put_last_tx_buf()
    const char * get_last_tx_buf();
    std::size_t get_last_tx_buf_sz();

private:
    const std::string addr_;
    int port_;
    std::atomic<bool> running_;

    int server_fd_;
    std::thread server_;

    // Buffer for keeping response we want to send to whoever uses do_send()
    std::size_t dsize_;
    unsigned char out_data_[2048];

    // Use this to store the buffer sent by the caller, useful when
    // doing inspection of data actually transmitted.
    std::size_t in_sz_;
    unsigned char in_data_[2048];
};

}
#endif  // TEST_SERVER_HPP
