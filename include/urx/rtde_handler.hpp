/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#ifndef RTDE_HANDLER_HPP
#define RTDE_HANDLER_HPP
#include <memory>
#include <unordered_map>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

#include <urx/handler.hpp>
#include <urx/header.hpp>
#include <urx/con.hpp>
#include <urx/rtde_recipe.hpp>

#ifdef USE_TSN
#include <tsn/tsn_socket.hpp>
#include <tsn/tsn_stream.hpp>
#endif

namespace urx
{
    class RTDE_Handler : public Handler
    {
public:

    RTDE_Handler(Con* c) :
        Handler(c),
        out(nullptr),
        tsn_mode(false),
        proxy_running(false)
    {
        con_->do_connect();
    };

    RTDE_Handler(const std::string remote) :
        RTDE_Handler(new Con(remote, RTDE_PORT))
    {};

#ifdef USE_TSN
        RTDE_Handler(std::shared_ptr<tsn::TSN_Stream> talker,
                     std::shared_ptr<tsn::TSN_Stream> listener) :
            RTDE_Handler(new TSNCon(talker, listener))
        {
            tsn_mode = true;
            // setup tsn
        };
#endif

    /**
     * \brief Set protoocol version (currently v2)
     *
     * \return true if UR controller supports v2
     */
    bool set_version();

    /**
     * \brief Kick connection and open socket to remote
     *
     * \return true if UR controller supports v2
     */
    bool connect_ur() { return con_->do_connect(true); }

    /**
     * \return supported max version for librtde
     */
    int get_version() { return RTDE_SUPPORTED_PROTOCOL_VERSION; }

    /**
     * \brief Get installed SW on the UR Controller
     *
     * \return true if a valid response was received from UR
     */
    bool get_ur_version(struct rtde_ur_ver_payload& urver);

    /**
     * \brief send (emergency message) to UR
     *
     * The message will end up in the logs/ tab in the interface on the
     * controller. No ACK from the server is received, so there's no
     * guarantee that the message is actually received and displayed
     * UR-side.
     *
     * \return true if the bytes were sent
     */
    bool send_emerg_msg(const std::string& msg);

    // recipe for data flowing between EE and CON
    //
    // Output: CON -> EE
    // Input:  EE  -> CON
    // Based on the recipe-type, it will either add to output (what
    // robot controller sends to client (us))
    // Handler can have a *single* output recipe and up to 255 input recipes
    /**
     * \brief Register a new recipe with the handler
     *
     * The recipes are the main way of retrieving and and sending data
     * to the UR Controller.
     *
     * Output: CON -> EE
     * Input :  EE -> CON
     *
     * There can only be *one* output recipe but up to 255 input
     * recipes. Note that a single variable can only be present in a
     * single input recipe at a time.
     *
     * There are no limitations to how many *different* RTDE clients
     * that can be connected at the same time, but only one can receive
     * output from the robot. This is perhaps orthogonal to how one
     * would expect a robotics controller to operate...
     *
     * \input New recipe
     * \return true if valid recipe and it was accepted by the UR controller
     */
    bool register_recipe(urx::RTDE_Recipe *r);

    /**
     * \brief: start the stream of data relevant to the registered
     * output recipe
     *
     * If a valid output_recipe is registered with CON, data will start
     * to flow. Since there's only a single output_recipe, this does not
     * require a recipe_id to be provided.
     *
     * This also means that if you call this *before* registering a
     * recipe, but while CON has an old but valid recipe around, data
     * may start flowing.
     *
     * \return true if CON accepted the request.
     */
    bool start();

    /**
     * \brief: send an update of a specified input-recipe
     *
     * Given that one or more recipe-ids have been constructed and
     * successfully registred, this will take the specified recipe,
     * gather data from all the relevant memory locations and construct
     * a new DATA_PACKAGE and send to CON.
     *
     * \return true if it successfully compiled a new frame and it was sent OK
     */
    bool send(int recipe_id);

    /**
     * \brief receive new data (blocking)
     *
     * Data will be stored internally and handed over to
     * parse_incoming_data() for decoding. The decoded data will be
     * stored in the locations specified for the recipe-fields.
     *
     * Currently it assumes all incoming data is of type
     * RTDE_DATA_PACKAGE belonging to the output_recipe.

     * \return true if valid, parseable data was received.
     */
    virtual bool recv();

    /**
     * \brief kindly ask CON to stop sending output updates
     * \return true if request was accepted
     */
    bool stop();

    /**
     * \brief Apply the registered recipe(s) to the incoming data.
     *
     * \return true if it managed to parse the incoming data
     *
     * @param data: received data
     * @param rx_ts : local timestamp (from con) for when the data package was received
     * @returns true if incoming data was parsed correctly
     */
    bool parse_incoming_data(struct rtde_data_package* data) { return parse_incoming_data(data, 0); }
    bool parse_incoming_data(struct rtde_data_package* data, unsigned long rx_ts);

    bool enable_tsn_proxy(const std::string& ifname,
                              int prio,
                              const std::string& dst_mac, // talker
                              const std::string& src_mac, // listener
                              uint64_t sid_out,
                              uint64_t sid_in)
#ifdef USE_TSN
        ;
#else
        {
            (void) ifname;
            (void) prio;
            (void) dst_mac;
            (void) src_mac;
            (void) sid_out;
            (void) sid_in;
            throw std::runtime_error("Built without TSN support");
        }
#endif
    /** Start Proxy workers
     *
     * It will in turn signal robot controller to start sending
     * messages. Incoming frames will be forwarded to TSN stream.
     */
    bool start_tsn_proxy()
#ifdef USE_TSN
        ;
#else
        {
            throw std::runtime_error("Built without TSN support");
        }
#endif

private:
    RTDE_Recipe *out;
    std::unordered_map<int, RTDE_Recipe *> recipes_in;
    // URControl will return a frame of *maximum* 2000 bytes, so set
    // recv-buffer to handle that.
    unsigned char buffer_[2048];

    void rtde_worker();

    bool tsn_mode;
#ifdef USE_TSN
    std::shared_ptr<tsn::TSN_Talker> socket_out;
    std::shared_ptr<tsn::TSN_Stream> stream_out;
    std::shared_ptr<tsn::TSN_Listener> socket_in;
    std::shared_ptr<tsn::TSN_Stream> stream_in;
    void tsn_worker();
#endif


    std::mutex bottleneck;
    std::condition_variable tsn_cv;
    std::thread tsn_proxy;
    std::thread rtde_proxy;

    bool proxy_running;
};

}
#endif	// RTDE_HANDLER_HPP
