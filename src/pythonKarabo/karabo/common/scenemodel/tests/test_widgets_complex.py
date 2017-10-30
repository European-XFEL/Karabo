from ..api import (
    ColorBoolModel, DisplayProgressBarModel, DisplayStateColorModel,
    DoubleLineEditModel, EvaluatorModel, FloatSpinBoxModel, MonitorModel,
    SingleBitModel, TableElementModel,
)
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


def test_doubleline_edit():
    traits = base_widget_traits()
    traits['decimals'] = 5
    model = DoubleLineEditModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.decimals == 5


def test_color_bool_widget():
    traits = base_widget_traits()
    traits['invert'] = True
    model = ColorBoolModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.invert


def test_display_progress_bar_widget():
    traits = base_widget_traits()
    traits['is_vertical'] = True
    model = DisplayProgressBarModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.is_vertical


def test_display_state_color_widget():
    traits = base_widget_traits()
    traits['text'] = 'foo'
    model = DisplayStateColorModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.text == 'foo'


def test_evaluator_widget():
    traits = base_widget_traits()
    traits['expression'] = 'x'
    model = EvaluatorModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.expression == 'x'


def test_float_spinbox_widget():
    traits = base_widget_traits()
    traits['step'] = 1.5
    model = FloatSpinBoxModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.step == 1.5


def test_monitor_widget():
    traits = base_widget_traits()
    traits['filename'] = 'foo.log'
    traits['interval'] = 1.5
    model = MonitorModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.filename == 'foo.log'
    assert read_model.interval == 1.5


def test_single_bit_widget():
    traits = base_widget_traits()
    traits['invert'] = True
    traits['bit'] = 42
    model = SingleBitModel(**traits)
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
        model = TableElementModel(**traits)
        read_model = single_model_round_trip(model)
        assert_base_traits(read_model)
        assert read_model.klass == klass_name
        assert read_model.column_schema == TABLE_SCHEMA
