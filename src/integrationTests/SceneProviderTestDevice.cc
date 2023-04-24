/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   SceneProviderTestDevice.cc
 * Author: steffen.hauf@xfel.eu
 *
 */

#include "SceneProviderTestDevice.hh"

using namespace std;

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