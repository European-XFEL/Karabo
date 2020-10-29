from xml.etree.ElementTree import SubElement

from traits.api import Bool, Enum, Float, Int, String

from karabo.common.scenemodel.bases import (
    BaseDisplayEditableWidget, BaseEditWidget, BaseWidgetObjectData)
from karabo.common.scenemodel.const import (
    NS_KARABO, WIDGET_ELEMENT_TAG, SceneTargetWindow)
from karabo.common.scenemodel.io_utils import (
    read_base_widget_data, read_empty_display_editable_widget,
    read_font_format_data, write_base_widget_data, write_font_format_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)

from .simple import DisplayLabelModel


class ColorBoolModel(BaseWidgetObjectData):
    """ A model for DisplayBool Widget"""
    invert = Bool(False)


class DisplayCommandModel(BaseWidgetObjectData):
    """ A model for DisplayCommand"""
    requires_confirmation = Bool(False)


class DisplayIconCommandModel(BaseWidgetObjectData):
    """ A model for DisplayIconCommand"""
    icon_name = String


class DeviceSceneLinkModel(BaseWidgetObjectData):
    """ A model for a DeviceSceneLink Widget """

    # What device scene is being linked to?
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
    background = String("transparent")
    # The line width of a frame around the text
    frame_width = Int(1)


class DoubleLineEditModel(BaseEditWidget):
    """ A model for DoubleLineEdit Widget"""

    # The floating point precision
    decimals = Int(-1)


class DisplayProgressBarModel(BaseWidgetObjectData):
    """ A model for Progress Bar
    """

    # the orientation of the progress bar
    is_vertical = Bool(False)


class DisplayStateColorModel(BaseWidgetObjectData):
    """ A model for DisplayStateColor
    """
    # The state shown on the widget
    show_string = Bool(False)


class ErrorBoolModel(BaseWidgetObjectData):
    """ A model for ErrorBool Widget"""
    invert = Bool(False)


class EvaluatorModel(DisplayLabelModel):
    """ A model for Evaluator
    """
    # The expression which is evaluated
    expression = String('x')


class FloatSpinBoxModel(BaseEditWidget):
    """ A model for FloatSpinBox
    """
    # The step size
    step = Float
    decimals = Int(3)


class MonitorModel(BaseWidgetObjectData):
    """ A model for Monitor
    """
    # A file path
    filename = String
    # An interval
    interval = Float


class SingleBitModel(BaseWidgetObjectData):
    """ A model for SingleBit
    """
    # Which bit is being displayed
    bit = Int
    # True if the value of the bit should be inverted
    invert = Bool(False)


class TableElementModel(BaseDisplayEditableWidget):
    """ A model for TableElement
    """
    # The actual type of the widget
    klass = Enum('DisplayTableElement', 'EditableTableElement')


@register_scene_reader('DisplayCommand', version=2)
def __display_command_reader(element):
    confirmation = element.get(NS_KARABO + 'requires_confirmation', '')
    confirmation = confirmation.lower() == 'true'
    traits = read_base_widget_data(element)
    traits['requires_confirmation'] = confirmation
    return DisplayCommandModel(**traits)


@register_scene_writer(DisplayCommandModel)
def __display_command_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'DisplayCommand')
    element.set(NS_KARABO + 'requires_confirmation',
                str(model.requires_confirmation).lower())
    return element


@register_scene_reader('DisplayIconCommand')
def __display_icon_command_reader(element):
    traits = read_base_widget_data(element)
    traits['icon_name'] = element.get(NS_KARABO + 'icon_name', '')

    return DisplayIconCommandModel(**traits)


@register_scene_writer(DisplayIconCommandModel)
def __display_icon_command_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'DisplayIconCommand')
    element.set(NS_KARABO + 'icon_name', model.icon_name)
    return element


@register_scene_reader('DeviceSceneLink', version=2)
def _device_scene_link_reader(element):
    traits = read_base_widget_data(element)
    # If unspecified, the default is 'scene'
    traits['target'] = element.get(NS_KARABO + 'target', '')
    traits['text'] = element.get(NS_KARABO + 'text', '')
    traits['font'] = element.get(NS_KARABO + 'font')
    traits['foreground'] = element.get(NS_KARABO + 'foreground', 'black')
    bg = element.get(NS_KARABO + 'background')
    if bg is not None:
        traits['background'] = bg
    fw = element.get(NS_KARABO + 'frameWidth')
    if fw is not None:
        traits['frame_width'] = int(fw)
    # If unspecified, the default is 'mainwin'
    target_window = element.get(NS_KARABO + 'target_window', 'mainwin')
    traits['target_window'] = SceneTargetWindow(target_window)
    return DeviceSceneLinkModel(**traits)


@register_scene_writer(DeviceSceneLinkModel)
def _device_scene_link_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'DeviceSceneLink')
    for name in ('text', 'font', 'foreground'):
        element.set(NS_KARABO + name, getattr(model, name))

    element.set(NS_KARABO + 'frameWidth', str(model.frame_width))
    if model.background != '':
        element.set(NS_KARABO + 'background', model.background)
    element.set(NS_KARABO + 'target', model.target)
    element.set(NS_KARABO + 'target_window', model.target_window.value)
    return element


@register_scene_reader('DisplayColorBool', version=2)
def _color_bool_reader(element):
    traits = read_base_widget_data(element)
    value = element.get(NS_KARABO + 'invert')
    traits['invert'] = (value.lower() == 'true')
    return ColorBoolModel(**traits)


@register_scene_writer(ColorBoolModel)
def _color_bool_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'DisplayColorBool')
    element.set(NS_KARABO + 'invert', str(model.invert).lower())
    return element


@register_scene_reader('DisplayErrorBool')
def _error_bool_reader(element):
    traits = read_base_widget_data(element)
    value = element.get(NS_KARABO + 'invert')
    traits['invert'] = (value.lower() == 'true')
    return ErrorBoolModel(**traits)


@register_scene_writer(ErrorBoolModel)
def _error_bool_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'DisplayErrorBool')
    element.set(NS_KARABO + 'invert', str(model.invert).lower())
    return element


@register_scene_reader('DoubleLineEdit', version=1)
def _double_line_edit_reader(element):
    traits = read_base_widget_data(element)
    decimals = element.get(NS_KARABO + 'decimals', '')
    if decimals:
        traits['decimals'] = int(decimals)
    return DoubleLineEditModel(**traits)


@register_scene_writer(DoubleLineEditModel)
def _double_line_edit_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'DoubleLineEdit')
    element.set(NS_KARABO + 'decimals', str(model.decimals))
    return element


@register_scene_reader('DisplayProgressBar', version=2)
def _display_progress_bar_reader(element):
    traits = read_base_widget_data(element)
    value = element.get(NS_KARABO + 'is_vertical', '')
    traits['is_vertical'] = (value.lower() == 'true')
    return DisplayProgressBarModel(**traits)


@register_scene_writer(DisplayProgressBarModel)
def _display_progress_bar_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'DisplayProgressBar')
    element.set(NS_KARABO + 'is_vertical', str(model.is_vertical).lower())
    return element


@register_scene_reader('DisplayStateColor', version=1)
def _display_state_color_reader(element):
    traits = read_base_widget_data(element)
    value = element.get(NS_KARABO + 'show_string', '')
    traits['show_string'] = (value.lower() == 'true')
    return DisplayStateColorModel(**traits)


@register_scene_writer(DisplayStateColorModel)
def _display_state_color_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'DisplayStateColor')
    element.set(NS_KARABO + 'show_string', str(model.show_string).lower())
    return element


@register_scene_reader('Evaluator', version=1)
def _evaluator_reader(element):
    traits = read_base_widget_data(element)
    traits.update(read_font_format_data(element))
    traits['expression'] = element.get('expression')
    return EvaluatorModel(**traits)


@register_scene_writer(EvaluatorModel)
def _evaluator_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'Evaluator')
    write_font_format_data(model, element)
    element.set('expression', model.expression)
    return element


@register_scene_reader('FloatSpinBox', version=1)
def _float_spin_box_reader(element):
    traits = read_base_widget_data(element)
    traits['step'] = float(element.get(NS_KARABO + 'step', 1.0))
    decimals = element.get(NS_KARABO + 'decimals', '')
    if decimals:
        traits['decimals'] = int(decimals)
    return FloatSpinBoxModel(**traits)


@register_scene_writer(FloatSpinBoxModel)
def _float_spin_box_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'FloatSpinBox')
    element.set(NS_KARABO + 'step', str(model.step))
    element.set(NS_KARABO + 'decimals', str(model.decimals))
    return element


@register_scene_reader('Monitor', version=1)
def _monitor_reader(element):
    traits = read_base_widget_data(element)
    traits['filename'] = element.get('filename', '')
    traits['interval'] = float(element.get('interval'))
    return MonitorModel(**traits)


@register_scene_writer(MonitorModel)
def _monitor_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'Monitor')
    element.set('interval', str(model.interval))
    if len(model.filename) > 0:
        element.set('filename', model.filename)
    return element


@register_scene_reader('SingleBit', version=1)
def _single_bit_reader(element):
    traits = read_base_widget_data(element)
    invert = element.get(NS_KARABO + 'invert', '')
    traits['invert'] = (invert.lower() == 'true')
    traits['bit'] = int(element.get(NS_KARABO + 'bit', 0))
    return SingleBitModel(**traits)


@register_scene_writer(SingleBitModel)
def _single_bit_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'SingleBit')
    element.set(NS_KARABO + 'bit', str(model.bit))
    element.set(NS_KARABO + 'invert', str(model.invert).lower())
    return element


@register_scene_reader('DisplayTableElement', version=1)
@register_scene_reader('EditableTableElement', version=1)
def _table_element_reader(element):
    traits = read_empty_display_editable_widget(element)
    return TableElementModel(**traits)


@register_scene_writer(TableElementModel)
def _table_element_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, model.klass)
    return element
