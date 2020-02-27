/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <urx/con.hpp>

namespace urx {
    class Handler{
    public:
        Handler(Con *c) :  con_(c) {};
        virtual ~Handler()
        {
            con_->disconnect();
            delete con_;
        }

    /**
     * \return true if connection is active and valid
     */
    bool is_connected() { return con_->is_connected(); }

    protected:
        Con* con_;
    };
}

#endif  // HANDLER_HPP
