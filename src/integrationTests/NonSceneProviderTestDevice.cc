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
 * File:   NonSceneProviderTestDevice.cc
 * Author: steffen.hauf@xfel.eu
 *
 */

#include "NonSceneProviderTestDevice.hh"

using namespace std;

USING_KARABO_NAMESPACES

namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device, NonSceneProviderTestDevice)

    void NonSceneProviderTestDevice::expectedParameters(Schema& expected) {}


    NonSceneProviderTestDevice::NonSceneProviderTestDevice(const karabo::util::Hash& config) : Device(config) {
        KARABO_INITIAL_FUNCTION(initialize);
    }


    NonSceneProviderTestDevice::~NonSceneProviderTestDevice() {}


    void NonSceneProviderTestDevice::initialize() {}

} // namespace karabo
