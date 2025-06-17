/*
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
#include "Runner.hh"

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <string>
#include <unordered_map>

#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/util/Version.hh"

using namespace karabo::data;

namespace karabo {
    namespace core {

        DeviceServer::Pointer Runner::instantiate(int argc, const char** argv) {
            const std::string classId = "DeviceServer";
            try {
                Hash config;
                bool ok = parseCommandLine(argc, argv, config);
                if (ok) {
                    if (!config.empty()) {
                        return Configurator<DeviceServer>::create(classId, config);
                    } else {
                        return Configurator<DeviceServer>::create(classId);
                    }
                }
            } catch (const karabo::data::Exception& e) {
                KARABO_RETHROW_AS(KARABO_INIT_EXCEPTION("Failed to instantiate DeviceServer."));
            }
            return DeviceServer::Pointer();
        }

        bool Runner::parseCommandLine(int argc, const char** argv, Hash& config) {
            if (argc < 2) return true;

            std::string arg1(argv[1]);
            if (arg1 == "--help" || arg1 == "-h") {
                if (argc > 2) {
                    // Show documentation of key
                    showUsage(std::string(argv[0]), std::string(argv[2]));
                    return false;
                } else {
                    showUsage(std::string(argv[0]));
                    return false;
                }
            } else if (arg1 == "--version" || arg1 == "-v") {
                std::cout << "Karabo-Version: " << karabo::util::Version::getVersion() << std::endl;
                return false;
            }

            for (int i = 1; i < argc; ++i) {
                std::string token(argv[i]);
                size_t pos = token.find('=');
                if (pos == std::string::npos) {
                    throw KARABO_PARAMETER_EXCEPTION("Parameter format requires a '=': <key>=<value>. Got '" + token +
                                                     "'");
                }
                std::string key = token.substr(0, pos);
                std::string value = token.substr(pos + 1);
                config.set(key, value);
            }

            return true;
        }

        void Runner::showUsage(const std::string& programName, const std::string& what) {
            std::cout << "\n ###################################################################\n"
                      << " #                   Karabo Device Server\n"
                      << " #\n"
                      << " # Karabo-Version: " << karabo::util::Version::getVersion() << "\n"
                      << " # Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.\n"
                      << " ###################################################################\n\n";

            if (what.empty()) {
                std::cout << "Usage: " << programName << " <configuration>\n\n";
                std::cout << "Positional arguments:\n";
                std::cout << "<configuration> A set of (hierarchical) <key>=<value> pairs\n";
                std::cout << "                Use: --help [key] to list available keys or sub-keys\n";
                DeviceServer::getSchema("DeviceServer").help();
            } else {
                DeviceServer::getSchema("DeviceServer").help(what);
            }
            std::cout << std::endl;
        }

    } // namespace core
} // namespace karabo
