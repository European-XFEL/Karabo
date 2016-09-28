from ..widgets.icon import (
    IconData, DigitIconsModel, DisplayIconsetModel, SelectionIconsModel,
    TextIconsModel)
from .utils import (assert_base_traits, base_widget_traits,
                    single_model_round_trip)


def _check_icon_widget(klass):
    traits = base_widget_traits(parent='DisplayComponent')
    icon = IconData(image='blah.svg')
    if klass is DigitIconsModel:
        icon.equal = True
        icon.value = '14'
    traits['values'] = [icon]
    model = klass(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert len(read_model.values) == 1
    assert read_model.values[0].image == 'blah.svg'
    if klass is DigitIconsModel:
        assert read_model.values[0].equal is True
        assert read_model.values[0].value == '14'


def test_display_iconset_widget():
    traits = base_widget_traits(parent='DisplayComponent')
    traits['image'] = 'blah.svg'
    model = DisplayIconsetModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.image == 'blah.svg'


def test_icon_widgets():
    model_classes = (DigitIconsModel, SelectionIconsModel, TextIconsModel)
    for klass in model_classes:
        yield _check_icon_widget, klass
