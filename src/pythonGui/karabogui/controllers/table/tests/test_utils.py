from karabo.common.const import KARABO_SCHEMA_DISPLAY_TYPE
from karabogui.binding.api import StringBinding

from ..utils import convert_string_list, is_state_display_type


def test_display_type_state():
    string = StringBinding(attributes={KARABO_SCHEMA_DISPLAY_TYPE: "State"})
    assert is_state_display_type(string)
    string = StringBinding(attributes={KARABO_SCHEMA_DISPLAY_TYPE: "Alarm"})
    assert not is_state_display_type(string)


def test_convert_string_list():
    value = ""
    assert convert_string_list(value) == []
    value = "2"
    assert convert_string_list(value) == ["2"]
    value = "2,2,3"
    assert convert_string_list(value) == ["2", "2", "3"]
    # Only for testing. Prevented by delegates later.
    value = "2,2,    3"
    assert convert_string_list(value) == ["2", "2", "3"]
    value = ", , ,"
    assert convert_string_list(value) == ["", "", "", ""]
