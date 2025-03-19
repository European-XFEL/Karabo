# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from copy import deepcopy

import numpy as np
from qtpy.QtGui import QBrush

from karabo.common.const import (
    KARABO_SCHEMA_ACCESS_MODE, KARABO_SCHEMA_DISPLAY_TYPE)
from karabo.native import AccessMode, Hash
from karabogui.binding.api import BoolBinding, StringBinding
from karabogui.controllers.table.utils import (
    create_brushes, get_button_attributes, has_confirmation,
    is_state_display_type, is_writable_binding, list2string, parse_table_link,
    quick_table_copy, string2list)


def test_displayType_state():
    string = StringBinding(attributes={KARABO_SCHEMA_DISPLAY_TYPE: "State"})
    assert is_state_display_type(string)
    string = StringBinding(attributes={KARABO_SCHEMA_DISPLAY_TYPE: "Alarm"})
    assert not is_state_display_type(string)


def test_is_writable_string():
    string = StringBinding(
        attributes={KARABO_SCHEMA_ACCESS_MODE: AccessMode.RECONFIGURABLE})
    assert is_writable_binding(string)
    # Test that is works also by specificing a string binding
    assert is_writable_binding(string, StringBinding)

    string = StringBinding(
        attributes={KARABO_SCHEMA_ACCESS_MODE: AccessMode.READONLY})
    assert not is_writable_binding(string)

    boolean = BoolBinding(
        attributes={KARABO_SCHEMA_ACCESS_MODE: AccessMode.RECONFIGURABLE})
    assert is_writable_binding(boolean)
    assert not is_writable_binding(boolean, StringBinding)


def test_convert_string_list():
    value = ""
    assert string2list(value) == []
    value = "2"
    assert string2list(value) == ["2"]
    value = "2,2,3"
    assert string2list(value) == ["2", "2", "3"]
    # Only for testing. Prevented by delegates later.
    value = "2,2,    3"
    assert string2list(value) == ["2", "2", "3"]
    value = ", , ,"
    assert string2list(value) == ["", "", "", ""]
    value = ",2,"
    assert string2list(value) == ["", "2", ""]


def test_convert_list_string():
    value = None
    assert list2string(value) == ""
    value = ["2", "4.2"]
    assert list2string(value) == "2,4.2"
    value = ["2", "2", "3"]
    assert list2string(value) == "2,2,3"


def test_create_brushes():
    display_string = "TableColor|offline=red&online=green&default=blue"
    default, brushes = create_brushes(display_string)
    assert default is not None
    assert isinstance(default, QBrush)
    assert "offline" in brushes
    assert "online" in brushes
    assert isinstance(brushes["online"], QBrush)
    assert isinstance(brushes["offline"], QBrush)

    # Wonky typos, e.g. ? as uri identifier
    display_string = "TableColor?offline=red&online=green&default=blue"
    default, brushes = create_brushes(display_string)
    assert default is None
    assert not brushes

    # Wrong delimiter, no exceptions
    display_string = "TableColor|offline=red%online=green%nodefault=blue"
    default, brushes = create_brushes(display_string)
    assert default is None
    assert len(brushes) == 1
    assert "offline" in brushes

    # No default brush
    display_string = "TableColor|offline=red&online=green&nodefault=blue"
    default, brushes = create_brushes(display_string)
    assert default is None
    assert "offline" in brushes
    assert "online" in brushes
    assert "nodefault" in brushes


def test_has_confirmation():
    confirm = has_confirmation("TableColor|confirmation=1")
    assert confirm is True
    # No caseless hickups, we have a contract
    confirm = has_confirmation("TableColor|Confirmation=1")
    assert confirm is False
    confirm = has_confirmation("TableColor|confirmation=1&icon=0")
    assert confirm is True
    confirm = has_confirmation("TableColor|confirmation=0")
    assert confirm is False
    confirm = has_confirmation("TableColor|confirmation")
    assert confirm is False


def test_get_button_attributes():
    attrs = get_button_attributes("TableColor|confirmation=1")
    assert attrs.get("confirmation") == "1"
    attrs = get_button_attributes("TableSelection|Confirmation=1&")
    assert attrs.get("Confirmation") == "1"
    attrs = get_button_attributes("TableBoolButton|confirmation=1&icon=0")
    assert attrs.get("confirmation") == "1"
    assert attrs.get("icon") == "0"


def test_table_scheme_parsing():
    action, options = parse_table_link(
        "deviceScene|device_id=CAM&name=scene")
    assert action == "deviceScene"
    assert options["device_id"] == "CAM"
    assert options["name"] == "scene"

    action, options = parse_table_link("url|https://www.xfel.eu")
    assert action == "url"
    assert options == "https://www.xfel.eu"


def test_copy_hash():
    h = Hash("boolean", False, "string", "ginger", "int", 1, "float", 2.1,
             "list", [1, 2], "array", np.array([1, 2, 3]), "emptylist", [],
             "emptyarray", np.array([]))

    SIMPLES = ["string", "float", "boolean"]

    for func in (quick_table_copy, deepcopy):
        r = func(h)
        assert r.fullyEqual(h)
        hv = h["list"]
        rv = r["list"]
        assert id(hv) != id(rv)
        # Same id, but immutable objects
        assert id(hv[0]) == id(rv[0])
        hv = h["array"]
        rv = r["array"]
        assert id(hv) != id(rv)
        hv = h["emptylist"]
        rv = r["emptylist"]
        assert id(hv) != id(rv)
        hv = h["emptyarray"]
        rv = r["emptyarray"]
        assert id(hv) != id(rv)

        for simple in SIMPLES:
            hv = h[simple]
            rv = r[simple]
            assert id(hv) == id(rv)
