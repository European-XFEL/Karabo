from json import dumps
import re
from unittest import TestCase, main

import numpy

from karabo.middlelayer import (
    AccessMode, BoolValue, EnumValue, Hash, KaraboJSONEncoder, NoneValue,
    QuantityValue, StringValue, VectorStringValue)


class Tests(TestCase):
    pattern = re.compile(r"\s+")

    def assert_json_equal(self, left, right):
        """remove all whitespace in comparison"""
        left = re.sub(self.pattern, '', dumps(left, cls=KaraboJSONEncoder))
        right = re.sub(self.pattern, '', right)
        self.assertEqual(left, right)

    def test_hash(self):
        h = Hash("a", 3, "b", True, "c", 2.2, "d", 1e100, "e", 2 ** 70,
                 "f", ["a", 3], "g", numpy.arange(3), "h", None, "i", [],
                 "j", (), "k", (5, 4))
        self.assert_json_equal(
            h, '''{"a": 3, "b": true, "c": 2.2, "d": 1e+100,
                   "e": 1180591620717411303424, "f": ["a", 3], "g": [0, 1, 2],
                   "h": null,  "i": [], "j": [], "k": [5, 4]}''')

        h = Hash("a", Hash(), "b", Hash("c", 3))
        self.assert_json_equal(h, '{"a": {}, "b": {"c": 3}}')

    def test_karabo_value(self):
        h = Hash("a", BoolValue(False), "b", NoneValue(),
                 "c", EnumValue(AccessMode.READONLY),
                 "e", StringValue("def"), "f", VectorStringValue(["a", "b"]),
                 "g", VectorStringValue([]), "h", QuantityValue(34),
                 "i", QuantityValue(numpy.arange(2)))

        self.assert_json_equal(
            h, '''{"a": false, "b": null, "c": 2, "e": "def", "f": ["a", "b"],
                "g": [], "h": 34, "i": [0, 1]}''')


if __name__ == "__main__":
    main()
