from nose.tools import assert_raises

from ..api import (BitfieldModel, CheckBoxModel, ChoiceElementModel,
                   ComboBoxModel, DirectoryModel, DisplayCommandModel,
                   DisplayLabelModel, DisplayPlotModel, DoubleLineEditModel,
                   EditableListElementModel, EditableListModel,
                   EditableSpinBoxModel, FileInModel, FileOutModel,
                   HexadecimalModel, IntLineEditModel, KnobModel, LabelModel,
                   LineEditModel, PopUpModel, SceneLinkModel,
                   SceneWriterException, SliderModel, WorkflowItemModel,
                   XYPlotModel)
from .utils import (assert_base_traits, base_widget_traits,
                    single_model_round_trip)

UBUNTU_FONT_SPEC = 'Ubuntu,48,-1,5,63,0,0,0,0,0'


def _assert_geometry_traits(model):
    traits = _geometry_traits()
    for name, value in traits.items():
        msg = "{} has the wrong value!".format(name)
        assert getattr(model, name) == value, msg


def _check_display_editable_widget(klass):
    extras = (
        ('DisplayComponent', 'Display'), ('EditableComponent', 'Editable')
    )
    suffix = klass.__name__[:-len('Model')]
    for parent, prefix in extras:
        klass_name = prefix + suffix
        traits = base_widget_traits(parent=parent)
        traits['klass'] = klass_name
        model = klass(**traits)
        read_model = single_model_round_trip(model)
        assert_base_traits(read_model)
        assert read_model.klass == klass_name


def _check_empty_widget(klass):
    traits = base_widget_traits(parent='DisplayComponent')
    model = klass(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)


def _geometry_traits():
    return {'x': 0, 'y': 0, 'height': 100, 'width': 100}


def test_all_empty_widgets():
    model_classes = (
        BitfieldModel, DisplayCommandModel, DisplayLabelModel,
        DisplayPlotModel, DoubleLineEditModel, EditableListModel,
        EditableListElementModel, EditableSpinBoxModel, HexadecimalModel,
        IntLineEditModel, KnobModel, SliderModel, XYPlotModel, PopUpModel
    )
    for klass in model_classes:
        yield _check_empty_widget, klass


def test_display_editable_widgets():
    model_classes = (CheckBoxModel, ChoiceElementModel, ComboBoxModel,
                     DirectoryModel, FileInModel, FileOutModel, LineEditModel)
    for klass in model_classes:
        yield _check_display_editable_widget, klass


def test_missing_parent_component():
    traits = base_widget_traits()
    model = BitfieldModel(**traits)
    assert_raises(SceneWriterException, single_model_round_trip, model)


def test_label_model():
    traits = _geometry_traits()
    traits['text'] = 'foo'
    traits['font'] = UBUNTU_FONT_SPEC
    traits['foreground'] = '#000000'
    traits['background'] = '#ffffff'
    traits['frame_width'] = 0
    model = LabelModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.text == 'foo'
    assert read_model.font == UBUNTU_FONT_SPEC
    assert read_model.foreground == '#000000'
    assert read_model.background == '#ffffff'
    assert read_model.frame_width == 0


def test_scene_link_model():
    traits = _geometry_traits()
    traits['target'] = 'other.svg'
    model = SceneLinkModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.target == 'other.svg'


def test_workflowitem_model():
    for klass_name in ('WorkflowItem', 'WorkflowGroupItem'):
        traits = _geometry_traits()
        traits['device_id'] = 'bar'
        traits['font'] = UBUNTU_FONT_SPEC
        traits['klass'] = klass_name
        model = WorkflowItemModel(**traits)
        read_model = single_model_round_trip(model)
        _assert_geometry_traits(read_model)
        assert read_model.device_id == 'bar'
        assert read_model.font == UBUNTU_FONT_SPEC
        assert read_model.klass == klass_name
