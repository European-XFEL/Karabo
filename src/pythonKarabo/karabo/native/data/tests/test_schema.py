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
from copy import deepcopy

import pytest

from ..enums import (
    AccessLevel, AccessMode, ArchivePolicy, Assignment, MetricPrefix, NodeType,
    Unit)
from ..hash import Hash, is_equal
from ..schema import Schema


@pytest.fixture
def schema():
    h = Hash("node", Hash("b", None), "a", None,
             "c", None, "d", None, "e", None)

    h["a", "nodeType"] = NodeType.Leaf.value
    h["a", "valueType"] = "INT32"
    h["a", "description"] = "a's description"
    h["a", "allowedStates"] = ["INIT", "UNKNOWN"]
    h["a", "unitSymbol"] = "A"
    h["a", "defaultValue"] = 22.5
    h["a", "metricPrefixSymbol"] = "m"
    h["a", "accessMode"] = AccessMode.RECONFIGURABLE.value
    h["a", "requiredAccessLevel"] = AccessLevel.OPERATOR.value
    h["a", "assignment"] = Assignment.INTERNAL.value
    h["a", "archivePolicy"] = ArchivePolicy.EVERY_EVENT.value
    h["a", "alias"] = "Karabo"
    h["node", "nodeType"] = NodeType.Node.value
    h["node.b", "nodeType"] = NodeType.Leaf.value
    h["node.b", "valueType"] = "STRING"
    h["node.b", "description"] = "b's description"
    h["node.b", "allowedStates"] = ["CHANGING", "UNKNOWN"]
    h["node.b", "unitSymbol"] = "A"
    h["node.b", "defaultValue"] = 22.5
    h["node.b", "tags"] = ["mpod"]
    h["c", "nodeType"] = NodeType.Leaf.value
    h["c", "valueType"] = "INT32"
    h["c", "description"] = "c's description"
    h["c", "allowedStates"] = ["INIT", "UNKNOWN"]
    h["c", "unitSymbol"] = "A"
    h["c", "defaultValue"] = 17.5
    h["c", "tags"] = ["mpod"]
    h["d", "nodeType"] = NodeType.Leaf.value
    h["d", "valueType"] = "INT32"
    h["d", "description"] = "d's description"
    h["d", "allowedStates"] = ["INIT", "UNKNOWN"]
    h["d", "unitSymbol"] = "A"
    h["d", "defaultValue"] = 27.5
    h["d", "tags"] = ["mpod", "plc"]

    h["e", "nodeType"] = NodeType.Leaf.value
    h["e", "valueType"] = "VECTOR_STRING"
    h["e", "description"] = "e's description"
    h["e", "unitSymbol"] = ""
    h["e", "defaultValue"] = ["One", "Two", "Three"]

    return Schema("XFEL", hash=h)


def test_getKeyFromAlias(schema):
    key = schema.getKeyFromAlias("Karabo")
    assert key == "a"


def test_keyHasAlias(schema):
    assert schema.keyHasAlias("a") is True
    assert schema.keyHasAlias("node.b") is False


def test_getAliasAsString(schema):
    alias = schema.getAliasAsString("a")
    assert alias == "Karabo"


def test_filterByTags(schema):
    h = schema.filterByTags("plc")
    path = h.paths()
    assert "d" in path

    h = schema.filterByTags("mpod")
    path = h.paths()
    assert "node.b" in path
    assert "c" in path
    assert "d" in path


def test_schema_equal(schema):
    h = schema.hash
    assert h.fullyEqual(schema.hash)

    s = h.deepcopy()
    assert h.fullyEqual(s)
    s.erase("d")
    assert not h.fullyEqual(s)
    assert not s.fullyEqual(h)

    s = h.deepcopy()
    assert h.fullyEqual(s)
    s["e", "defaultValue"] = ["Three"]
    assert not h.fullyEqual(s)

    s = h.deepcopy()
    assert h.fullyEqual(s)
    s["d", "tags"] = ["mpod"]
    assert not h.fullyEqual(s)

    deep = deepcopy(h)
    assert deep["node.b", ...] != {}
    assert h["node.b", ...] != {}
    assert h.fullyEqual(deep)

    mutable = [1, 2, 3, 4]
    h["mutable"] = mutable
    s = h.deepcopy()
    deep = deepcopy(h)
    assert h.fullyEqual(s)
    assert h["mutable"] == mutable
    assert s["mutable"] == mutable
    assert deep["mutable"] == mutable
    mutable.append(6)
    assert h["mutable"] == mutable
    assert s["mutable"] != mutable
    assert deep["mutable"] != mutable
    assert h["mutable"] is mutable
    assert s["mutable"] is not mutable
    assert deep["mutable"] is not mutable
    assert deep.fullyEqual(s)

    h = Hash("value", "None")
    assert is_equal(Schema(name="foo", hash=h), Schema(name="foo", hash=h))
    assert not is_equal(Schema(name="bar", hash=h), Schema(name="foo", hash=h))
    assert not is_equal(Schema(name="foo", hash=h),
                        Schema(name="foo", hash=Hash()))
    assert is_equal(Schema(name="test", hash=h), Schema(name="test", hash=h))


def test_helpers(schema):
    attrs = schema.hash["a", ...]
    assert AccessLevel.fromAttributes(attrs) == AccessLevel.OPERATOR
    assert AccessMode.fromAttributes(attrs) == AccessMode.RECONFIGURABLE
    assert ArchivePolicy.fromAttributes(attrs) == ArchivePolicy.EVERY_EVENT
    assert Assignment.fromAttributes(attrs) == Assignment.INTERNAL
    assert MetricPrefix.fromAttributes(attrs) == MetricPrefix.MILLI
    assert Unit.fromAttributes(attrs) == Unit.AMPERE

    attrs = schema.hash["c", ...]
    assert AccessLevel.fromAttributes(attrs) is None
    assert AccessMode.fromAttributes(attrs) is None
    assert ArchivePolicy.fromAttributes(attrs) is None
    assert Assignment.fromAttributes(attrs) is None
    assert MetricPrefix.fromAttributes(attrs) is None
