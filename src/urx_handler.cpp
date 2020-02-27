/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#include <urx/urx_handler.hpp>

bool urx::URX_Handler::upload_script(const std::string script_name)
{
    if (!script_.set_script(script_name)) {
        printf("%s: failed setting script\n", __func__);
        return false;
    }
    if (!script_.read_file()) {
        printf("%s: failed reading file\n", __func__);
        return false;
    }

    std::string s = script_.get_script();
    int res = con_->do_send((void *)s.c_str(), s.size());
    return res > 0;
}
