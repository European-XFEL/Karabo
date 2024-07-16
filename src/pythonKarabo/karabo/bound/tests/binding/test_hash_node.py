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

from karabo.bound import Hash, HashNode, Types


def test_hashNode():
    # Test "getKey", "setValue", "getValue", "getValueAs",
    #      "setAttribute", "getAttribute", "getAttributeAs",
    #      "hasAttribute", "setAttributes", "getAttributes",
    #      "copyAttributes", "getType", "setType"

    # 'getValueAs' for HashNode is testing also the other methods:
    # `getAs`, `getAttributeAs` for Hash and HashAttributesNode.
    # Only one conversion version is used.
    # One exception to that is Hash's `getAs` for few types.

    # BOOL
    # Boolean is something special which can be converted
    # to any other type
    h = Hash("a.b.c", True)
    assert isinstance(h, Hash)
    node = h.getNode("a.b.c")
    assert isinstance(node, HashNode)

    # Test "getKey"
    assert node.getKey() == 'c'

    # Test "getValue"
    assert node.getValue() is True

    # Test "getType"
    assert node.getType() == Types.BOOL

    # Test "setType"
    node.setType("VECTOR_BOOL")
    assert node.getType() == Types.VECTOR_BOOL
    assert node.getValue() == [True]
    node.setType(Types.VECTOR_INT32)
    assert node.getType() == Types.VECTOR_INT32
    assert node.getValue() == [1]

    # Test "setValue"
    node.setValue([1, 2, 3])
    assert node.getValue() == [1, 2, 3]
    node.setValue(2.7818281828)
    assert node.getValue() == 2.7818281828
    assert node.getType() == Types.DOUBLE

    # Test "getValueAs"
    node.setValue(True)
    assert node.getValueAs(Types.CHAR) == '1'
    assert node.getValueAs("INT8") == 1
    # "getValueAs" uses default conversion tested in
    # "test_hash_attributes_node.py"
    # .... skipped tests
    assert node.getValueAs(Types.DOUBLE) == 1.0
    assert node.getValueAs("COMPLEX_FLOAT") == (1 + 0j)
    assert node.getValueAs("COMPLEX_DOUBLE") == (1 + 0J)
    assert node.getValueAs(Types.STRING) == '1'
    assert node.getValueAs(Types.VECTOR_BOOL) == [True]
    assert node.getValueAs(Types.VECTOR_INT16) == [1]
    # .... skipped tests ...
    assert node.getValueAs(Types.VECTOR_FLOAT) == [1.0]
    assert node.getValueAs(Types.VECTOR_DOUBLE) == [1.0]
    assert node.getValueAs(Types.VECTOR_COMPLEX_FLOAT) == [(1 + 0j)]
    assert node.getValueAs(Types.VECTOR_COMPLEX_DOUBLE) == [(1 + 0j)]
    # Call is not supported ...
    # FIXME: VECTOR_STRING not supported in both cases

    # INT32
    # Special value 1 or 0 can be converted like boolean to any type...
    h = Hash('a.b.c', 1)
    n = h.getNode('a.b.c')
    assert n.getKey() == 'c'
    assert n.getValue() == 1
    assert n.getType() == Types.INT32
    assert n.getValueAs(Types.BOOL) is True
    assert n.getValueAs(Types.CHAR) == '1'
    assert n.getValueAs(Types.INT8) == 1
    # ... skipped tests ...
    assert n.getValueAs(Types.UINT64) == 1
    assert n.getValueAs(Types.FLOAT) == 1.0
    assert n.getValueAs(Types.DOUBLE) == 1.0
    assert n.getValueAs(Types.COMPLEX_FLOAT) == (1 + 0j)
    assert n.getValueAs(Types.COMPLEX_DOUBLE) == (1 + 0j)
    assert n.getValueAs(Types.STRING) == '1'
    assert n.getValueAs(Types.VECTOR_BOOL) == [True]
    # assert n.getValueAs(Types.VECTOR_CHAR) == ['1']
    # assert n.getValueAs(Types.VECTOR_INT8) == [1]
    # assert n.getValueAs(Types.VECTOR_UINT8) == [1]
    assert n.getValueAs(Types.VECTOR_INT16) == [1]
    # ... skipped tests
    assert n.getValueAs(Types.VECTOR_UINT64) == [1]
    assert n.getValueAs(Types.VECTOR_FLOAT) == [1.0]
    assert n.getValueAs(Types.VECTOR_DOUBLE) == [1.0]
    assert n.getValueAs(Types.VECTOR_COMPLEX_FLOAT) == [(1 + 0j)]
    assert n.getValueAs(Types.VECTOR_COMPLEX_DOUBLE) == [(1 + 0j)]
    # Call is not supported ...
    # assert n.getValueAs(Types.VECTOR_STRING) == ['1']

    h = Hash("a.b.c", 42)
    n = h.getNode("a.b.c")
    assert n.getValue() == 42
    assert n.getType() == Types.INT32
    assert n.getValueAs(Types.INT8) == 42
    # ... skipped tests
    assert n.getValueAs(Types.DOUBLE) == 42
    assert n.getValueAs(Types.COMPLEX_FLOAT) == (42 + 0j)
    assert n.getValueAs(Types.COMPLEX_DOUBLE) == (42 + 0j)
    assert n.getValueAs(Types.STRING) == '42'
    # Base64 encoding
    # Since we use `karabo::util::StringTools.hh` for conversion
    # the following types are converted using Base64 encoding, so
    # we skip them here...
    # assert n.getValueAs(Types.VECTOR_INT8) == [42]
    # assert n.getValueAs(Types.VECTOR_UINT8) == [42]
    assert n.getValueAs(Types.VECTOR_INT16) == [42]
    # ... skipped tests
    assert n.getValueAs(Types.VECTOR_UINT64) == [42]
    assert n.getValueAs(Types.VECTOR_FLOAT) == [42.0]
    assert n.getValueAs(Types.VECTOR_DOUBLE) == [42.0]
    assert n.getValueAs("VECTOR_COMPLEX_FLOAT") == [(42 + 0j)]
    assert n.getValueAs("VECTOR_COMPLEX_DOUBLE") == [(42 + 0j)]
    # Call is not supported...
    # assert n.getValueAs(Types.VECTOR_STRING) == '42'

    # Test "setAttribute"
    h = Hash("a", 42)
    h.setAttribute("a", "a", 15)
    node = h.getNode("a")
    assert isinstance(node, HashNode)

    # Test "getAttribute"
    assert node.getAttribute("a") == 15

    # Test "getAttributeAs"
    assert node.getAttributeAs("a", Types.INT8) == 15
    # ... skipped tests
    assert node.getAttributeAs("a", "DOUBLE") == 15
    assert node.getAttributeAs("a", Types.COMPLEX_FLOAT) == (15 + 0j)
    assert node.getAttributeAs("a", Types.COMPLEX_DOUBLE) == (15 + 0j)
    assert node.getAttributeAs("a", Types.STRING) == '15'
    # Base64 encoding
    # assert node.getAttributeAs("a", Types.VECTOR_INT8) == [15]
    # assert node.getAttributeAs("a", Types.VECTOR_UINT8) == [15]
    assert node.getAttributeAs("a", "VECTOR_INT16") == [15]
    # ... skipped tests
    assert node.getAttributeAs("a", Types.VECTOR_DOUBLE) == [15]
    assert node.getAttributeAs("a", Types.VECTOR_COMPLEX_FLOAT) == [
        (15 + 0j)]
    assert node.getAttributeAs("a", Types.VECTOR_COMPLEX_DOUBLE) == [
        (15 + 0j)]
    # Not supported
    # assert node.getAttributeAs("a", Types.VECTOR_STRING) == ['15']

    # Test "hasAttribute"
    assert node.hasAttribute("a")
    assert not node.hasAttribute("b")

    # Test "setAttributes", "getAttributes", "copyAttributes"
    h = Hash('a', 12)
    n1 = h.getNode('a')
    n1.setAttribute('a', 1)
    n1.setAttribute('b', True)
    n1.setAttribute('c', "test")
    n1.setAttribute('d', 3.141592)

    # Test "getAttributes"
    attrs = n1.getAttributes()
    assert attrs['a'] == 1
    assert attrs['d'] == 3.141592
    attrs['b'] = False
    assert not n1.getAttribute('b')

    h['b'] = "value"
    n2 = h.getNode('b')

    # Test "setAttributes"
    n2.setAttributes(attrs)  # copy
    n1.setAttribute('c', "newtest")
    assert n2.getAttribute('c') != "newtest"

    # Test "copyAttributes"
    attrs2 = n1.copyAttributes()
    attrs2['a'] = 2
    assert n1.getAttribute('a') != 2
