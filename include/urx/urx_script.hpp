/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#ifndef URX_SCRIPT_HPP
#define URX_SCRIPT_HPP
#include <string>

namespace urx {
    class URX_Script
    {
    public:
        // shorthand for quickly setting up a new script
        URX_Script(const std::string fname)
        {
            set_script(fname);
        };

        URX_Script() : URX_Script("") {};
        ~URX_Script() =default;

        /**
         * \brief attempt to read the local file
         *
         * \return true if file is present and basic checks pass
         */
        bool read_file();

        /**
         * \brief returns a formatted script ready to push to an UR Controller
         *
         * \return string
         * \throws runtime_error if no file has been read
         */
        std::string& get_script();


        /**
         * \brief set new name for script to read
         *
         * This will invalidate all prior parsed scripts and reset the
         * script container.
         *
         * In the case of an invalid file, the old results will be
         * retained and false returned. A false here indicates that no
         * internal state has been changed.
         *
         * \returns true if valid file and it exists
         */
        bool set_script(const std::string fname);

        /**
         * \brief given the fname, read the script and return formatted.
         *
         * Makes it possible to reuse a script to read multiple
         * files. Note that internal state will be updated, so a call to
         * read_script() is destructive to an already read script.
         *
         * \returns string of preformatted script
         * \throws runtime_error if invalid file or parsing fails.
         */
        std::string& read_script(const std::string fname);

    protected:

        /**
         * \brief filename to read script from
         */
        std::string fname_;
        std::string script_;

        /**
         * \brief state of successful parsing of supplied fname_
         */
        bool parsed_;
    };

}
#endif  // URX_SCRIPT_HPP
