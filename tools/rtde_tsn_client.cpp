#include <iostream>
#include <getopt.h>
#include <tsn/tsn_socket.hpp>
#include <tsn/tsn_stream.hpp>
#include <urx/rtde_handler.hpp>
#include <urx/helper.hpp>
#include <urx/rtde_recipe.hpp>

#include "tool_base.cpp"

double new_speed(const double err, const double prev_err, const double prev_speed)
{
    constexpr double k_p = 0.7;
    constexpr double k_i = 0.005;
    constexpr double k_d = 1.1;

    double p = k_p * err;
    double i = k_i *  prev_speed; // accumlated speed, not really summed error
    double d = (err - prev_err * k_d)/k_d;
    return  p + i + d;
}

int main(int argc, char *argv[])
{
    get_opts(argc, argv);

    std::shared_ptr<tsn::TSN_Talker> talker = tsn::TSN_Talker::CreateTalker(prio, ifname, src_mac);
    std::shared_ptr<tsn::TSN_Listener> listener = tsn::TSN_Listener::Create(ifname, dst_mac);
    urx::RTDE_Handler h = urx::RTDE_Handler(tsn::TSN_Stream::CreateTalker(talker, sid_in),
                                            tsn::TSN_Stream::CreateListener(listener, sid_out));
    listener->set_ready();

    // Acquire joint angles (what we aim for)
    std::vector<double> aq(urx::END);
    aq[urx::BASE]     = urx::deg_to_rad( 180.0);
    aq[urx::SHOULDER] = urx::deg_to_rad(-135.0);
    aq[urx::ELBOW]    = urx::deg_to_rad(  90.0);
    aq[urx::W1]       = urx::deg_to_rad( -90.0);
    aq[urx::W2]       = urx::deg_to_rad(  90.0);
    aq[urx::W3]       = urx::deg_to_rad( -90.0);

    // Output recipe (robot state)
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

    // Input recipe
    int32_t in_seqnr = 0;
    int32_t cmd = 0;
    std::vector<double> rqd(urx::END); // Ref angle speed (target_qd update)
    for (auto &r : rqd)
        r = 0.0;

    urx::RTDE_Recipe in =urx::RTDE_Recipe();
    in.dir_input();
    in.add_field("input_int_register_0", &in_seqnr);
    in.add_field("input_int_register_1", &cmd);
    in.add_field("input_double_register_0", &rqd[0]);
    in.add_field("input_double_register_1", &rqd[1]);
    in.add_field("input_double_register_2", &rqd[2]);
    in.add_field("input_double_register_3", &rqd[3]);
    in.add_field("input_double_register_4", &rqd[4]);
    in.add_field("input_double_register_5", &rqd[5]);
    if (!h.register_recipe(&in)) {
        printf("%s: Failed setting input_recipe!\n", __func__);
        return -1;
    }

    bool running = true;
    std::vector<double> prev_err(urx::END);

    int last_seqnr = 0;
    while (running) {
        if (!h.recv())
            continue;

        // Find joint-angle error and compute new joint speed
        for (int i = 0; i < urx::END; ++i) {
            double err = aq[i] - target_q[i];
            rqd[i] = new_speed(err, prev_err[i], rqd[i]);
            prev_err[i] = err;
        }

        // error acceptable?
        // close_vec(std::vector<double> actual, std::vector<double> target, const double epsilon);
        if (close_vec_unsafe(target_q, aq, 0.001)) {
            running = false;
            for (auto &r : rqd)
                r = 0.0;
        }

        // new speedsetting
        cmd = 2;
        in_seqnr = last_seqnr++;
        if (!(in_seqnr % 50)) {
            for (int i = 0; i < urx::END; ++i)
                printf("rqd[%d]=%f ", i, rqd[i]);
            printf("\n");
        }

        h.send(in.recipe_id());
        cmd = 0; // NO_COMMAND;
    }

    // Make sure we enter a safe sdtate (no speed and halt program)
    for (auto &r : rqd)
        r = 0.0;
    cmd = 3;
    h.send(in.recipe_id());

    h.stop();

    return 0;
}
