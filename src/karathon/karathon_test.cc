/*
 * Author: CONTROLS DEV group
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

#include <boost/python.hpp>
#include <karabo/core/DeviceServer.hh>
#include <karabo/util/Hash.hh>

#include "Wrapper.hh"


namespace py = boost::python;
using namespace karabo::util;

namespace karathon {
    static std::map<std::string, boost::shared_ptr<karabo::core::DeviceServer>> testServersRegistry;


    static void _startDeviceServerPy(karabo::util::Hash& config) {
        using namespace karabo::core;
        if (!config.has("serverId")) config.set("serverId", "testDeviceServer");
        if (!config.has("Logger.priority")) config.set("Logger.priority", "FATAL");
        if (!config.has("scanPlugins")) config.set("scanPlugins", false);
        auto deviceServer = DeviceServer::create("DeviceServer", config);
        deviceServer->finalizeInternalInitialization();
        testServersRegistry[config.get<std::string>("serverId")] = deviceServer;
    }


    static void _stopDeviceServerPy(const std::string& serverId) {
        using namespace karabo::core;
        auto it = testServersRegistry.find(serverId);
        if (it == testServersRegistry.end()) return;
        testServersRegistry.erase(it);
    }

} // namespace karathon


void exportPyKarathonTestUtilities() {
    py::def("startDeviceServer", &karathon::_startDeviceServerPy, (py::arg("config")));
    py::def("stopDeviceServer", &karathon::_stopDeviceServerPy, (py::arg("serverId")));
}
