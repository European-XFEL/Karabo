# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.bound import (
    DOUBLE_ELEMENT, INT32_ELEMENT, KARABO_CLASSINFO, NODE_ELEMENT,
    TABLE_ELEMENT, VECTOR_INT32_ELEMENT, Hash, PythonDevice, Schema)


@KARABO_CLASSINFO("DeviceWithAlarm", "1.0")
class DeviceWithAlarm(PythonDevice):
    ALARM_HIGH = 1000.

    def expectedParameters(expected):
        (
            DOUBLE_ELEMENT(expected).key("valueWithAlarm")
            .readOnly()
            .alarmHigh(DeviceWithAlarm.ALARM_HIGH).needsAcknowledging(False)
            .commit(),

            NODE_ELEMENT(expected).key("node")
            .commit(),

            INT32_ELEMENT(expected).key("node.number")
            .reconfigurable()
            .assignmentOptional()
            .defaultValue(0)
            .commit(),

            VECTOR_INT32_ELEMENT(expected).key("vector")
            .reconfigurable()
            .assignmentOptional()
            .defaultValue([0])
            .commit(),
        )
        tableRow = Schema()
        (
            INT32_ELEMENT(tableRow).key("int32")
            .readOnly()
            .commit(),

            TABLE_ELEMENT(expected).key("table")
            .reconfigurable()
            .setColumns(tableRow)
            .assignmentOptional()
            .defaultValue([Hash("int32", 1), Hash("int32", 2)])
            .commit(),
        )

    def __init__(self, config):
        super(DeviceWithAlarm, self).__init__(config)
