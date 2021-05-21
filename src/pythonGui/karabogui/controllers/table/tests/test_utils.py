from karabo.common.const import KARABO_SCHEMA_DISPLAY_TYPE
from karabogui.binding.api import StringBinding

from ..utils import is_state_display_type, list2string, string2list


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
