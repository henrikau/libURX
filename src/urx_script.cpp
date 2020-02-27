/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#include <urx/urx_script.hpp>
#include <fstream>
#include <iostream>
#include <unistd.h>             // access()
#include <boost/algorithm/string.hpp>

bool urx::URX_Script::set_script(const std::string fname)
{
    if (fname.size() == 0)
        return false;

    if (access(fname.c_str(), F_OK)) {
        std::cout << __func__ << "() ERROR: file not found (" << fname_ << ")" << std::endl;
        return false;
    }

    fname_ = fname;
    parsed_ = false;
    return true;
}


bool urx::URX_Script::read_file()
{
    if (fname_.size() == 0)
        throw std::runtime_error("No filename provided");

    script_ = "";

    std::ifstream in(fname_, std::ios::out);
    if (!in.is_open()) {
        std::cerr << "Error: Unable to open file \"" << fname_ << "\" for reading!" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(in, line))
        script_.append(line + "\n");

    if (script_.size() <= 0)
        return false;

    parsed_ = true;
    return true;
}

std::string& urx::URX_Script::get_script()
{
    if (!parsed_)
        throw std::runtime_error("Script not read");

    // format script to make it ready for shipping (proper newlines etc)
    boost::replace_all(script_, "\r\n", "\n");
    boost::replace_all(script_, "\t", "        ");
    return script_;
}

std::string& urx::URX_Script::read_script(const std::string fname)
{
    if (!set_script(fname))
        throw std::runtime_error("Invalid filename provided");

    if (!read_file())
        throw std::runtime_error("Reading file failed");

    return get_script();
}
