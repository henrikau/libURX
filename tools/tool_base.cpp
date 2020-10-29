#include <iostream>
#include <unistd.h>

void usage(const char *argv0)
{
    std::cout << "Usage: " << argv0 << "options" << std::endl;

    std::cout << "    -d UR_Controller_IPv4 address" << std::endl;
    std::cout << "    -i TSN target interface" << std::endl;
    std::cout << "    -m TSN Destination MAC address (consumer of TSN stream)" << std::endl;
    std::cout << "    -m TSN Source MAC address (sink of TSN stream, RTDE proxy)" << std::endl;
    std::cout << "    -p TSN stream priority " << std::endl;
    std::cout << "    -s TSN Stream-ID (output recipe)" << std::endl;
    std::cout << "    -S TSN Stream-ID (input recipe)" << std::endl;
    std::cout << "    -h show this help" << std::endl;
}

std::string ip4("127.0.0.1");
std::string ifname("eth0");
std::string dst_mac("01:00:5e:00:00:00");
std::string src_mac("01:00:5e:00:00:01");

int prio = 3;
int sid_out = 42;
int sid_in = 43;

void get_opts(int argc, char *argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "d:i:m:M:p:s:S:h")) != -1) {
        switch (opt) {
        case 'd':
            ip4 = optarg;
            break;
        case 'i':
            ifname = optarg;
            break;
        case 'm':
            dst_mac = optarg;
            break;
        case 'M':
            src_mac = optarg;
            break;
        case 'p':
            prio = atoi(optarg);
            break;
        case 's':
            sid_out = atoi(optarg);
            break;
        case 'S':
            sid_in = atoi(optarg);
            break;
        case 'h':
            usage(argv[0]);
            exit(0);
        default:
            usage(argv[0]);
            exit(1);
        }
    }
}


union debug {
    double fl;
    uint64_t hex;
};

void debug_print(double ts, double target_q[urx::END])
{
    printf("ts=%f", ts);
    for (auto i = 0; i < urx::END; i++)
        std::cout << " q["<<i<<"]=" << target_q[i];
    std::cout << std::endl;

    // union debug d;
    // for (auto i = 0; i < urx::END; i++) {
    //     d.fl = target_q[i];
    //     printf(" target_q[%d]= %f\t(0x%016lx)\n", i, d.fl, d.hex);
    // }

}
