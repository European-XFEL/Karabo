import unittest

from karabo.native import Hash, Schema, NodeType

from karabo.config_db.utils import (
    hashFromBase64Bin, hashToBase64Bin,
    schemaToBase64Bin, schemaFromBase64Bin
)


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