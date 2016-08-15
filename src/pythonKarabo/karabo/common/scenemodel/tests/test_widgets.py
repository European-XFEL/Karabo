from nose.tools import assert_raises

from ..exceptions import SceneWriterException
from ..widgets import (
    BitfieldModel, CheckBoxModel, ChoiceElementModel, ComboBoxModel,
    DirectoryModel, DisplayAlignedImageModel, DisplayCommandModel,
    DisplayIconsetModel, DisplayImageModel, DisplayImageElementModel,
    DisplayLabelModel, DisplayPlotModel, DisplayStateColorModel,
    DoubleLineEditModel, EditableListModel, EditableListElementModel,
    EditableSpinBoxModel, EvaluatorModel, FileInModel, FileOutModel,
    FloatSpinBoxModel, HexadecimalModel, IconData, DigitIconsModel,
    SelectionIconsModel, TextIconsModel, IntLineEditModel, KnobModel,
    LineEditModel, PlotCurveModel, LinePlotModel, MonitorModel, SingleBitModel,
    SliderModel, TableElementModel, VacuumWidgetModel, XYPlotModel,
    VACUUM_WIDGETS
)
from .utils import single_model_round_trip

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


def _assert_base_traits(model):
    assert model.x == 0
    assert model.y == 0
    assert model.width == 100
    assert model.height == 100
    assert model.keys == ['device_id.prop']


def _base_widget_traits(parent=None):
    traits = {
        'keys': ['device_id.prop'],
        'x': 0, 'y': 0, 'height': 100, 'width': 100
    }
    if parent is not None:
        traits['parent_component'] = parent
    return traits


def _check_empty_widget(klass):
    traits = _base_widget_traits(parent='DisplayComponent')
    model = klass(**traits)
    read_model = single_model_round_trip(model)
    _assert_base_traits(read_model)


def _check_display_editable_widget(klass):
    extras = (
        ('DisplayComponent', 'Display'), ('EditableComponent', 'Editable')
    )
    suffix = klass.__name__[:-len('Model')]
    for parent, prefix in extras:
        klass_name = prefix + suffix
        traits = _base_widget_traits(parent=parent)
        traits['klass'] = klass_name
        model = klass(**traits)
        read_model = single_model_round_trip(model)
        _assert_base_traits(read_model)
        assert read_model.klass == klass_name


def _check_icon_widget(klass):
    traits = _base_widget_traits(parent='DisplayComponent')
    icon = IconData(image='blah.svg')
    if klass is DigitIconsModel:
        icon.equal = True
        icon.value = '14'
    traits['values'] = [icon]
    model = klass(**traits)
    read_model = single_model_round_trip(model)
    _assert_base_traits(read_model)
    assert len(read_model.values) == 1
    assert read_model.values[0].image == 'blah.svg'
    if klass is DigitIconsModel:
        assert read_model.values[0].equal is True
        assert read_model.values[0].value == '14'


def test_all_empty_widgets():
    model_classes = (
        BitfieldModel, DisplayAlignedImageModel, DisplayCommandModel,
        DisplayIconsetModel, DisplayImageModel, DisplayImageElementModel,
        DisplayLabelModel, DisplayPlotModel, DoubleLineEditModel,
        EditableListModel, EditableListElementModel, EditableSpinBoxModel,
        HexadecimalModel, IntLineEditModel, KnobModel, SliderModel, XYPlotModel
    )
    for klass in model_classes:
        yield _check_empty_widget, klass


def test_display_editable_widgets():
    model_classes = (CheckBoxModel, ChoiceElementModel, ComboBoxModel,
                     DirectoryModel, FileInModel, FileOutModel, LineEditModel)
    for klass in model_classes:
        yield _check_display_editable_widget, klass


def test_icon_widgets():
    model_classes = (DigitIconsModel, SelectionIconsModel, TextIconsModel)
    for klass in model_classes:
        yield _check_icon_widget, klass


def test_missing_parent_component():
    traits = _base_widget_traits()
    model = BitfieldModel(**traits)
    assert_raises(SceneWriterException, single_model_round_trip, model)


def test_display_state_color_widget():
    traits = _base_widget_traits(parent='DisplayComponent')
    traits['text'] = 'foo'
    traits['colors'] = {'red': (255, 0, 0, 255)}
    model = DisplayStateColorModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_base_traits(read_model)
    assert read_model.text == 'foo'
    assert len(read_model.colors) == 1
    assert read_model.colors['red'] == (255, 0, 0, 255)


def test_evaluator_widget():
    traits = _base_widget_traits(parent='DisplayComponent')
    traits['expression'] = 'x'
    model = EvaluatorModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_base_traits(read_model)
    assert read_model.expression == 'x'


def test_float_spinbox_widget():
    traits = _base_widget_traits(parent='DisplayComponent')
    traits['step'] = 1.5
    model = FloatSpinBoxModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_base_traits(read_model)
    assert read_model.step == 1.5


def test_line_plot_widget():
    curve = PlotCurveModel(device='dev1', path='prop', curve_object_data='00')
    for name in ('DisplayTrendline', 'XYVector'):
        traits = _base_widget_traits(parent='DisplayComponent')
        traits['klass'] = name
        traits['boxes'] = [curve]
        model = LinePlotModel(**traits)
        read_model = single_model_round_trip(model)
        _assert_base_traits(read_model)
        assert read_model.klass == name
        assert len(read_model.boxes) == 1
        assert read_model.boxes[0].device == 'dev1'
        assert read_model.boxes[0].path == 'prop'
        assert read_model.boxes[0].curve_object_data == '00'


def test_line_plot_duplicate_curves():
    curve0 = PlotCurveModel(device='dev1', path='foo', curve_object_data='00')
    curve1 = PlotCurveModel(device='dev1', path='bar', curve_object_data='11')
    curve2 = PlotCurveModel(device='dev1', path='foo', curve_object_data='22')
    traits = _base_widget_traits(parent='DisplayComponent')
    model = LinePlotModel(klass='XYVector', boxes=[curve0, curve1], **traits)

    model.boxes.append(curve1)
    model.boxes.append(curve2)
    assert len(model.boxes) == 2
    assert model.boxes[0].curve_object_data == curve2.curve_object_data
    assert model.boxes[1] is curve1


def test_monitor_widget():
    traits = _base_widget_traits(parent='DisplayComponent')
    traits['filename'] = 'foo.log'
    traits['interval'] = 1.5
    model = MonitorModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_base_traits(read_model)
    assert read_model.filename == 'foo.log'
    assert read_model.interval == 1.5


def test_single_bit_widget():
    traits = _base_widget_traits(parent='DisplayComponent')
    traits['bit'] = 42
    model = SingleBitModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_base_traits(read_model)
    assert read_model.bit == 42


def test_table_element_widget():
    parts = (
        ('DisplayComponent', 'DisplayTableElement'),
        ('EditableComponent', 'EditableTableElement')
    )
    for parent, klass_name in parts:
        traits = _base_widget_traits(parent=parent)
        traits['klass'] = klass_name
        # XXX: What does a schema look like?
        traits['column_schema'] = TABLE_SCHEMA
        model = TableElementModel(**traits)
        read_model = single_model_round_trip(model)
        _assert_base_traits(read_model)
        assert read_model.klass == klass_name
        assert read_model.column_schema == TABLE_SCHEMA


def test_vacuum_widget():
    model = VacuumWidgetModel()
    for name in VACUUM_WIDGETS:
        traits = _base_widget_traits(parent='DisplayComponent')
        traits['klass'] = name
        model = VacuumWidgetModel(**traits)
        read_model = single_model_round_trip(model)
        _assert_base_traits(read_model)
        assert read_model.klass == name
