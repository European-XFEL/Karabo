# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
            .assignmentOptional().defaultValue([Hash("type", "INT", "name", "firstLine"),
                                                Hash("type", "BOOL", "name", "secondLine")])
            .init()
            .commit(),
        )

    def __init__(self, config):
        super(DeviceWithTableElementParam, self).__init__(config)
