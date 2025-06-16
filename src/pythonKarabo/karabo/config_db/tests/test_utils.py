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

from karabo.config_db.utils import (
    datetime_from_str, datetime_now, datetime_to_str, hashFromBase64Bin,
    hashToBase64Bin, schemaFromBase64Bin, schemaToBase64Bin)
from karabo.native import Hash, NodeType, Schema


def test_HashConversions():
    h = Hash('a', 1, 'b', 2.0)
    encHash = hashToBase64Bin(h)
    decHash = hashFromBase64Bin(encHash)
    assert h == decHash


def test_SchemaConversions():
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
    assert sch.hash == decSch.hash


def test_datetime_tools():
    dt = datetime_now()
    ts = datetime_to_str(dt)
    assert datetime_to_str(dt) == ts
    assert datetime_from_str(ts) == dt

    with pytest.raises(ValueError):
        datetime_from_str("Throw-Date")
