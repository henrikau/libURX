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
    std::cout << "Usage: " << std::endl;
    std::cout << argv0 << "-i UR_Controller_IPv4 [-h help]" << std::endl;
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
    char ip4[16] = {0};
    int opt;

    while ((opt = getopt(argc, argv, "i:h")) != -1) {
        switch (opt) {
        case 'i':
            strncpy(ip4, optarg, 15);
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        default:
            usage(argv[0]);
            return 1;
        }
    }

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

    h.enable_tsn_proxy("eth2", 3, "01:00:5e:01:01:01", 42);
    h.start_tsn_proxy();


    // while (running) {
        usleep(1000000);
        std::cout << "."; fflush(stdout);
    // }

    h.stop();
    printf("Robot stopped\n");
    return 0;
}
