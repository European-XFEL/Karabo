from xml.etree.ElementTree import Element

from nose.tools import assert_raises

from ..exceptions import SceneWriterException
from ..model import SceneModel
from ..io import read_scene, write_scene
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
    SliderModel, TableElementModel, VacuumWidgetModel, XYPlotModel
)
from .utils import temp_file


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
    read_model = _perform_data_round_trip(model)
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
        read_model = _perform_data_round_trip(model)
        _assert_base_traits(read_model)
        assert read_model.klass == klass_name


def _check_icon_widget(klass):
    traits = _base_widget_traits(parent='DisplayComponent')
    icon = IconData(image='blah.svg')
    if klass is DigitIconsModel:
        icon.equal = False
        icon.value = '14'
    traits['values'] = [icon]
    model = klass(**traits)
    read_model = _perform_data_round_trip(model)
    _assert_base_traits(read_model)
    assert len(read_model.values) == 1
    assert read_model.values[0].image == 'blah.svg'
    if klass is DigitIconsModel:
        assert read_model.values[0].equal is False
        assert read_model.values[0].value == '14'


def _perform_data_round_trip(model):
    scene = SceneModel(children=[model])
    xml = write_scene(scene)
    with temp_file(xml.decode('utf-8')) as fn:
        rt_scene = read_scene(fn)

    return rt_scene.children[0]


def test_all_empty_widgets():
    model_classes = (
        BitfieldModel, DisplayAlignedImageModel, DisplayCommandModel,
        DisplayIconsetModel, DisplayImageModel, DisplayImageElementModel,
        DisplayPlotModel, DoubleLineEditModel, EditableListModel,
        EditableListElementModel, EditableSpinBoxModel, HexadecimalModel,
        IntLineEditModel, KnobModel, SliderModel, XYPlotModel
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
    assert_raises(SceneWriterException, _perform_data_round_trip, model)
