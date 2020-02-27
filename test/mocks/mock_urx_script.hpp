/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#ifndef MOCK_URX_CON_SCRIPT
#define MOCK_URX_CON_SCRIPT

// We cheat, overload an object to get access to internal state to
// verify formatting routine
#include <urx/urx_script.hpp>
namespace urx {
    class URX_Script_mock : public URX_Script
    {
    public:
        URX_Script_mock() :
            URX_Script("/dev/null") {}
        void set_script(std::string new_script)
        {
            script_ = new_script;
            parsed_ = true;
        }
    };
}


#endif  // MOCK_URX_CON_SCRIPT
