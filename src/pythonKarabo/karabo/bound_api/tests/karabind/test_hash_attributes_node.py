# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import unittest

import karabind

import karathon


class Tests(unittest.TestCase):
    def test_hashAttributesNode_getValueAs(self):
        def inner(Hash, HashAttributes, HashAttributesNode, Types):
            # Test "getKey", "getValue", "getType", "getValueAs",
            #      "setType", "setValue"
            h = Hash("a.b.c", "value")
            self.assertTrue(isinstance(h, Hash))
            h.setAttribute("a.b.c", "attr1", 10)
            h.setAttribute("a.b.c", "attr2", [True, False])
            attrs = h.getAttributes("a.b.c")
            self.assertTrue(isinstance(attrs, HashAttributes))
            n1 = attrs.getNode("attr1")
            self.assertTrue(isinstance(n1, HashAttributesNode))
            n2 = attrs.getNode("attr2")

            # Test "getKey"
            self.assertEqual(n1.getKey(), "attr1")
            self.assertEqual(n2.getKey(), "attr2")

            # Test "getType"
            self.assertEqual(n1.getType(), Types.INT32)
            self.assertEqual(n2.getType(), Types.VECTOR_BOOL)

            # Test "getValue"
            self.assertEqual(n1.getValue(), 10)
            self.assertEqual(n2.getValue(), [True, False])

            # Test "getValueAs"
            self.assertEqual(n1.getValueAs(Types.INT8), 10)
            self.assertEqual(n1.getValueAs(Types.UINT8), 10)
            self.assertEqual(n1.getValueAs(Types.INT16), 10)
            self.assertEqual(n1.getValueAs(Types.UINT16), 10)
            self.assertEqual(n1.getValueAs(Types.INT32), 10)
            self.assertEqual(n1.getValueAs(Types.UINT32), 10)
            self.assertEqual(n1.getValueAs(Types.INT64), 10)
            self.assertEqual(n1.getValueAs(Types.UINT64), 10)
            self.assertEqual(n1.getValueAs(Types.FLOAT), 10)
            self.assertEqual(n1.getValueAs(Types.DOUBLE), 10)
            if Types is karabind.Types:
                self.assertEqual(n1.getValueAs(Types.COMPLEX_FLOAT), (10+0j))
                self.assertEqual(n1.getValueAs(Types.COMPLEX_DOUBLE), (10+0j))
            self.assertEqual(n1.getValueAs(Types.STRING), '10')
            self.assertEqual(n1.getValueAs("STRING"), '10')
            self.assertEqual(
                n2.getValueAs("VECTOR_INT8"), bytearray(b'\x01\x00'))
            self.assertEqual(n2.getValueAs("VECTOR_INT16"), [1, 0])
            self.assertEqual(n2.getValueAs("VECTOR_UINT16"), [1, 0])
            self.assertEqual(n2.getValueAs("VECTOR_INT32"), [1, 0])
            self.assertEqual(n2.getValueAs("VECTOR_UINT32"), [1, 0])
            self.assertEqual(n2.getValueAs("VECTOR_INT64"), [1, 0])
            self.assertEqual(n2.getValueAs("VECTOR_UINT64"), [1, 0])
            if Types is karabind.Types:
                self.assertEqual(n2.getValueAs("VECTOR_FLOAT"), [1.0, 0.0])
                self.assertEqual(n2.getValueAs("VECTOR_DOUBLE"), [1.0, 0.0])
                self.assertEqual(
                    n2.getValueAs("VECTOR_COMPLEX_FLOAT"), [(1+0j), (0+0j)])
                self.assertEqual(
                    n2.getValueAs("VECTOR_COMPLEX_DOUBLE"), [(1+0j), (0+0j)])
                self.assertEqual(n2.getValueAs("VECTOR_STRING"), ['1', '0'])

            # Test 'setType' & 'setValue' for INT8
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("INT8")
            self.assertEqual(n1.getType(), Types.INT8)
            self.assertEqual(n1.getValue(), 10)
            n1.setValue(99)
            self.assertEqual(n1.getValue(), 99)

            # 'setType' & 'setValue' for UINT8
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("UINT8")
            self.assertEqual(n1.getType(), Types.UINT8)
            self.assertEqual(n1.getValue(), 10)
            n1.setValue(13)
            self.assertEqual(n1.getValue(), 13)

            # 'setType' & 'setValue' for INT16
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("INT16")
            self.assertEqual(n1.getType(), Types.INT16)
            self.assertEqual(n1.getValue(), 10)
            n1.setValue(3000)
            self.assertEqual(n1.getValue(), 3000)

            # 'setType' & 'setValue' for UINT16
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("UINT16")
            self.assertEqual(n1.getType(), Types.UINT16)
            self.assertEqual(n1.getValue(), 10)
            n1.setValue(64000)
            self.assertEqual(n1.getValue(), 64000)

            # 'setType' & 'setValue' for INT32
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("INT32")
            self.assertEqual(n1.getType(), Types.INT32)
            self.assertEqual(n1.getValue(), 10)
            n1.setValue(1_000_000_000)
            self.assertEqual(n1.getValue(), 1_000_000_000)

            # 'setType' & 'setValue' for UINT32
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("UINT32")
            self.assertEqual(n1.getType(), Types.UINT32)
            self.assertEqual(n1.getValue(), 10)
            n1.setValue(3_000_000_000)
            self.assertEqual(n1.getValue(), 3_000_000_000)

            # 'setType' & 'setValue' for INT64
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("INT64")
            self.assertEqual(n1.getType(), Types.INT64)
            self.assertEqual(n1.getValue(), 10)
            n1.setValue(9_223_372_036_854_775_807)
            self.assertEqual(n1.getValue(), 9_223_372_036_854_775_807)

            # 'setType' & 'setValue' for UINT64
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("UINT64")
            self.assertEqual(n1.getType(), Types.UINT64)
            self.assertEqual(n1.getValue(), 10)
            n1.setValue(18_446_744_073_709_551_600)
            self.assertEqual(n1.getValue(), 18_446_744_073_709_551_600)

            # 'setType' & 'setValue' for FLOAT
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("FLOAT")
            self.assertEqual(n1.getType(), Types.FLOAT)
            self.assertEqual(n1.getValue(), 10)
            n1.setValue(-3.141592)
            self.assertEqual(n1.getValue(), -3.141592)

            # 'setType' & 'setValue' for DOUBLE
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("DOUBLE")
            self.assertEqual(n1.getType(), Types.DOUBLE)
            self.assertEqual(n1.getValue(), 10)
            n1.setValue(2345.123456799)
            self.assertEqual(n1.getValue(), 2345.123456799)

            # 'setType' & 'setValue' for COMPLEX_FLOAT
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("COMPLEX_FLOAT")
            self.assertEqual(n1.getType(), Types.COMPLEX_FLOAT)
            self.assertEqual(n1.getValue(), (10+0j))
            n1.setValue(12+9.9j)
            self.assertEqual(n1.getValue(), (12+9.9j))

            # 'setType' & 'setValue' for COMPLEX_DOUBLE
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("COMPLEX_DOUBLE")
            self.assertEqual(n1.getType(), Types.COMPLEX_DOUBLE)
            self.assertEqual(n1.getValue(), (10+0j))
            n1.setValue(2.222222222-33.3333333333j)
            self.assertEqual(n1.getValue(), (2.222222222-33.3333333333j))

            attrs = h.getAttributes("a.b.c")
            # 'setType' & 'setValue' for STRING
            h.setAttribute("a.b.c", "attr1", 10)
            n1 = attrs.getNode("attr1")
            n1.setType(Types.STRING)
            self.assertEqual(n1.getType(), Types.STRING)
            self.assertEqual(n1.getValue(), '10')
            n1.setValue('3_000_000_000')
            self.assertEqual(n1.getValue(), '3_000_000_000')

            # Base64 encoding
            # 'setType' VECTOR_INT8
            # h.setAttribute("a.b.c", "attr1", 10)
            # attrs = h.getAttributes("a.b.c")
            # n1 = attrs.getNode("attr1")
            # n1.setType("VECTOR_INT8")
            # self.assertEqual(n1.getType(), Types.VECTOR_INT8)
            # self.assertEqual(n1.getValue(), [10])

            # 'setType' VECTOR_UINT8
            # h.setAttribute("a.b.c", "attr1", 10)
            # attrs = h.getAttributes("a.b.c")
            # n1 = attrs.getNode("attr1")
            # n1.setType("VECTOR_UINT8")
            # self.assertEqual(n1.getType(), Types.VECTOR_UINT8)
            # self.assertEqual(n1.getValue(), [10])

            # 'setType' VECTOR_INT16
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("VECTOR_INT16")
            self.assertEqual(n1.getType(), Types.VECTOR_INT16)
            self.assertEqual(n1.getValue(), [10])
            n1.setValue([8192, -16000])
            self.assertEqual(n1.getValue(), [8192, -16000])

            # 'setType' VECTOR_UINT16
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("VECTOR_UINT16")
            self.assertEqual(n1.getType(), Types.VECTOR_UINT16)
            self.assertEqual(n1.getValue(), [10])
            n1.setValue([64000, 61000])
            self.assertEqual(n1.getValue(), [64000, 61000])

            # 'setType' VECTOR_INT32
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("VECTOR_INT32")
            self.assertEqual(n1.getType(), Types.VECTOR_INT32)
            self.assertEqual(n1.getValue(), [10])
            n1.setValue([20064000, 1333332, 12, -402345])
            self.assertEqual(n1.getValue(), [20064000, 1333332, 12, -402345])

            # 'setType' VECTOR_UINT32
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("VECTOR_UINT32")
            self.assertEqual(n1.getType(), Types.VECTOR_UINT32)
            self.assertEqual(n1.getValue(), [10])
            n1.setValue([1, 2, 3, 4, 5])
            self.assertEqual(n1.getValue(), [1, 2, 3, 4, 5])

            # 'setType' VECTOR_INT64
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("VECTOR_INT64")
            self.assertEqual(n1.getType(), Types.VECTOR_INT64)
            self.assertEqual(n1.getValue(), [10])
            n1.setValue([-10064000, 2999999999999])
            self.assertEqual(n1.getValue(), [-10064000, 2999999999999])

            # 'setType' VECTOR_UINT64
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("VECTOR_UINT64")
            self.assertEqual(n1.getType(), Types.VECTOR_UINT64)
            self.assertEqual(n1.getValue(), [10])
            n1.setValue([478564000])
            self.assertEqual(n1.getValue(), [478564000])

            # 'setType' VECTOR_FLOAT
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("VECTOR_FLOAT")
            self.assertEqual(n1.getType(), Types.VECTOR_FLOAT)
            self.assertEqual(n1.getValue(), [10.0])
            n1.setValue([3.14])
            self.assertEqual(n1.getValue(), [3.14])

            # 'setType' VECTOR_DOUBLE
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType("VECTOR_DOUBLE")
            self.assertEqual(n1.getType(), Types.VECTOR_DOUBLE)
            self.assertEqual(n1.getValue(), [10.0])
            n1.setValue([111.22222, 33333.4444])
            self.assertEqual(n1.getValue(), [111.22222, 33333.4444])

            if Types is karabind.Types:
                # 'setType' VECTOR_COMPLEX_FLOAT
                h.setAttribute("a.b.c", "attr1", 10)
                attrs = h.getAttributes("a.b.c")
                n1 = attrs.getNode("attr1")
                n1.setType("VECTOR_COMPLEX_FLOAT")
                self.assertEqual(n1.getType(), Types.VECTOR_COMPLEX_FLOAT)
                self.assertEqual(n1.getValue(), [(10+0j)])
                n1.setValue([(1.2-3.2j), (5.4+3.3j)])
                self.assertEqual(n1.getValue(), [(1.2-3.2j), (5.4+3.3j)])

                # 'setType' VECTOR_COMPLEX_DOUBLE
                h.setAttribute("a.b.c", "attr1", 10)
                attrs = h.getAttributes("a.b.c")
                n1 = attrs.getNode("attr1")
                n1.setType("VECTOR_COMPLEX_DOUBLE")
                self.assertEqual(n1.getType(), Types.VECTOR_COMPLEX_DOUBLE)
                self.assertEqual(n1.getValue(), [(10+0j)])
                n1.setValue([(1.2-3.2j), (5.4+3.3j)])
                self.assertEqual(n1.getValue(), [(1.2-3.2j), (5.4+3.3j)])

            # 'setType' VECTOR_STRING
            h.setAttribute("a.b.c", "attr1", 10)
            attrs = h.getAttributes("a.b.c")
            n1 = attrs.getNode("attr1")
            n1.setType(Types.VECTOR_STRING)
            self.assertEqual(n1.getType(), Types.VECTOR_STRING)
            self.assertEqual(n1.getValue(), ['10'])
            n1.setValue(['cast', 'to', 'python', 'type'])
            self.assertEqual(n1.getValue(), ['cast', 'to', 'python', 'type'])

            h.setAttribute("a.b.c", "attr2", "test")
            n2 = attrs.getNode("attr2")
            self.assertEqual(n2.getType(), Types.STRING)
            n1.setValue(['karathon', 'karabind'])
            self.assertEqual(n1.getValue(), ['karathon', 'karabind'])

        # Run test in karabind version
        inner(karabind.Hash, karabind.HashAttributes,
              karabind.HashAttributesNode, karabind.Types)
        # Run test in karathon version
        inner(karathon.Hash, karathon.HashAttributes,
              karathon.HashAttributesNode, karathon.Types)
