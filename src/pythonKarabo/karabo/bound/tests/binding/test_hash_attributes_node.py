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

from karabo.bound import Hash, HashAttributes, HashAttributesNode, Types


def test_hashAttributesNode_getValueAs():
    # Test "getKey", "getValue", "getType", "getValueAs",
    #      "setType", "setValue"
    h = Hash("a.b.c", "value")
    assert isinstance(h, Hash)
    h.setAttribute("a.b.c", "attr1", 10)
    h.setAttribute("a.b.c", "attr2", [True, False])
    attrs = h.getAttributes("a.b.c")
    assert isinstance(attrs, HashAttributes)
    n1 = attrs.getNode("attr1")
    assert isinstance(n1, HashAttributesNode)
    n2 = attrs.getNode("attr2")

    # Test "getKey"
    assert n1.getKey() == "attr1"
    assert n2.getKey() == "attr2"

    # Test "getType"
    assert n1.getType() == Types.INT32
    assert n2.getType() == Types.VECTOR_BOOL

    # Test "getValue"
    assert n1.getValue() == 10
    assert n2.getValue() == [True, False]

    # Test "getValueAs"
    assert n1.getValueAs(Types.INT8) == 10
    assert n1.getValueAs(Types.UINT8) == 10
    assert n1.getValueAs(Types.INT16) == 10
    assert n1.getValueAs(Types.UINT16) == 10
    assert n1.getValueAs(Types.INT32) == 10
    assert n1.getValueAs(Types.UINT32) == 10
    assert n1.getValueAs(Types.INT64) == 10
    assert n1.getValueAs(Types.UINT64) == 10
    assert n1.getValueAs(Types.FLOAT) == 10
    assert n1.getValueAs(Types.DOUBLE) == 10
    assert n1.getValueAs(Types.COMPLEX_FLOAT) == (10+0j)
    assert n1.getValueAs(Types.COMPLEX_DOUBLE) == (10+0j)
    assert n1.getValueAs(Types.STRING) == '10'
    assert n1.getValueAs("STRING") == '10'
    # Raises an exception:
    # assert n2.getValueAs("VECTOR_CHAR") == bytearray(b'\x01\x00')
    assert n2.getValueAs("VECTOR_INT8") == [1, 0]
    assert n2.getValueAs("VECTOR_UINT8") == [1, 0]
    assert n2.getValueAs("VECTOR_INT16") == [1, 0]
    assert n2.getValueAs("VECTOR_UINT16") == [1, 0]
    assert n2.getValueAs("VECTOR_INT32") == [1, 0]
    assert n2.getValueAs("VECTOR_UINT32") == [1, 0]
    assert n2.getValueAs("VECTOR_INT64") == [1, 0]
    assert n2.getValueAs("VECTOR_UINT64") == [1, 0]
    assert n2.getValueAs("VECTOR_FLOAT") == [1.0, 0.0]
    assert n2.getValueAs("VECTOR_DOUBLE") == [1.0, 0.0]
    assert n2.getValueAs("VECTOR_COMPLEX_FLOAT") == [(1+0j), (0+0j)]
    assert n2.getValueAs("VECTOR_COMPLEX_DOUBLE") == [(1+0j), (0+0j)]
    assert n2.getValueAs("VECTOR_STRING") == ['1', '0']

    # Test 'setType' & 'setValue' for INT8
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("INT8")
    assert n1.getType() == Types.INT8
    assert n1.getValue() == 10
    n1.setValue(99)
    assert n1.getValue() == 99

    # 'setType' & 'setValue' for UINT8
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("UINT8")
    assert n1.getType() == Types.UINT8
    assert n1.getValue() == 10
    n1.setValue(13)
    assert n1.getValue() == 13

    # 'setType' & 'setValue' for INT16
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("INT16")
    assert n1.getType() == Types.INT16
    assert n1.getValue() == 10
    n1.setValue(3000)
    assert n1.getValue() == 3000

    # 'setType' & 'setValue' for UINT16
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("UINT16")
    assert n1.getType() == Types.UINT16
    assert n1.getValue() == 10
    n1.setValue(64000)
    assert n1.getValue() == 64000

    # 'setType' & 'setValue' for INT32
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("INT32")
    assert n1.getType() == Types.INT32
    assert n1.getValue() == 10
    n1.setValue(1_000_000_000)
    assert n1.getValue() == 1_000_000_000

    # 'setType' & 'setValue' for UINT32
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("UINT32")
    assert n1.getType() == Types.UINT32
    assert n1.getValue() == 10
    n1.setValue(3_000_000_000)
    assert n1.getValue() == 3_000_000_000

    # 'setType' & 'setValue' for INT64
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("INT64")
    assert n1.getType() == Types.INT64
    assert n1.getValue() == 10
    n1.setValue(9_223_372_036_854_775_807)
    assert n1.getValue() == 9_223_372_036_854_775_807

    # 'setType' & 'setValue' for UINT64
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("UINT64")
    assert n1.getType() == Types.UINT64
    assert n1.getValue() == 10
    n1.setValue(18_446_744_073_709_551_600)
    assert n1.getValue() == 18_446_744_073_709_551_600

    # 'setType' & 'setValue' for FLOAT
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("FLOAT")
    assert n1.getType() == Types.FLOAT
    assert n1.getValue() == 10
    n1.setValue(-3.141592)
    assert n1.getValue() == -3.141592

    # 'setType' & 'setValue' for DOUBLE
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("DOUBLE")
    assert n1.getType() == Types.DOUBLE
    assert n1.getValue() == 10
    n1.setValue(2345.123456799)
    assert n1.getValue() == 2345.123456799

    # 'setType' & 'setValue' for COMPLEX_FLOAT
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("COMPLEX_FLOAT")
    assert n1.getType() == Types.COMPLEX_FLOAT
    assert n1.getValue() == (10+0j)
    n1.setValue(12+9.9j)
    assert n1.getValue() == (12+9.9j)

    # 'setType' & 'setValue' for COMPLEX_DOUBLE
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("COMPLEX_DOUBLE")
    assert n1.getType() == Types.COMPLEX_DOUBLE
    assert n1.getValue() == (10+0j)
    n1.setValue(2.222222222-33.3333333333j)
    assert n1.getValue() == (2.222222222-33.3333333333j)

    attrs = h.getAttributes("a.b.c")
    # 'setType' & 'setValue' for STRING
    h.setAttribute("a.b.c", "attr1", 10)
    n1 = attrs.getNode("attr1")
    n1.setType(Types.STRING)
    assert n1.getType() == Types.STRING
    assert n1.getValue() == '10'
    n1.setValue('3_000_000_000')
    assert n1.getValue() == '3_000_000_000'

    # 'setType' VECTOR_INT8
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("VECTOR_INT8")
    assert n1.getType() == Types.VECTOR_INT8
    assert n1.getValue() == [10]

    # 'setType' VECTOR_UINT8
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("VECTOR_UINT8")
    assert n1.getType() == Types.VECTOR_UINT8
    assert n1.getValue() == [10]

    # 'setType' VECTOR_INT16
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("VECTOR_INT16")
    assert n1.getType() == Types.VECTOR_INT16
    assert n1.getValue() == [10]
    n1.setValue([8192, -16000])
    assert n1.getValue() == [8192, -16000]

    # 'setType' VECTOR_UINT16
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("VECTOR_UINT16")
    assert n1.getType() == Types.VECTOR_UINT16
    assert n1.getValue() == [10]
    n1.setValue([64000, 61000])
    assert n1.getValue() == [64000, 61000]

    # 'setType' VECTOR_INT32
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("VECTOR_INT32")
    assert n1.getType() == Types.VECTOR_INT32
    assert n1.getValue() == [10]
    n1.setValue([20064000, 1333332, 12, -402345])
    assert n1.getValue() == [20064000, 1333332, 12, -402345]

    # 'setType' VECTOR_UINT32
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("VECTOR_UINT32")
    assert n1.getType() == Types.VECTOR_UINT32
    assert n1.getValue() == [10]
    n1.setValue([1, 2, 3, 4, 5])
    assert n1.getValue() == [1, 2, 3, 4, 5]

    # 'setType' VECTOR_INT64
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("VECTOR_INT64")
    assert n1.getType() == Types.VECTOR_INT64
    assert n1.getValue() == [10]
    n1.setValue([-10064000, 2999999999999])
    assert n1.getValue() == [-10064000, 2999999999999]

    # 'setType' VECTOR_UINT64
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("VECTOR_UINT64")
    assert n1.getType() == Types.VECTOR_UINT64
    assert n1.getValue() == [10]
    n1.setValue([478564000])
    assert n1.getValue() == [478564000]

    # 'setType' VECTOR_FLOAT
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("VECTOR_FLOAT")
    assert n1.getType() == Types.VECTOR_FLOAT
    assert n1.getValue() == [10.0]
    n1.setValue([3.14])
    assert n1.getValue() == [3.14]

    # 'setType' VECTOR_DOUBLE
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("VECTOR_DOUBLE")
    assert n1.getType() == Types.VECTOR_DOUBLE
    assert n1.getValue() == [10.0]
    n1.setValue([111.22222, 33333.4444])
    assert n1.getValue() == [111.22222, 33333.4444]

    # 'setType' VECTOR_COMPLEX_FLOAT
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("VECTOR_COMPLEX_FLOAT")
    assert n1.getType() == Types.VECTOR_COMPLEX_FLOAT
    assert n1.getValue() == [(10+0j)]
    n1.setValue([(1.2-3.2j), (5.4+3.3j)])
    assert n1.getValue() == [(1.2-3.2j), (5.4+3.3j)]

    # 'setType' VECTOR_COMPLEX_DOUBLE
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType("VECTOR_COMPLEX_DOUBLE")
    assert n1.getType() == Types.VECTOR_COMPLEX_DOUBLE
    assert n1.getValue() == [(10+0j)]
    n1.setValue([(1.2-3.2j), (5.4+3.3j)])
    assert n1.getValue() == [(1.2-3.2j), (5.4+3.3j)]

    # 'setType' VECTOR_STRING
    h.setAttribute("a.b.c", "attr1", 10)
    attrs = h.getAttributes("a.b.c")
    n1 = attrs.getNode("attr1")
    n1.setType(Types.VECTOR_STRING)
    assert n1.getType() == Types.VECTOR_STRING
    assert n1.getValue() == ['10']
    n1.setValue(['cast', 'to', 'python', 'type'])
    assert n1.getValue() == ['cast', 'to', 'python', 'type']

    h.setAttribute("a.b.c", "attr2", "test")
    n2 = attrs.getNode("attr2")
    assert n2.getType() == Types.STRING
    n1.setValue(['karabim', 'karabom'])
    assert n1.getValue() == ['karabim', 'karabom']
