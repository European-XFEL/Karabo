from qtpy.QtGui import QBrush

from karabo.common.const import (
    KARABO_SCHEMA_ACCESS_MODE, KARABO_SCHEMA_DISPLAY_TYPE)
from karabo.native import AccessMode
from karabogui.binding.api import BoolBinding, StringBinding

from ..utils import (
    create_brushes, get_button_attributes, has_confirmation,
    is_state_display_type, is_writable_string, list2string, parse_table_link,
    string2list)


def test_display_type_state():
    string = StringBinding(attributes={KARABO_SCHEMA_DISPLAY_TYPE: "State"})
    assert is_state_display_type(string)
    string = StringBinding(attributes={KARABO_SCHEMA_DISPLAY_TYPE: "Alarm"})
    assert not is_state_display_type(string)


def test_is_writable_string():
    string = StringBinding(
        attributes={KARABO_SCHEMA_ACCESS_MODE: AccessMode.RECONFIGURABLE})
    assert is_writable_string(string)

    string = StringBinding(
        attributes={KARABO_SCHEMA_ACCESS_MODE: AccessMode.READONLY})
    assert not is_writable_string(string)

    boolean = BoolBinding(
        attributes={KARABO_SCHEMA_ACCESS_MODE: AccessMode.RECONFIGURABLE})
    assert not is_writable_string(boolean)


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
