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
import unittest

from karabo.config_db.utils import (
    hashFromBase64Bin, hashToBase64Bin, schemaFromBase64Bin, schemaToBase64Bin)
from karabo.native import Hash, NodeType, Schema


class TestUtils(unittest.TestCase):

    def testHashConversions(self):
        h = Hash('a', 1, 'b', 2.0)
        encHash = hashToBase64Bin(h)
        decHash = hashFromBase64Bin(encHash)
        self.assertEqual(h, decHash)

    def testSchemaConversions(self):
        h = Hash("a", None)
        h["a", "nodeType"] = NodeType.Leaf.value
        h["a", "valueType"] = "INT32"
        h["a", "description"] = "a's description"
        h["a", "allowedStates"] = ["INIT", "UNKNOWN"]
        h["a", "unitSymbol"] = "A"
        h["a", "defaultValue"] = 22.5
        h["a", "alias"] = "Karabo"
        sch = Schema('blub', hash=h)
        encSch = schemaToBase64Bin(sch)
        decSch = schemaFromBase64Bin(encSch)
        self.assertEqual(sch.hash, decSch.hash)


if __name__ == '__main__':
    unittest.main()
