# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
from karabo.bound import (
    KARABO_CLASSINFO, STRING_ELEMENT, TABLE_ELEMENT, Hash, PythonDevice,
    Schema)


@KARABO_CLASSINFO("DeviceWithTableElementParam", "1.0")
class DeviceWithTableElementParam(PythonDevice):

    def expectedParameters(expected):

        row_schema = Schema()

        (
            STRING_ELEMENT(row_schema).key("type")
            .displayedName("Type")
            .description("Device Type")
            .assignmentOptional().noDefaultValue()
            .commit(),

            STRING_ELEMENT(row_schema).key("name")
            .displayedName("Name")
            .description("Device Name")
            .assignmentOptional().noDefaultValue()
            .commit(),

            TABLE_ELEMENT(expected).key("deviceTable")
            .displayedName("Device Table")
            .description("Table with devices types and names.")
            .setNodeSchema(row_schema)
            .assignmentOptional().defaultValue(
                [Hash("type", "INT", "name", "firstLine"),
                 Hash("type", "BOOL", "name", "secondLine")])
            .init()
            .commit(),
        )

    def __init__(self, config):
        super().__init__(config)
