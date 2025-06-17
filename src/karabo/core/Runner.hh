/*
 * $Id: Runner.hh 4649 2011-11-04 17:46:36Z heisenb@DESY.DE $
 *
 * File:   Runner.hh
 * Author: <burkhard.heisen@xfel.eu>
 * Modified by: <krzysztof.wrona@xfel.eu>
 *
 * Created on December 1, 2011, 2:24 PM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef KARABO_CORE_RUNNER_HH
#define KARABO_CORE_RUNNER_HH

#include <string>
#include <vector>

#include "karabo/core/DeviceServer.hh"
#include "karabo/data/types/Hash.hh"

namespace karabo {
    namespace core {

        /**
         * @class Runner
         * @brief The Runner class starts device-servers in the distributed system
         *
         * The Runner class instantiates device-servers in the distributed system. It parses
         * command line arguments to deduce configuration.
         */

        class Runner {
           public:
            KARABO_CLASSINFO(Runner, "Runner", karabo::util::Version::getVersion())
            KARABO_CONFIGURATION_BASE_CLASS;

            /**
             * Instantiates a device server taking command line arguments into account
             *
             * Users of this function must check the returned pointer for validity.
             * The pointer may be empty in case the --help option is given.
             *
             * @param argc Number of commandline arguments
             * @param argv String array of commandline options
             * @return Pointer to device server instance (may be empty)
             */
            static DeviceServer::Pointer instantiate(int argc, const char** argv);
            static bool parseCommandLine(int argc, const char** argv, karabo::data::Hash& config);

           private:
            static void showUsage(const std::string& programName, const std::string& what = "");
        };

    } // namespace core
} // namespace karabo

#endif
