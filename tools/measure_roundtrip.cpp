/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#include <urx/rtde_handler.hpp>
#include <urx/urx_handler.hpp>
#include <urx/helper.hpp>
#include <unistd.h>
#include <iostream>

#include <chrono>
using namespace std::chrono;

void write_file(const char *fname, std::vector<std::tuple<long,double>> ts)
{
    FILE *ts_log_fd = fopen(fname, "w");
    fprintf(ts_log_fd, "ts_loc,ts_ur\n");

    for(const auto &i : ts) {
        fprintf(ts_log_fd, "%ld,%f\n", std::get<0>(i), std::get<1>(i));
    }

    fflush(ts_log_fd);
    fclose(ts_log_fd);
}

static FILE *tracefd = NULL;
void tag_tracebuffer(int expseqnr, int actseqnr, long diff_us)
{
    if (!tracefd)
        return;

    fprintf(tracefd, "expected_seqnr: %d, actual_seqnr: %d, diff_us: %ld\n", expseqnr, actseqnr, diff_us);
    fflush(tracefd);
}

void stop_trace(void)
{
    if (!tracefd)
        return;

    FILE *tracing_on = fopen("/sys/kernel/debug/tracing/tracing_on", "w");
    FILE *tfd;
    if (!tracing_on) {
        perror("Could not open tracing_on");
        exit(-1);
    }
    std::cerr << "closing trace" << std::endl;

    fprintf(tracing_on, "0\n");
    fflush(tracing_on);
    fclose(tracing_on);

    tfd = tracefd;
    tracefd = NULL;
    fclose(tfd);
}

int main(int argc, char *argv[])
{
    char ip4[16] = {0};
    char fpath[256] = {0};
    char csv_file[256] = {0};
    int opt;
    int loopctr = 5000;
    int trace_timeout = -1;
    while ((opt = getopt(argc, argv, "c:i:l:f:t:")) != -1) {
        switch (opt) {
        case 'c':
            strncpy(csv_file, optarg, 255);
            std::cout << "copy optarg (" << optarg << ") into csv_file" << std::endl;
            break;
        case 'i':
            strncpy(ip4, optarg, 15);
            break;
        case 'l':
            loopctr = atoi(optarg);
            if (loopctr <= 0) {
                std::cerr << "Invalid loopctr received" << std::endl;
                return -1;
            }
            break;
        case 'f':
            strncpy(fpath, optarg, 255);;
            break;
        case 't':
            std::cout << "Running with tracing enabled (assuming debugfs is properly mounted)" << std::endl;
            trace_timeout = atoi(optarg);
            if (trace_timeout > 2000) {
                tracefd = fopen("/sys/kernel/debug/tracing/trace_marker", "w");
                if (!tracefd)
                    perror("Unable to open trace-marker");
            }
        }
    }

    if (strlen(ip4) == 0 || strlen(fpath) == 0) {
        std::cerr << "Missing address (" << ip4 << ") of remote to attach to and file ("<<fpath<<")to upload." << std::endl;
        return -1;
    }

    urx::RTDE_Handler h(ip4);
    h.connect_ur();
    if (!h.set_version()) {
        std::cout << "Setting version FAILED" << std::endl;
        return -1;
    }

    urx::RTDE_Recipe out = urx::RTDE_Recipe();
    double ts;
    int32_t out_seqnr;
    out.add_field("timestamp", &ts);
    out.add_field("output_int_register_0", &out_seqnr);
    if (!h.register_recipe(&out)) {
        std::cerr << "Failed setting output recipe" << std::endl;
        return -1;
    }

    urx::RTDE_Recipe in = urx::RTDE_Recipe();
    in.dir_input();
    int32_t in_seqnr = 1336;
    in.add_field("input_int_register_0", &in_seqnr);
    if (!h.register_recipe(&in)) {
        std::cerr << "Failed setting input recipe" << std::endl;
        return -1;
    }


    h.start();
    in_seqnr = 1;
    h.send(in.recipe_id());

    urx::URX_Handler u(ip4);
    if (!u.upload_script(fpath)) {
        std::cerr << "Failed uploading script to robot" << std::endl;
        return -1;
    }
    // wait for UR to load script, compile and get ready
    usleep(1000000);
    h.recv();

    std::vector<std::tuple<long, double>> ts_set;
    long err_ctr = 100;

    bool horiz_found = false;
    std::vector<long> horiz;

    std::cout << "Running round-trip delay measurements, " << loopctr << " trips, failsafe-timeout of " << err_ctr << std::endl;
    for (int i = 0; i < loopctr; i++) {
        long start_us = duration_cast<microseconds >(system_clock::now().time_since_epoch()).count();
        in_seqnr++;
    again:
        h.send(in.recipe_id());
        if (--err_ctr < 0) {
            std::cerr << "Failed getting updated seqnr after 100 attempts, closing." << std::endl;
            break;
        }
        h.recv();
        long end_us = duration_cast<microseconds >(system_clock::now().time_since_epoch()).count();
        long diff = end_us - start_us;
        tag_tracebuffer(in_seqnr, out_seqnr, diff);

        if (in_seqnr != out_seqnr)
            goto again;

        ts_set.push_back(std::tuple<long, double>(end_us, ts));

        if (diff > trace_timeout)
            stop_trace();

        if (!horiz_found) {
            horiz.push_back(diff);
            if (horiz.size() > 1000) {
                long sum = 0;
                for(const auto &t : horiz) {
                    sum += t;
                }
                double avg = sum / horiz.size();
                if (avg > 5000) {
                    std::cout << "avg for 1k samples: " << avg << ", doing a sleep to try to find mismatch to thread" << std::endl;
                    usleep(750);
                }
                horiz_found = true;
            }
        }


        err_ctr = 100;
    }

    stop_trace();

    in_seqnr = 200000;          // trigger close in script
    h.send(in.recipe_id());

    if (strlen(csv_file) > 0)
        write_file(csv_file, ts_set);

    usleep(5000);

    return 0;
}
