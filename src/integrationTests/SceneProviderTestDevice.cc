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
/*
 * File:   SceneProviderTestDevice.cc
 * Author: steffen.hauf@xfel.eu
 *
 */

#include "SceneProviderTestDevice.hh"

using namespace std;

using boost::placeholders::_1;

USING_KARABO_NAMESPACES

namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, SceneProviderTestDevice)

    void SceneProviderTestDevice::expectedParameters(Schema& expected) {
        VECTOR_STRING_ELEMENT(expected).key("availableScenes").readOnly().initialValue(std::vector<string>()).commit();
    }


    SceneProviderTestDevice::SceneProviderTestDevice(const karabo::util::Hash& config) : Device<>(config) {
        KARABO_INITIAL_FUNCTION(initialize);
        registerSlot<karabo::util::Hash>(boost::bind(&SceneProviderTestDevice::slotGetScenes, this, _1),
                                         "slotGetScenes");
    }


    SceneProviderTestDevice::~SceneProviderTestDevice() {}


    void SceneProviderTestDevice::initialize() {}

    void SceneProviderTestDevice::slotGetScenes(const karabo::util::Hash& args) {
        const std::vector<std::string> scenes = args.get<std::vector<std::string> >("scenes");
        const Hash sceneHash = Hash(scenes[0], "encoded(bar scene)");
        reply(sceneHash);
    }

} // namespace karabo
