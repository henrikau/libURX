/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#ifndef URX_HANDLER_HPP
#define URX_HANDLER_HPP

#include <urx/handler.hpp>
#include <urx/con.hpp>

#include <urx/urx_script.hpp>

namespace urx {
    /**
     * \brief Overall handler of the URX connection
     */
class URX_Handler : public Handler
{
public:
    URX_Handler(Con *c) : Handler(c)
    {
        con_->do_connect();
    };

    URX_Handler(const std::string remote) : URX_Handler(new Con(remote, URX_PORT)) {};

    void disconnect() { con_->disconnect(); };

    bool upload_script(const std::string script_name);

private:
    URX_Script script_;
};
}
#endif  // URX_HANDLER_HPP
