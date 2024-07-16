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

import pytest

from karabo.bound import Hash, HashAttributes, HashAttributesNode, Types


def test_hashAttributes():
    # Test "has", "__contains__", "isType", "getType", "erase",
    #      "__delitem__", "size", "__len__", "empty", "bool",
    #      "clear",  "getNode", "get", "__getitem__", "getAs", "set",
    #      "__setitem__", "__iter__"

    h = Hash("a.b.c", "value")
    attrs = h.getAttributes("a.b.c")
    assert isinstance(attrs, HashAttributes)

    # Test 'empty'
    assert attrs.empty()

    # Test 'bool'
    assert not attrs

    # Test 'has' ...
    assert not attrs.has('a.b.c')

    # Test 'set'...
    attrs.set("aq", 12345678993758)
    assert attrs.has("aq")

    # Test 'get' ...
    assert attrs.get("aq") == 12345678993758

    # Test "__len__"
    assert len(attrs) == 1
    # add the second attribute ...
    attrs.set("ua", "interesting")

    # Test "__getitem__"
    assert attrs["ua"] == "interesting"
    assert len(attrs) == 2

    # Test "isType"
    # assert attrs.isType("ua", "STRING") is True
    # assert attrs.isType("ua", Types.STRING) is True

    # Test "size"
    assert attrs.size() == 2

    # add one more attribute ...
    attrs['minerale'] = [True, False]
    assert attrs["minerale"] == [True, False]

    # Test "getType", "isType"
    # check attribute types ...
    assert attrs.getType('minerale') == Types.VECTOR_BOOL
    # assert attrs.isType('minerale', "VECTOR_BOOL") is True

    # Test "__contains__ ..."
    assert "aq" in attrs

    # Test "erase" ...
    attrs.erase("aq")
    assert "aq" not in attrs

    # Test "getNode"
    node = attrs.getNode("minerale")
    assert isinstance(node, HashAttributesNode)
    # Tests for node can be found in "test_hash_attributes_node.py".

    # Test "__delitem__" ... reference to "minerale" node
    del node
    assert "minerale" in attrs
    # delete attribute
    del attrs['minerale']
    assert 'minerale' not in attrs
    # attribute 'ua' is still in attrs container
    assert not attrs.empty()

    # Test "clear"
    attrs.clear()
    assert attrs.empty()
    assert not bool(attrs)

    # set new attributes ...
    # Test 'set'
    attrs.set("a1", 12)
    attrs.set("a2", True)

    # Test "__setitem__" ...
    attrs["a3"] = 120_333_666
    attrs["a4"] = 3.141592
    attrs["a5"] = [1, 2, 3]
    attrs.set("a6", 'abrakadabra')
    attrs["a7"] = ['abra', 'kadabra']
    assert len(attrs) == 7
    assert attrs.size() == 7

    # Test "__iter__"
    it = iter(attrs)
    # Test first item
    n = next(it)
    assert isinstance(n, HashAttributesNode)
    assert n.getKey() == 'a1'
    assert n.getValue() == 12
    assert n.getType() == Types.INT32
    n = next(it)
    assert n.getKey() == 'a2'
    n = next(it)
    assert n.getKey() == 'a3'
    n = next(it)
    assert n.getKey() == 'a4'
    n = next(it)
    assert n.getKey() == 'a5'
    n = next(it)
    assert n.getKey() == 'a6'
    assert n.getValue() == 'abrakadabra'
    assert n.getType() == Types.STRING
    n = next(it)
    assert n.getKey() == 'a7'
    assert n.getValue() == ['abra', 'kadabra']
    assert n.getType() == Types.VECTOR_STRING
    # Test end of iteration: "StopIteration"
    with pytest.raises(StopIteration):
        n = next(it)

    attrs = h.getAttributes("a.b.c")

    # We do not need to test `getAs` here for each possible source and
    # target type since `getAs` uses the same conversion method as for
    # 'getValueAs'
    #
    # Test "getAs"
    assert attrs.getAs("a2", "BOOL") is True
    assert attrs.getAs("a2", Types.VECTOR_BOOL) == [True]
    assert attrs.getAs("a2", "VECTOR_UINT64") == [1]
    assert attrs.getAs("a2", "FLOAT") == 1.0
    assert attrs.getAs("a2", "COMPLEX_FLOAT") == (1+0j)
    assert attrs.getAs("a2", "VECTOR_COMPLEX_DOUBLE") == [(1+0j)]

    assert attrs.getAs("a1", Types.UINT32) == 12
    assert attrs.getAs("a1", "STRING") == '12'
