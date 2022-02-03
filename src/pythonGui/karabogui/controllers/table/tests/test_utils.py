from qtpy.QtGui import QBrush

from karabo.common.const import KARABO_SCHEMA_DISPLAY_TYPE
from karabogui.binding.api import StringBinding

from ..utils import (
    create_brushes, is_state_display_type, list2string, string2list)


def test_display_type_state():
    string = StringBinding(attributes={KARABO_SCHEMA_DISPLAY_TYPE: "State"})
    assert is_state_display_type(string)
    string = StringBinding(attributes={KARABO_SCHEMA_DISPLAY_TYPE: "Alarm"})
    assert not is_state_display_type(string)


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
