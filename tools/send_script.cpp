/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#include <iostream>
#include <urx/urx_handler.hpp>

int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cout << "Wrong number of arguments, want:" << std::endl;
        std::cout << argv[0] << "<IP> <script>" << std::endl;
    }

    urx::URX_Handler h = urx::URX_Handler(argv[1]);
    if (!h.upload_script(argv[2])) {
        std::cout << "Failed uploading script to UR Controller" << std::endl;
        return 1;
    }

    std::cout << "Hello, world" << std::endl;
    return 0;
}
