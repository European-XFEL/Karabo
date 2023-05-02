/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   NonSceneProviderTestDevice.cc
 * Author: steffen.hauf@xfel.eu
 *
 */

#include "NonSceneProviderTestDevice.hh"

using namespace std;

USING_KARABO_NAMESPACES

namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, NonSceneProviderTestDevice)

    void NonSceneProviderTestDevice::expectedParameters(Schema& expected) {}


    NonSceneProviderTestDevice::NonSceneProviderTestDevice(const karabo::util::Hash& config) : Device<>(config) {
        KARABO_INITIAL_FUNCTION(initialize);
    }


    NonSceneProviderTestDevice::~NonSceneProviderTestDevice() {}


    void NonSceneProviderTestDevice::initialize() {}

} // namespace karabo
