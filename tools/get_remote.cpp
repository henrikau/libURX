/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#include <iostream>
#include <urx/rtde_handler.hpp>
#include <urx/helper.hpp>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

static int loopctr = 5000;
void write_file(const char *fname, std::vector<std::tuple<unsigned long,double>> ts)
{
    FILE *ts_log_fd = fopen(fname, "w");
    fprintf(ts_log_fd, "ts_loc_ns,ts_ur\n");

    for(const auto &i : ts) {
        fprintf(ts_log_fd, "%lu,%f\n", std::get<0>(i), std::get<1>(i));
    }

    fflush(ts_log_fd);
    fclose(ts_log_fd);
}
void usage(const char *argv0)
{
    std::cout << "Usage: " << std::endl;
    std::cout << argv0 << "-i UR_Controller_IPv4 -c CSV_File -l loops [-h help]" << std::endl;
}

int main(int argc, char *argv[])
{
    char ip4[16] = {0};
    char csv_file[256] = {0};
    int opt;
    while ((opt = getopt(argc, argv, "i:c:hl:")) != -1) {
        switch (opt) {
        case 'i':
            strncpy(ip4, optarg, 15);
            break;
        case 'c':
            strncpy(csv_file, optarg, 255);
            std::cout << "copy optarg (" << optarg << ") into csv_file" << std::endl;
            break;
        case 'l':
            loopctr = atoi(optarg);
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        default:
            usage(argv[0]);
            return 1;
        }
    }

    if (strlen(ip4) <= 0 || strlen(csv_file) <= 0)  {
        std::cerr << "Need IP for UR Controller and filename to log to!" << std::endl;
        usage(argv[0]);
        return 1;
    }

    urx::RTDE_Handler h(ip4);
    h.connect_ur();
    if (!h.set_version()) {
        std::cout << "Setting version FAILED" << std::endl;
        return -1;
    }

    double ur_ts;
    unsigned long ts_ns;
    urx::RTDE_Recipe out = urx::RTDE_Recipe();

    out.track_ts_ns(&ts_ns);
    out.add_field("timestamp", &ur_ts);

    if (!h.register_recipe(&out)) {
        std::cerr << __func__ << ": Failed setting output_recipe!" << std::endl;
        return -1;
    }

    std::cout << __func__ << ": Output-recipe set OK, got ID: " << \
        out.recipe_id() << ", starting handler" << std::endl;

    h.start();
    std::vector<std::tuple<unsigned long, double>> ts_set;
    for (int ctr = 0; ctr < loopctr; ctr++) {
        if (h.recv()) {
            ts_set.push_back(std::tuple<unsigned long, double>(ts_ns, ur_ts));
        }
    }
    h.stop();

    if (strlen(csv_file) > 0)
        write_file(csv_file, ts_set);

    std::cout << "Robot stopped" << std::endl;
    return 0;
}
