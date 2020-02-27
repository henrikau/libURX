/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#include <urx/urx_con.hpp>

bool urx::URX_Con::do_connect()
{
    if (connected_)
        return true;

    connected_ = true;
    return true;
}

bool urx::URX_Con::send(std::string script)
{
    (void)script;
    return true;
}
