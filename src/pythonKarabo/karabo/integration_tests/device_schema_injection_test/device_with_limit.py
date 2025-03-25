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
    DOUBLE_ELEMENT, INT32_ELEMENT, KARABO_CLASSINFO, NODE_ELEMENT,
    TABLE_ELEMENT, VECTOR_INT32_ELEMENT, Hash, PythonDevice, Schema)


@KARABO_CLASSINFO("DeviceWithLimit", "1.0")
class DeviceWithLimit(PythonDevice):
    LIMIT_HIGH = 1000.

    def expectedParameters(expected):
        (
            DOUBLE_ELEMENT(expected).key("valueWithExc")
            .assignmentOptional()
            .defaultValue(0.0)
            .maxExc(DeviceWithLimit.LIMIT_HIGH)
            .reconfigurable()
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
        super().__init__(config)
