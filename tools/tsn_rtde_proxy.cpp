#include <iostream>
#include <fstream>
#include <chrono>
#include <atomic>
#include <csignal>

#include <urx/rtde_handler.hpp>
#include <urx/helper.hpp>
#include <urx/urx_handler.hpp>
#include <unistd.h>


std::atomic<bool> running;
void signal_handler(int)
{
    std::cerr << "Stopping system.." << std::endl;
    running = false;
}

void usage(const char *argv0)
{
    std::cout << "Usage: " << argv0 << "options" << std::endl;

    std::cout << "    -d UR_Controller_IPv4 address" << std::endl;
    std::cout << "    -i TSN target interface" << std::endl;
    std::cout << "    -m TSN Destination MAC address " << std::endl;
    std::cout << "    -p TSN stream priority " << std::endl;
    std::cout << "    -s TSN Stream-ID " << std::endl;
    std::cout << "    -h show this help" << std::endl;
}

void printit(double ts, double target_q[urx::END])
{
    std::cout << ts;
    std::cout << " q[0]=" << target_q[0];
    std::cout << " q[1]=" << target_q[1];
    std::cout << " q[2]=" << target_q[2];
    std::cout << " q[3]=" << target_q[3];
    std::cout << " q[4]=" << target_q[4];
    std::cout << " q[5]=" << target_q[5] << std::endl;
}
union d {
    double a;
    uint64_t b;
};

int main(int argc, char *argv[])
{
    std::string ip4("127.0.0.1"), ifname("eth0"), dest_mac("01:00:5e:00:00:00");
    int prio = 3, sid = 42;
    int opt;

    while ((opt = getopt(argc, argv, "d:i:m:p:s:h")) != -1) {
        switch (opt) {
        case 'd':
            ip4 = optarg;
            break;
        case 'i':
            ifname = optarg;
            break;
        case 'm':
            dest_mac = optarg;
            break;
        case 'p':
            prio = atoi(optarg);
            break;
        case 's':
            sid = atoi(optarg);
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        default:
            usage(argv[0]);
            return 1;
        }
    }
    std::cout << "Setting up TSN RTDE Proxy:" << std::endl;
    std::cout << "UR ip4: " << ip4 << std::endl;
    std::cout << "TSN ifname: " << ifname << std::endl;
    std::cout << "dest-mac: " << dest_mac << std::endl;
    std::cout << "TSN Stream priority: " << prio << std::endl;
    std::cout << "TSN Stream ID: " << sid << std::endl;

    urx::RTDE_Handler h(ip4);
    h.connect_ur();
    if (!h.set_version()) {
        std::cout << "Setting version FAILED" << std::endl;
        return -1;
    }

    running = true;
    signal(SIGINT, signal_handler);

    urx::RTDE_Recipe out = urx::RTDE_Recipe();
    double ts;
    double target_q[urx::END];

    out.add_field("timestamp", &ts);
    out.add_field("target_q", target_q);
    if (!h.register_recipe(&out)) {
        printf("%s: Failed setting output_recipe!\n", __func__);
        return -1;
    }

    h.enable_tsn_proxy(ifname, prio, dest_mac, prio);
    h.start_tsn_proxy();

    while (running) {
        usleep(1000000);
        std::cout << "."; fflush(stdout);
    }

    h.stop();
    printf("Robot stopped\n");
    return 0;
}
