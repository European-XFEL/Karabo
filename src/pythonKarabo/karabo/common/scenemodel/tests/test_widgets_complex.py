
from .. import api
from .utils import (assert_base_traits, base_widget_traits,
                    single_model_round_trip)

TABLE_SCHEMA = (
    ":&lt;root KRB_Artificial=&quot;KRB_STRING:&quot;"
    " KRB_Type=&quot;HASH&quot;&gt;&lt;a KRB_Type=&quot;INT32&quot;"
    " accessMode=&quot;KRB_INT32:1&quot; assignment=&quot;KRB_INT32:0&quot;"
    " description=&quot;KRB_STRING:First Column&quot; displayedName=&quot;"
    "KRB_STRING:A&quot; leafType=&quot;KRB_INT32:0&quot; nodeType=&quot;"
    "KRB_INT32:0&quot; requiredAccessLevel=&quot;KRB_INT32:1&quot;"
    " valueType=&quot;KRB_STRING:STRING&quot;"
    "&gt;0&lt;/a&gt;&lt;b KRB_Type=&quot;INT32&quot; accessMode=&quot;"
    "KRB_INT32:1&quot; assignment=&quot;KRB_INT32:0&quot; description=&quot;"
    "KRB_STRING:Second Column&quot; displayedName=&quot;KRB_STRING:B&quot;"
    " leafType=&quot;KRB_INT32:0&quot; nodeType=&quot;KRB_INT32:0&quot;"
    " requiredAccessLevel=&quot;KRB_INT32:1&quot; valueType=&quot;"
    "KRB_STRING:BOOL&quot;&gt;0&lt;/b&gt;&lt;/root&gt;"
)

UBUNTU_FONT_SPEC = 'Ubuntu,48,-1,5,63,0,0,0,0,0'


def test_device_scene_link_model():
    traits = base_widget_traits()
    traits['target'] = 'scene1'
    traits['target_window'] = api.SceneTargetWindow.Dialog
    traits['text'] = 'foo'
    traits['font'] = UBUNTU_FONT_SPEC
    traits['foreground'] = '#000000'
    traits['background'] = '#ffffff'
    traits['frame_width'] = 0
    model = api.DeviceSceneLinkModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)

    assert read_model.text == 'foo'
    assert read_model.font == UBUNTU_FONT_SPEC
    assert read_model.foreground == '#000000'
    assert read_model.background == '#ffffff'
    assert read_model.frame_width == 0
    assert read_model.target == 'scene1'
    assert read_model.target_window == api.SceneTargetWindow.Dialog
    assert model.parent_component == 'DisplayComponent'


def test_doubleline_edit():
    traits = base_widget_traits()
    traits['decimals'] = 5
    model = api.DoubleLineEditModel(**traits)
    assert model.parent_component == 'EditableApplyLaterComponent'
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.decimals == 5
    assert read_model.parent_component == model.parent_component


def test_color_bool_widget():
    traits = base_widget_traits()
    traits['invert'] = True
    model = api.ColorBoolModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.invert


def test_display_progress_bar_widget():
    traits = base_widget_traits()
    traits['is_vertical'] = True
    model = api.DisplayProgressBarModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.is_vertical


def test_display_state_color_widget():
    traits = base_widget_traits()
    traits['show_string'] = True
    model = api.DisplayStateColorModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.show_string


def test_evaluator_widget():
    traits = base_widget_traits()
    traits['expression'] = 'x'
    model = api.EvaluatorModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.expression == 'x'


def test_float_spinbox_widget():
    traits = base_widget_traits()
    traits['step'] = 1.5
    model = api.FloatSpinBoxModel(**traits)
    assert model.parent_component == 'EditableApplyLaterComponent'
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.step == 1.5
    assert read_model.parent_component == model.parent_component


def test_monitor_widget():
    traits = base_widget_traits()
    traits['filename'] = 'foo.log'
    traits['interval'] = 1.5
    model = api.MonitorModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.filename == 'foo.log'
    assert read_model.interval == 1.5


def test_single_bit_widget():
    traits = base_widget_traits()
    traits['invert'] = True
    traits['bit'] = 42
    model = api.SingleBitModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.invert
    assert read_model.bit == 42


def test_table_element_widget():
    for klass_name in ('DisplayTableElement', 'EditableTableElement'):
        traits = base_widget_traits()
        traits['klass'] = klass_name
        # XXX: What does a schema look like?
        traits['column_schema'] = TABLE_SCHEMA
        model = api.TableElementModel(**traits)
        read_model = single_model_round_trip(model)
        assert_base_traits(read_model)
        assert read_model.klass == klass_name
        assert read_model.column_schema == TABLE_SCHEMA
