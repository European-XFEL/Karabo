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
#include "DataLogTestDevice.hh"

namespace karabo {

    USING_KARABO_NAMESPACES

    KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::Device, DataLogTestDevice)

    const Epochstamp DataLogTestDevice::THREE_DAYS_AGO = Epochstamp() - TimeDuration(3, 0, 0, 0, 0);

    void DataLogTestDevice::expectedParameters(karabo::data::Schema& expected) {
        OVERWRITE_ELEMENT(expected)
              .key("state")
              .setNewOptions(State::INIT, State::ON)
              .setNewDefaultValue(State::INIT)
              .commit();

        INT32_ELEMENT(expected).key("oldValue").readOnly().initialValue(-1).commit();

        INT32_ELEMENT(expected).key("value").readOnly().initialValue(0).commit();

        VECTOR_INT32_ELEMENT(expected).key("vector").readOnly().initialValue({}).commit();

        INT32_ELEMENT(expected)
              .key("int32Property")
              .displayedName("Int32 property")
              .reconfigurable()
              .assignmentOptional()
              .defaultValue(32000000)
              .commit();

        INT32_ELEMENT(expected)
              .key("Int32NoDefault")
              .displayedName("Int32 without default")
              .reconfigurable()
              .assignmentOptional()
              .noDefaultValue()
              .commit();

        STRING_ELEMENT(expected)
              .key("stringProperty")
              .displayedName("String property")
              .description("A string property")
              .readOnly()
              .commit();

        SLOT_ELEMENT(expected).key("slotIncreaseValue").commit();

        SLOT_ELEMENT(expected).key("slotUpdateSchema").commit();
    }

    DataLogTestDevice::DataLogTestDevice(const karabo::data::Hash& input) : karabo::core::Device(input) {
        KARABO_SLOT(slotIncreaseValue);
        KARABO_SLOT(slotUpdateSchema, const karabo::data::Schema);
        // NOTE: this is a terrible idea. Never do this in the field.
        KARABO_SLOT(slotUpdateConfigGeneric, const karabo::data::Hash);
        KARABO_INITIAL_FUNCTION(initialize);
    }

    DataLogTestDevice::~DataLogTestDevice() {}

    void DataLogTestDevice::initialize() {
        set("oldValue", 99, Timestamp(DataLogTestDevice::THREE_DAYS_AGO, 0ull));

        updateState(State::ON);
    }

    void DataLogTestDevice::slotIncreaseValue() {
        set("value", get<int>("value") + 1);
    }

    void DataLogTestDevice::slotUpdateConfigGeneric(const Hash conf) {
        // this is a terrible idea, but is helpful in this test.
        // Do NOT use this pattern in any production system!
        set(conf);
    }

    void DataLogTestDevice::slotUpdateSchema(const Schema sch) {
        updateSchema(sch);
    }
} // namespace karabo
