from xml.etree.ElementTree import SubElement

from traits.api import Enum, Int, String

from karabo.common.scenemodel.bases import BaseWidgetObjectData
from karabo.common.scenemodel.const import NS_KARABO, NS_SVG
from karabo.common.scenemodel.io_utils import (
    get_numbers, set_numbers, read_base_widget_data,
    read_empty_display_editable_widget, write_base_widget_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)


class BitfieldModel(BaseWidgetObjectData):
    """ A model for Bitfield"""


class CheckBoxModel(BaseWidgetObjectData):
    """ A model for DisplayCheckBox/EditableCheckBox
    """
    # The actual type of the widget
    klass = Enum('DisplayCheckBox', 'EditableCheckBox')


class ChoiceElementModel(BaseWidgetObjectData):
    """ A model for DisplayChoiceElement/EditableChoiceElement
    """
    # The actual type of the widget
    klass = Enum('DisplayChoiceElement', 'EditableChoiceElement')


class ComboBoxModel(BaseWidgetObjectData):
    """ A model for DisplayComboBox/EditableComboBox
    """
    # The actual type of the widget
    klass = Enum('DisplayComboBox', 'EditableComboBox')


class DirectoryModel(BaseWidgetObjectData):
    """ A model for DisplayDirectory/EditableDirectory
    """
    # The actual type of the widget
    klass = Enum('DisplayDirectory', 'EditableDirectory')


class DisplayCommandModel(BaseWidgetObjectData):
    """ A model for DisplayCommand"""


class DisplayLabelModel(BaseWidgetObjectData):
    """ A model for DisplayLabel"""


class DisplayPlotModel(BaseWidgetObjectData):
    """ A model for DisplayPlot"""


class DoubleLineEditModel(BaseWidgetObjectData):
    """ A model for DoubleLineEdit"""


class EditableListModel(BaseWidgetObjectData):
    """ A model for EditableList"""


class EditableListElementModel(BaseWidgetObjectData):
    """ A model for EditableListElement"""


class EditableSpinBoxModel(BaseWidgetObjectData):
    """ A model for EditableSpinBox"""


class FileInModel(BaseWidgetObjectData):
    """ A model for DisplayFileIn/EditableFileIn
    """
    # The actual type of the widget
    klass = Enum('DisplayFileIn', 'EditableFileIn')


class FileOutModel(BaseWidgetObjectData):
    """ A model for DisplayFileOut/EditableFileOut
    """
    # The actual type of the widget
    klass = Enum('DisplayFileOut', 'EditableFileOut')


class HexadecimalModel(BaseWidgetObjectData):
    """ A model for Hexadecimal"""


class IntLineEditModel(BaseWidgetObjectData):
    """ A model for IntLineEdit"""


class KnobModel(BaseWidgetObjectData):
    """ A model for Knob"""


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
    background = String
    # The line width of a frame around the text
    frame_width = Int(0)


class LineEditModel(BaseWidgetObjectData):
    """ A model for DisplayLineEdit/EditableLineEdit
    """
    # The actual type of the widget
    klass = Enum('DisplayLineEdit', 'EditableLineEdit')


class PopUpModel(BaseWidgetObjectData):
    """ A model for a pop up window """


class SceneLinkModel(BaseWidgetObjectData):
    """ A model for a scene link
    """
    # What scene is being linked to?
    target = String


class SliderModel(BaseWidgetObjectData):
    """ A model for Slider"""


class WorkflowItemModel(BaseWidgetObjectData):
    """ A model for a WorkflowItem
    """
    # The device id for this item
    device_id = String
    # A string describing the font for this item
    font = String
    # What time of item is this?
    klass = Enum('WorkflowItem', 'WorkflowGroupItem')


class XYPlotModel(BaseWidgetObjectData):
    """ A model for XYPlot"""


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
def __label_reader(read_func, element):
    """ A reader for Label objects in Version 1 scenes
    """
    traits = _read_geometry_data(element)
    traits['text'] = element.get(NS_KARABO + 'text')
    traits['font'] = element.get(NS_KARABO + 'font')
    traits['foreground'] = element.get(NS_KARABO + 'foreground', 'black')

    bg = element.get(NS_KARABO + 'background')
    if bg is not None:
        traits['background'] = bg
    fw = element.get(NS_KARABO + 'frameWidth')
    if fw is not None:
        traits['frame_width'] = int(fw)
    return LabelModel(**traits)


@register_scene_writer(LabelModel)
def __label_writer(write_func, model, parent):
    """ A writer for LabelModel objects
    """
    element = SubElement(parent, NS_SVG + 'rect')
    _write_class_and_geometry(model, element, 'Label')

    for name in ('text', 'font', 'foreground'):
        element.set(NS_KARABO + name, getattr(model, name))

    element.set(NS_KARABO + 'frameWidth', str(model.frame_width))
    if model.background != '':
        element.set(NS_KARABO + 'background', model.background)

    return element


@register_scene_reader('SceneLink', version=1)
def __scene_link_reader(read_func, element):
    traits = _read_geometry_data(element)
    traits['target'] = element.get(NS_KARABO + 'target')
    return SceneLinkModel(**traits)


@register_scene_writer(SceneLinkModel)
def __scene_link_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    _write_class_and_geometry(model, element, 'SceneLink')
    element.set(NS_KARABO + 'target', model.target)
    return element


@register_scene_reader('WorkflowItem', version=1)
@register_scene_reader('WorkflowGroupItem', version=1)
def __workflow_item_reader(read_func, element):
    traits = _read_geometry_data(element)
    traits['klass'] = element.get(NS_KARABO + 'class')
    traits['device_id'] = element.get(NS_KARABO + 'text')
    traits['font'] = element.get(NS_KARABO + 'font')
    return WorkflowItemModel(**traits)


@register_scene_writer(WorkflowItemModel)
def __workflow_item_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    _write_class_and_geometry(model, element, model.klass)
    element.set(NS_KARABO + 'text', model.device_id)
    element.set(NS_KARABO + 'font', model.font)
    return element


def _build_empty_widget_readers_and_writers():
    """ Build readers and writers for the empty widget classes
    """
    def _build_reader_func(klass):
        def reader(read_func, element):
            traits = read_base_widget_data(element)
            return klass(**traits)
        return reader

    def _build_writer_func(name):
        def writer(write_func, model, parent):
            element = SubElement(parent, NS_SVG + 'rect')
            write_base_widget_data(model, element, name)
            return element
        return writer

    names = ('BitfieldModel', 'DisplayCommandModel', 'DisplayLabelModel',
             'DisplayPlotModel', 'DoubleLineEditModel', 'EditableListModel',
             'EditableListElementModel', 'EditableSpinBoxModel',
             'HexadecimalModel', 'IntLineEditModel', 'KnobModel', 'PopUpModel',
             'SliderModel', 'XYPlotModel')
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
        def reader(read_func, element):
            traits = read_empty_display_editable_widget(element)
            return klass(**traits)
        return reader

    def _writer_func(write_func, model, parent):
        element = SubElement(parent, NS_SVG + 'rect')
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
