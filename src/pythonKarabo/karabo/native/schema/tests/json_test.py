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
import re
from json import dumps

import numpy

from karabo.native import (
    AccessMode, BoolValue, EnumValue, Hash, KaraboJSONEncoder, NoneValue,
    QuantityValue, StringValue, VectorStringValue)

test_pattern = re.compile(r"\s+")


def assert_json_equal(left, right):
    """remove all whitespace in comparison"""
    left = re.sub(test_pattern, '', dumps(left, cls=KaraboJSONEncoder))
    right = re.sub(test_pattern, '', right)
    assert left == right


def test_hash():
    h = Hash("a", 3, "b", True, "c", 2.2, "d", 1e100, "e", 2 ** 70,
             "f", ["a", 3], "g", numpy.arange(3), "h", None, "i", [],
             "j", (), "k", (5, 4))
    assert_json_equal(
        h, '''{"a": 3, "b": true, "c": 2.2, "d": 1e+100,
                "e": 1180591620717411303424, "f": ["a", 3], "g": [0, 1, 2],
                "h": null,  "i": [], "j": [], "k": [5, 4]}''')

    h = Hash("a", Hash(), "b", Hash("c", 3))
    assert_json_equal(h, '{"a": {}, "b": {"c": 3}}')


def test_karabo_value():
    h = Hash("a", BoolValue(False), "b", NoneValue(),
             "c", EnumValue(AccessMode.READONLY),
             "e", StringValue("def"), "f", VectorStringValue(["a", "b"]),
             "g", VectorStringValue([]), "h", QuantityValue(34),
             "i", QuantityValue(numpy.arange(2)))

    assert_json_equal(
        h, '''{"a": false, "b": null, "c": 2, "e": "def", "f": ["a", "b"],
            "g": [], "h": 34, "i": [0, 1]}''')
