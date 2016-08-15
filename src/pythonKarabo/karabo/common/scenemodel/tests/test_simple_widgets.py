from ..simple_widgets import LabelModel, SceneLinkModel, WorkflowItemModel
from .utils import single_model_round_trip

UBUNTU_FONT_SPEC = 'Ubuntu,48,-1,5,63,0,0,0,0,0'


def _assert_base_traits(model):
    traits = _base_widget_traits()
    for name, value in traits.items():
        msg = "{} has the wrong value!".format(name)
        assert getattr(model, name) == value, msg


def _base_widget_traits():
    return {'x': 0, 'y': 0, 'height': 100, 'width': 100}


def test_label_model():
    traits = _base_widget_traits()
    traits['text'] = 'foo'
    traits['font'] = UBUNTU_FONT_SPEC
    traits['foreground'] = '#000000'
    traits['background'] = '#ffffff'
    traits['frame_width'] = 0
    model = LabelModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_base_traits(read_model)
    assert read_model.text == 'foo'
    assert read_model.font == UBUNTU_FONT_SPEC
    assert read_model.foreground == '#000000'
    assert read_model.background == '#ffffff'
    assert read_model.frame_width == 0


def test_scene_link_model():
    traits = _base_widget_traits()
    traits['target'] = 'other.svg'
    model = SceneLinkModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_base_traits(read_model)
    assert read_model.target == 'other.svg'


def test_workflowitem_model():
    for klass_name in ('WorkflowItem', 'WorkflowGroupItem'):
        traits = _base_widget_traits()
        traits['device_id'] = 'bar'
        traits['font'] = UBUNTU_FONT_SPEC
        traits['klass'] = klass_name
        model = WorkflowItemModel(**traits)
        read_model = single_model_round_trip(model)
        _assert_base_traits(read_model)
        assert read_model.device_id == 'bar'
        assert read_model.font == UBUNTU_FONT_SPEC
        assert read_model.klass == klass_name
