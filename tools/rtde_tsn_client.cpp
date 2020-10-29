#include <iostream>
#include <getopt.h>
#include <tsn/tsn_socket.hpp>
#include <tsn/tsn_stream.hpp>
#include <urx/rtde_handler.hpp>
#include <urx/helper.hpp>
#include <urx/rtde_recipe.hpp>

#include "tool_base.cpp"

int main(int argc, char *argv[])
{
    get_opts(argc, argv);

    std::cout << "Setting up RTDE TSN Client:" << std::endl;
    std::cout << "TSN ifname: " << ifname << std::endl;
    std::cout << "dst-mac (tsn client): " << dst_mac << std::endl;
    std::cout << "src-mac (rtde proxy): " << src_mac << std::endl;
    std::cout << "TSN Stream priority: " << prio << std::endl;
    std::cout << "TSN Stream ID (out): " << sid_out << std::endl;
    std::cout << "TSN Stream ID (in): " << sid_in << std::endl;

    std::shared_ptr<tsn::TSN_Talker> talker = tsn::TSN_Talker::CreateTalker(prio, ifname, src_mac);
    std::shared_ptr<tsn::TSN_Listener> listener = tsn::TSN_Listener::Create(ifname, dst_mac);

    // Output recipe, what we get from the robot is caught by listener,
    // sid_in to be input-recipe response from us, to our talker
    urx::RTDE_Handler h = urx::RTDE_Handler(tsn::TSN_Stream::CreateTalker(talker, sid_in),
                                            tsn::TSN_Stream::CreateListener(listener, sid_out));

    listener->set_ready();

    // Get robot state
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

    // Set robot updates
    int32_t in_seqnr = 0;
    int32_t cmd = 0;
    double set_qd[urx::END];
    for (auto i = 0; i < urx::END; i++)
        set_qd[i] = 0;

    urx::RTDE_Recipe in =urx::RTDE_Recipe();
    in.dir_input();
    in.add_field("input_int_register_0", &in_seqnr);
    in.add_field("input_int_register_1", &cmd);
    in.add_field("input_double_register_0", &set_qd[0]);
    in.add_field("input_double_register_1", &set_qd[1]);
    in.add_field("input_double_register_2", &set_qd[2]);
    in.add_field("input_double_register_3", &set_qd[3]);
    in.add_field("input_double_register_4", &set_qd[4]);
    in.add_field("input_double_register_5", &set_qd[5]);

    if (!h.register_recipe(&in)) {
        printf("%s: Failed setting input_recipe!\n", __func__);
        return -1;
    }

    for (auto ctr = 0; ctr < 10; ctr++) {
        if (!h.recv())
            continue;

        in_seqnr = ctr;
        h.send(1);
        debug_print(ts, target_q);
    }
    h.stop();

    return 0;
}
