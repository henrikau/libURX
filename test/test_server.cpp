/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
// Server side C/C++ program to demonstrate Socket programming
#include "test_server.hpp"

#include <thread>

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

void test::TestServer::set_resp(unsigned char *data, std::size_t dsize)
{
  if (!data || dsize > 2048)
        return;

    memset(out_data_, 0, dsize_);
    dsize_ = dsize;
    memcpy(out_data_, data, dsize_);
    std::cout << "New response set OK, " << dsize_ << " bytes" << std::endl;
}

void test::TestServer::serve()
{
    unsigned char in_buf[2048] = {0};

    if (listen(server_fd_, 3) < 0) {
        perror("failed listening on socket");
        running_ = false;
        return;
    }

    while (running_) {
        int new_socket = -1;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	int sz;

        if ((new_socket = accept4(server_fd_,
                                 (struct sockaddr *)&address,
                                  (socklen_t*)&addrlen, SOCK_CLOEXEC)) < 0 ) {
            running_ = false;
            continue;
        }
        if (!running_) {
            std::cout << "stop() tickled us, closing..." << std::endl;
            continue;
        }
        in_sz_ = read( new_socket , in_buf, 2048);
        memset(in_data_, 0, 2048);
        memcpy(in_data_, in_buf, in_sz_);

        sz = send(new_socket , out_data_ , dsize_, MSG_DONTWAIT);
	if (sz < 0) {
	  std::cout << "FAILED sending " << dsize_ << " to client, got " << sz << std::endl;
	  running_ = false;
	  continue;
	}

    }
    close(server_fd_);
    pthread_exit(NULL);
}

const char * test::TestServer::get_last_tx_buf()
{
    return (const char *)in_data_;
}

std::size_t test::TestServer::get_last_tx_buf_sz()
{
    // We need to makes ure that the server-thread has had a chance to
    // run and receive the data (the test-harness can be quite agressive
    // when pushing the framework).
    usleep(10000);
    return in_sz_;
}

bool test::TestServer::start()
{
    struct sockaddr_in address;
    int opt = 1;

    if ((server_fd_ = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        return false;

    // Test-ficture, OK to forcefully attach socket to port
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt))) {
        perror("setsockopt failed");
        return false;
    }

    address.sin_family = AF_INET;
    address.sin_port = htons( port_ );
    inet_pton(AF_INET, addr_.c_str(), &address.sin_addr);
    if (bind(server_fd_, (struct sockaddr *)&address,
             sizeof(address))<0) {
        perror("bind() failed");
        return false;
    }

    running_ = true;
    server_ = std::thread(&TestServer::serve, this);
    return true;
}

void test::TestServer::stop()
{
    if (!running_)
        return;

    running_ = false;

    // // This is somewhat of a hail-mary, trying to kick a listening
    // // server into action. We've disabled running_, now we have to get
    // // it out of accept()
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    const char *msg = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaahh, I'm dyyyyyiiiiiingghhhhhnrmf";

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    inet_pton(AF_INET, addr_.c_str(), &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cout << "connect failed!." << std::endl;
        return;
    }

    if (send(sock , &msg , strlen(msg) , 0) < 0)
      std::cout << "send failed!." << std::endl;

    std::cout << "message sent, awaiting closure" << std::endl;
    server_.join();
    std::cout << "thread stopped and joined" << std::endl;
}
