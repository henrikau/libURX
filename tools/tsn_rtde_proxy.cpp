#include <iostream>
#include <fstream>
#include <chrono>
#include <atomic>
#include <csignal>

#include <urx/rtde_handler.hpp>
#include <urx/helper.hpp>
#include <urx/urx_handler.hpp>

#include "tool_base.cpp"

std::atomic<bool> running;
void signal_handler(int)
{
    std::cerr << "Stopping system.." << std::endl;
    running = false;
}

int main(int argc, char *argv[])
{

    get_opts(argc, argv);

    std::cout << "Setting up TSN RTDE Proxy:" << std::endl;
    std::cout << "UR ip4: " << ip4 << std::endl;
    std::cout << "TSN ifname: " << ifname << std::endl;
    std::cout << "dst-mac (tsn client): " << dst_mac << std::endl;
    std::cout << "src-mac (rtde proxy): " << src_mac << std::endl;
    std::cout << "TSN Stream priority: " << prio << std::endl;
    std::cout << "TSN Stream ID (out): " << sid_out << std::endl;
    std::cout << "TSN Stream ID (in): " << sid_in << std::endl;

    urx::URX_Handler urxh(ip4);
    urx::RTDE_Handler h(ip4);

    h.connect_ur();
    if (!h.set_version()) {
        std::cout << "Setting version FAILED" << std::endl;
        return -1;
    }
    if (!urxh.upload_script(sfile)) {
        std::cerr << "Uploading scrpit to UR FAILED" << std::endl;
        return -1;
    }

    // Setup output
    urx::RTDE_Recipe out = urx::RTDE_Recipe();
    int out_seqnr;
    double ts;
    double target_q[urx::END];
    double target_qd[urx::END];
    double target_qdd[urx::END];
    double target_moment[urx::END];

    out.add_field("output_int_register_0", &out_seqnr);
    out.add_field("timestamp", &ts);
    out.add_field("target_q", target_q);
    out.add_field("target_qd", target_qd);
    out.add_field("target_qdd", target_qdd);
    out.add_field("target_moment", target_moment);
    if (!h.register_recipe(&out)) {
        printf("%s: Failed setting output_recipe!\n", __func__);
        return -1;
    }

    int32_t in_seqnr = 0;
    int32_t cmd = 2;
    double wqd[urx::END];
    for (auto &r : wqd)
        r = 0.0;

    urx::RTDE_Recipe in =urx::RTDE_Recipe();
    in.dir_input();
    in.add_field("input_int_register_0", &in_seqnr);
    in.add_field("input_int_register_1", &cmd);
    in.add_field("input_double_register_0", &wqd[0]);
    in.add_field("input_double_register_1", &wqd[1]);
    in.add_field("input_double_register_2", &wqd[2]);
    in.add_field("input_double_register_3", &wqd[3]);
    in.add_field("input_double_register_4", &wqd[4]);
    in.add_field("input_double_register_5", &wqd[5]);
    if (!h.register_recipe(&in)) {
        printf("%s: Failed setting input_recipe!\n", __func__);
        return -1;
    }
    // Make sure we reset speed from the beginning to clear out any old values
    h.send(in.recipe_id());

    h.enable_tsn_proxy(ifname, prio, dst_mac, src_mac, sid_out, sid_in);
    h.start_tsn_proxy();

    running = true;
    signal(SIGINT, signal_handler);

    while (running) {

        usleep(1000000);
        debug_print(ts, target_q);
    }

    h.stop();
    printf("Robot stopped\n");
    return 0;
}
