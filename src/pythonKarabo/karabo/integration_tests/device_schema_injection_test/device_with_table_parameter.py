from karabo.bound import (KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS,
                          PythonDevice, Schema, State,
                          STRING_ELEMENT, TABLE_ELEMENT)


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
            .assignmentOptional().noDefaultValue()
            .init()
            .commit(),
        )

    def __init__(self, config):
        super(DeviceWithTableElementParam, self).__init__(config)
