from xml.etree.ElementTree import SubElement

from traits.api import Bool, Enum, Int, String

from karabo.common.scenemodel.bases import (
    BaseDisplayEditableWidget, BaseEditWidget, BaseWidgetObjectData)
from karabo.common.scenemodel.const import (
    NS_KARABO, SCENE_FONT_SIZE, SCENE_FONT_SIZES, SCENE_FONT_WEIGHT,
    SCENE_FONT_WEIGHTS, SceneTargetWindow, WIDGET_ELEMENT_TAG)
from karabo.common.scenemodel.io_utils import (
    get_numbers, set_numbers, read_base_widget_data,
    read_empty_display_editable_widget, read_font_format_data,
    write_base_widget_data, write_font_format_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)


class AnalogModel(BaseWidgetObjectData):
    """ A model for Analog Widget"""


class BitfieldModel(BaseEditWidget):
    """ A model for Bitfield"""


class WidgetNodeModel(BaseWidgetObjectData):
    """ A model for the basic widget node box"""


class CheckBoxModel(BaseDisplayEditableWidget):
    """ A model for DisplayCheckBox/EditableCheckBox
    """
    # The actual type of the widget
    klass = Enum('DisplayCheckBox', 'EditableCheckBox')


class ChoiceElementModel(BaseDisplayEditableWidget):
    """ A model for DisplayChoiceElement/EditableChoiceElement
    """
    # The actual type of the widget
    klass = Enum('DisplayChoiceElement', 'EditableChoiceElement')


class ComboBoxModel(BaseDisplayEditableWidget):
    """ A model for DisplayComboBox/EditableComboBox
    """
    # The actual type of the widget
    klass = Enum('DisplayComboBox', 'EditableComboBox')


class DirectoryModel(BaseDisplayEditableWidget):
    """ A model for DisplayDirectory/EditableDirectory
    """
    # The actual type of the widget
    klass = Enum('DisplayDirectory', 'EditableDirectory')


class DaemonManagerModel(BaseWidgetObjectData):
    """ A model for the DaemonManager Table element"""


class DisplayLabelModel(BaseWidgetObjectData):
    """ A model for DisplayLabel"""
    font_size = Enum(*SCENE_FONT_SIZES)
    font_weight = Enum(*SCENE_FONT_WEIGHTS)

    def __init__(self, font_size=SCENE_FONT_SIZE,
                 font_weight=SCENE_FONT_WEIGHT, **traits):
        # We set the default values as Enum doesn't set this straightforwardly
        super(DisplayLabelModel, self).__init__(font_size=font_size,
                                                font_weight=font_weight,
                                                **traits)


class DisplayListModel(BaseWidgetObjectData):
    """ A model for DisplayList"""


class DisplayTextLogModel(BaseWidgetObjectData):
    """ A model for DisplayTextLog"""


class EditableListModel(BaseEditWidget):
    """ A model for EditableList"""


class EditableRegexListModel(BaseEditWidget):
    """ A model for EditableRegexList"""


class EditableListElementModel(BaseEditWidget):
    """ A model for EditableListElement"""


class EditableSpinBoxModel(BaseEditWidget):
    """ A model for EditableSpinBox"""


class EditableRegexModel(BaseEditWidget):
    """ A model for RegexEdit"""


class FileInModel(BaseDisplayEditableWidget):
    """ A model for DisplayFileIn/EditableFileIn
    """
    # The actual type of the widget
    klass = Enum('DisplayFileIn', 'EditableFileIn')


class FileOutModel(BaseDisplayEditableWidget):
    """ A model for DisplayFileOut/EditableFileOut
    """
    # The actual type of the widget
    klass = Enum('DisplayFileOut', 'EditableFileOut')


class GlobalAlarmModel(BaseWidgetObjectData):
    """ A model for GlobalAlarm Widget"""


class HexadecimalModel(BaseEditWidget):
    """ A model for Hexadecimal"""


class IntLineEditModel(BaseEditWidget):
    """ A model for IntLineEdit"""


class LabelModel(BaseWidgetObjectData):
    """ A fragment of text which is shown in a scene.
    """
    # The text to be displayed
    text = String
    # A string describing the font
    font = String
    # A foreground color, CSS-style
    foreground = String
    # A background color, CSS-style
    background = String('transparent')
    # The line width of a frame around the text
    frame_width = Int(0)


class LampModel(BaseWidgetObjectData):
    """ A model for LampWidget"""


class LineEditModel(BaseDisplayEditableWidget):
    """ A model for DisplayLineEdit/EditableLineEdit
    """
    # The actual type of the widget
    klass = Enum('DisplayLineEdit', 'EditableLineEdit')


class PopUpModel(BaseWidgetObjectData):
    """ A model for a pop up window """


class RunConfiguratorModel(BaseEditWidget):
    """ A model for RunConfigurator ListOfNodes editor
    """


class SceneLinkModel(BaseWidgetObjectData):
    """ A model for a scene link
    """
    # What scene is being linked to?
    target = String
    # Where should the target be opened?
    target_window = Enum(*list(SceneTargetWindow))
    # The text to be displayed
    text = String
    # A string describing the font
    font = String
    # A foreground color, CSS-style
    foreground = String
    # A background color, CSS-style
    background = String('transparent')
    # The line width of a frame around the text
    frame_width = Int(0)


class StickerModel(BaseWidgetObjectData):
    """ A model for a Sticker widget"""
    # The text to be displayed
    text = String
    # A string describing the font
    font = String
    # A foreground color, CSS-style
    foreground = String
    # A background color, CSS-style
    background = String('white')


class WebLinkModel(BaseWidgetObjectData):
    """ A model for the weblink widget
    """
    # What hyperlink do we store
    target = String
    # The text to be displayed
    text = String
    # A string describing the font
    font = String
    # A foreground color, CSS-style
    foreground = String
    # A background color, CSS-style
    background = String('transparent')
    # The line width of a frame around the text
    frame_width = Int(1)


class SliderModel(BaseEditWidget):
    """ A model for Slider"""


class TickSliderModel(BaseEditWidget):
    """ A model for TickSlider"""
    ticks = Int(1)
    show_value = Bool(True)


class DisplayTimeModel(BaseWidgetObjectData):
    """ A model for the time widget"""
    time_format = String('%H:%M:%S')


def _read_geometry_data(element):
    """ Read the geometry information.
    """
    return get_numbers(('x', 'y', 'width', 'height'), element)


def _write_class_and_geometry(model, element, widget_class_name):
    """ Write out the class name and geometry information.
    """
    element.set(NS_KARABO + 'class', widget_class_name)
    set_numbers(('x', 'y', 'width', 'height'), model, element)


@register_scene_reader('Label', version=1)
def __label_reader(element):
    """ A reader for Label objects in Version 1 scenes
    """
    traits = _read_geometry_data(element)
    traits['text'] = element.get(NS_KARABO + 'text', '')
    traits['font'] = element.get(NS_KARABO + 'font', '')
    traits['foreground'] = element.get(NS_KARABO + 'foreground', 'black')

    bg = element.get(NS_KARABO + 'background')
    if bg is not None:
        traits['background'] = bg
    fw = element.get(NS_KARABO + 'frameWidth')
    if fw is not None:
        traits['frame_width'] = int(fw)
    return LabelModel(**traits)


@register_scene_writer(LabelModel)
def __label_writer(model, parent):
    """ A writer for LabelModel objects
    """
    element = SubElement(parent, WIDGET_ELEMENT_TAG)

    _write_class_and_geometry(model, element, 'Label')

    for name in ('text', 'font', 'foreground'):
        element.set(NS_KARABO + name, getattr(model, name))

    element.set(NS_KARABO + 'frameWidth', str(model.frame_width))
    if model.background != '':
        element.set(NS_KARABO + 'background', model.background)

    return element


@register_scene_reader('TickSlider')
@register_scene_reader('Slider')  # Deprecated Slider
def _tick_slider_reader(element):
    traits = read_base_widget_data(element)
    traits['ticks'] = int(element.get(NS_KARABO + 'ticks', 1))
    show_value = element.get(NS_KARABO + 'show_value', 'true')
    traits['show_value'] = show_value.lower() == 'true'

    return TickSliderModel(**traits)


@register_scene_writer(SliderModel)
def _slider_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'TickSlider')
    element.set(NS_KARABO + 'ticks', '1')
    element.set(NS_KARABO + 'show_value', 'true')

    return element


@register_scene_writer(TickSliderModel)
def _tick_slider_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'TickSlider')
    element.set(NS_KARABO + 'ticks', str(model.ticks))
    element.set(NS_KARABO + 'show_value', str(model.show_value).lower())

    return element


@register_scene_reader('TimeLabel')
def _time_label_reader(element):
    traits = read_base_widget_data(element)
    time_format = element.get(NS_KARABO + 'time_format', '%H:%M:%S')
    traits['time_format'] = time_format

    return DisplayTimeModel(**traits)


@register_scene_writer(DisplayTimeModel)
def _time_label_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'TimeLabel')
    element.set(NS_KARABO + 'time_format', str(model.time_format))

    return element


@register_scene_reader('DisplayLabel')
def _display_label_reader(element):
    traits = read_base_widget_data(element)
    traits.update(read_font_format_data(element))

    return DisplayLabelModel(**traits)


@register_scene_writer(DisplayLabelModel)
def _display_label_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'DisplayLabel')
    write_font_format_data(model, element)

    return element


@register_scene_reader('WebLink', version=1)
def __web_link_reader(element):
    traits = _read_geometry_data(element)
    traits['target'] = element.get(NS_KARABO + 'target')
    traits['text'] = element.get(NS_KARABO + 'text', '')
    traits['font'] = element.get(NS_KARABO + 'font', '')
    traits['foreground'] = element.get(NS_KARABO + 'foreground', 'black')

    bg = element.get(NS_KARABO + 'background')
    if bg is not None:
        traits['background'] = bg
    fw = element.get(NS_KARABO + 'frameWidth')
    if fw is not None:
        traits['frame_width'] = int(fw)

    return WebLinkModel(**traits)


@register_scene_writer(WebLinkModel)
def __web_link_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    _write_class_and_geometry(model, element, 'WebLink')
    element.set(NS_KARABO + 'target', model.target)

    for name in ('text', 'font', 'foreground'):
        element.set(NS_KARABO + name, getattr(model, name))

    element.set(NS_KARABO + 'frameWidth', str(model.frame_width))
    if model.background != '':
        element.set(NS_KARABO + 'background', model.background)

    return element


@register_scene_reader('SceneLink', version=1)
def __scene_link_reader(element):
    traits = _read_geometry_data(element)
    traits['target'] = element.get(NS_KARABO + 'target')
    # If unspecified, the default is 'mainwin'
    target_window = element.get(NS_KARABO + 'target_window', 'mainwin')
    traits['target_window'] = SceneTargetWindow(target_window)
    traits['text'] = element.get(NS_KARABO + 'text', '')
    traits['font'] = element.get(NS_KARABO + 'font', '')
    traits['foreground'] = element.get(NS_KARABO + 'foreground', 'black')

    bg = element.get(NS_KARABO + 'background')
    if bg is not None:
        traits['background'] = bg
    fw = element.get(NS_KARABO + 'frameWidth')
    if fw is not None:
        traits['frame_width'] = int(fw)

    return SceneLinkModel(**traits)


@register_scene_writer(SceneLinkModel)
def __scene_link_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    _write_class_and_geometry(model, element, 'SceneLink')
    element.set(NS_KARABO + 'target', model.target)
    element.set(NS_KARABO + 'target_window', model.target_window.value)

    for name in ('text', 'font', 'foreground'):
        element.set(NS_KARABO + name, getattr(model, name))

    if model.background != '':
        element.set(NS_KARABO + 'background', model.background)

    return element


@register_scene_reader('StickerWidget')
def __sticker_widget_reader(element):
    traits = _read_geometry_data(element)
    traits['text'] = element.get(NS_KARABO + 'text', '')
    traits['font'] = element.get(NS_KARABO + 'font', '')
    traits['foreground'] = element.get(NS_KARABO + 'foreground', 'black')

    bg = element.get(NS_KARABO + 'background')
    if bg is not None:
        traits['background'] = bg

    return StickerModel(**traits)


@register_scene_writer(StickerModel)
def __sticker_widget_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    _write_class_and_geometry(model, element, 'StickerWidget')
    for name in ('text', 'font', 'foreground'):
        element.set(NS_KARABO + name, getattr(model, name))

    if model.background != '':
        element.set(NS_KARABO + 'background', model.background)

    return element


def _build_empty_widget_readers_and_writers():
    """ Build readers and writers for the empty widget classes
    """

    def _build_reader_func(klass):
        def reader(element):
            traits = read_base_widget_data(element)
            return klass(**traits)

        return reader

    def _build_writer_func(name):
        def writer(model, parent):
            element = SubElement(parent, WIDGET_ELEMENT_TAG)
            write_base_widget_data(model, element, name)
            return element

        return writer

    names = ('AnalogModel', 'BitfieldModel', 'DaemonManagerModel',
             'DisplayListModel', 'DisplayTextLogModel',
             'EditableListModel', 'EditableRegexListModel',
             'EditableListElementModel', 'EditableRegexModel',
             'EditableSpinBoxModel', 'GlobalAlarmModel',
             'HexadecimalModel', 'IntLineEditModel', 'LampModel', 'PopUpModel',
             'RunConfiguratorModel', 'WidgetNodeModel')
    for name in names:
        klass = globals()[name]
        file_name = name[:-len('Model')]
        register_scene_reader(file_name, version=1)(_build_reader_func(klass))
        register_scene_writer(klass)(_build_writer_func(file_name))


def _build_empty_display_editable_readers_and_writers():
    """ Build readers and writers for the empty widget classes which come in
    Editable and Display types.
    """

    def _build_reader_func(klass):
        def reader(element):
            traits = read_empty_display_editable_widget(element)
            return klass(**traits)

        return reader

    def _writer_func(model, parent):
        element = SubElement(parent, WIDGET_ELEMENT_TAG)
        write_base_widget_data(model, element, model.klass)
        return element

    names = ('CheckBoxModel', 'ChoiceElementModel', 'ComboBoxModel',
             'DirectoryModel', 'FileInModel', 'FileOutModel', 'LineEditModel')
    for name in names:
        klass = globals()[name]
        file_name = name[:-len('Model')]
        reader = _build_reader_func(klass)
        register_scene_reader('Display' + file_name, version=1)(reader)
        register_scene_reader('Editable' + file_name, version=1)(reader)
        register_scene_writer(klass)(_writer_func)


# Call the builders to register all the readers and writers
_build_empty_widget_readers_and_writers()
_build_empty_display_editable_readers_and_writers()
