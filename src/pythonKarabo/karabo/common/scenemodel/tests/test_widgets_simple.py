from nose.tools import assert_raises

from traits.api import TraitError

from .. import api
from .utils import (assert_base_traits, base_widget_traits,
                    single_model_round_trip)

UBUNTU_FONT_SPEC = 'Ubuntu,48,-1,5,63,0,0,0,0,0'


def _assert_geometry_traits(model):
    traits = _geometry_traits()
    for name, value in traits.items():
        msg = "{} has the wrong value!".format(name)
        assert getattr(model, name) == value, msg


def _check_display_editable_widget(klass):
    suffix = klass.__name__[:-len('Model')]
    for prefix in ('Display', 'Editable'):
        traits = base_widget_traits()
        klass_name = prefix + suffix
        traits['klass'] = klass_name
        model = klass(**traits)
        read_model = single_model_round_trip(model)
        assert_base_traits(read_model)
        assert read_model.klass == klass_name
        assert read_model.parent_component.startswith(prefix)


def _check_empty_widget(klass):
    traits = base_widget_traits()
    model = klass(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)


def _check_editable_empty_widget(klass):
    traits = base_widget_traits()
    model = klass(**traits)
    assert model.parent_component == 'EditableApplyLaterComponent'
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.parent_component == model.parent_component


def _geometry_traits():
    return {'x': 0, 'y': 0, 'height': 100, 'width': 100}


def test_all_empty_widgets():
    model_classes = (
        api.DisplayLabelModel, api.PopUpModel, api.WidgetNodeModel)
    for klass in model_classes:
        yield _check_empty_widget, klass


def test_display_editable_widgets():
    model_classes = (
        api.CheckBoxModel, api.ChoiceElementModel, api.ComboBoxModel,
        api.DirectoryModel, api.FileInModel, api.FileOutModel,
        api.LineEditModel
    )
    for klass in model_classes:
        yield _check_display_editable_widget, klass


def test_editable_simple_model():
    model_classes = (
        api.BitfieldModel, api.EditableListModel, api.EditableListElementModel,
        api.EditableSpinBoxModel, api.HexadecimalModel, api.IntLineEditModel,
        api.RunConfiguratorModel, api.SliderModel
    )
    for klass in model_classes:
        yield _check_editable_empty_widget, klass


def test_missing_parent_component():
    traits = base_widget_traits()
    traits['parent_component'] = ''  # explicitly empty!
    model = api.BitfieldModel(**traits)
    assert_raises(api.SceneWriterException, single_model_round_trip, model)


def test_displaylabel_model():
    # Check default model
    default_model = api.DisplayLabelModel()
    assert default_model.font_size == api.SCENE_FONT_SIZE
    assert default_model.font_weight == api.SCENE_FONT_WEIGHT

    # Check valid input
    input_size = 7
    input_weight = 'bold'
    valid_model = api.DisplayLabelModel(font_size=input_size,
                                        font_weight=input_weight)
    assert valid_model.font_size == input_size
    assert valid_model.font_weight == input_weight

    # Check invalid input
    assert_raises(TraitError, api.DisplayLabelModel, font_size=1)
    assert_raises(TraitError, api.DisplayLabelModel, font_weight='foo')


def test_label_model():
    traits = _geometry_traits()
    traits['text'] = 'foo'
    traits['font'] = UBUNTU_FONT_SPEC
    traits['foreground'] = '#000000'
    traits['background'] = '#ffffff'
    traits['frame_width'] = 0
    model = api.LabelModel(**traits)
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
    traits['target_window'] = api.SceneTargetWindow.Dialog
    model = api.SceneLinkModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.target == 'other.svg'
    assert read_model.target_window == api.SceneTargetWindow.Dialog


def test_sticker_model():
    traits = _geometry_traits()
    traits['text'] = 'foo'
    traits['font'] = UBUNTU_FONT_SPEC
    traits['foreground'] = '#000000'
    traits['background'] = '#ffffff'
    model = api.StickerModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.text == 'foo'
    assert read_model.font == UBUNTU_FONT_SPEC
    assert read_model.foreground == '#000000'
    assert read_model.background == '#ffffff'


def test_tickslider():
    traits = _geometry_traits()
    traits['ticks'] = 500
    traits['show_value'] = False
    model = api.TickSliderModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.ticks == 500
    assert read_model.show_value is False


def test_timelabel():
    traits = _geometry_traits()
    traits['time_format'] = '%H:%M:%S'
    model = api.DisplayTimeModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.time_format == '%H:%M:%S'


def test_web_link_model():
    traits = _geometry_traits()
    traits['target'] = 'www.xfel.eu'
    traits['text'] = 'www.karabo.eu'
    traits['font'] = UBUNTU_FONT_SPEC
    traits['foreground'] = '#000000'
    traits['background'] = '#ffffff'
    traits['frame_width'] = 1
    model = api.WebLinkModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.target == 'www.xfel.eu'
    assert read_model.text == 'www.karabo.eu'
    assert read_model.font == UBUNTU_FONT_SPEC
    assert read_model.foreground == '#000000'
    assert read_model.background == '#ffffff'
    assert read_model.frame_width == 1
