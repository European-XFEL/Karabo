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
#include "DataLogNanTestDevice.hh"

namespace karabo {

    USING_KARABO_NAMESPACES

    KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::Device, DataLogNanTestDevice)


    void DataLogNanTestDevice::expectedParameters(karabo::data::Schema& expected) {
        INT32_ELEMENT(expected).key("int32Property").reconfigurable().assignmentOptional().defaultValue(3).commit();

        FLOAT_ELEMENT(expected)
              .key("floatProperty")
              .reconfigurable()
              .assignmentOptional()
              .defaultValue(3.141596f)
              .commit();

        DOUBLE_ELEMENT(expected)
              .key("doubleProperty")
              .reconfigurable()
              .assignmentOptional()
              .defaultValue(3.1415967773331)
              .commit();

        DOUBLE_ELEMENT(expected).key("doublePropertyReadOnly").readOnly().initialValue(3.1415967773331).commit();
    }

    DataLogNanTestDevice::DataLogNanTestDevice(const karabo::data::Hash& input) : karabo::core::Device(input) {}

    void DataLogNanTestDevice::preReconfigure(Hash& incomingReconfiguration) {
        if (incomingReconfiguration.has("doubleProperty")) {
            set("doublePropertyReadOnly", incomingReconfiguration.get<double>("doubleProperty"));
        }
    }
}; // namespace karabo
