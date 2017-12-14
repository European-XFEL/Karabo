from xml.etree.ElementTree import SubElement

from traits.api import Bool, Enum, Float, Int, String

from karabo.common.scenemodel.bases import (
    BaseDisplayEditableWidget, BaseEditWidget, BaseWidgetObjectData)
from karabo.common.scenemodel.const import NS_KARABO, WIDGET_ELEMENT_TAG
from karabo.common.scenemodel.io_utils import (
    read_base_widget_data, read_empty_display_editable_widget,
    write_base_widget_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)


class ColorBoolModel(BaseWidgetObjectData):
    """ A model for DisplayBool Widget"""
    invert = Bool(False)


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
    # The text shown on the widget
    text = String


class EvaluatorModel(BaseWidgetObjectData):
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
    # The schema which defines the table
    # XXX: This is stored as text for now...
    column_schema = String
    # The actual type of the widget
    klass = Enum('DisplayTableElement', 'EditableTableElement')


@register_scene_reader('DisplayColorBool', version=2)
def _color_bool_reader(read_func, element):
    traits = read_base_widget_data(element)
    value = element.get(NS_KARABO + 'invert')
    traits['invert'] = (value.lower() == 'true')
    return ColorBoolModel(**traits)


@register_scene_writer(ColorBoolModel)
def _color_bool_writer(write_func, model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'DisplayColorBool')
    element.set(NS_KARABO + 'invert', str(model.invert).lower())
    return element


@register_scene_reader('DoubleLineEdit', version=1)
def _double_line_edit_reader(read_func, element):
    traits = read_base_widget_data(element)
    decimals = element.get(NS_KARABO + 'decimals', '')
    if decimals:
        traits['decimals'] = int(decimals)
    return DoubleLineEditModel(**traits)


@register_scene_writer(DoubleLineEditModel)
def _double_line_edit_writer(write_func, model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'DoubleLineEdit')
    element.set(NS_KARABO + 'decimals', str(model.decimals))
    return element


@register_scene_reader('DisplayProgressBar', version=2)
def _display_progress_bar_reader(read_func, element):
    traits = read_base_widget_data(element)
    value = element.get(NS_KARABO + 'is_vertical', '')
    traits['is_vertical'] = (value.lower() == 'true')
    return DisplayProgressBarModel(**traits)


@register_scene_writer(DisplayProgressBarModel)
def _display_progress_bar_writer(write_func, model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'DisplayProgressBar')
    element.set(NS_KARABO + 'is_vertical', str(model.is_vertical).lower())
    return element


@register_scene_reader('DisplayStateColor', version=1)
def _display_state_color_reader(read_func, element):
    traits = read_base_widget_data(element)
    traits['text'] = element.get(NS_KARABO + 'staticText', '')
    return DisplayStateColorModel(**traits)


@register_scene_writer(DisplayStateColorModel)
def _display_state_color_writer(write_func, model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'DisplayStateColor')
    element.set(NS_KARABO + 'staticText', model.text)
    return element


@register_scene_reader('Evaluator', version=1)
def _evaluator_reader(read_func, element):
    traits = read_base_widget_data(element)
    traits['expression'] = element.get('expression')
    return EvaluatorModel(**traits)


@register_scene_writer(EvaluatorModel)
def _evaluator_writer(write_func, model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'Evaluator')
    element.set('expression', model.expression)
    return element


@register_scene_reader('FloatSpinBox', version=1)
def _float_spin_box_reader(read_func, element):
    traits = read_base_widget_data(element)
    traits['step'] = float(element.get(NS_KARABO + 'step', 1.0))
    decimals = element.get(NS_KARABO + 'decimals', '')
    if decimals:
        traits['decimals'] = int(decimals)
    return FloatSpinBoxModel(**traits)


@register_scene_writer(FloatSpinBoxModel)
def _float_spin_box_writer(write_func, model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'FloatSpinBox')
    element.set(NS_KARABO + 'step', str(model.step))
    element.set(NS_KARABO + 'decimals', str(model.decimals))
    return element


@register_scene_reader('Monitor', version=1)
def _monitor_reader(read_func, element):
    traits = read_base_widget_data(element)
    traits['filename'] = element.get('filename', '')
    traits['interval'] = float(element.get('interval'))
    return MonitorModel(**traits)


@register_scene_writer(MonitorModel)
def _monitor_writer(write_func, model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'Monitor')
    element.set('interval', str(model.interval))
    if len(model.filename) > 0:
        element.set('filename', model.filename)
    return element


@register_scene_reader('SingleBit', version=1)
def _single_bit_reader(read_func, element):
    traits = read_base_widget_data(element)
    invert = element.get(NS_KARABO + 'invert', '')
    traits['invert'] = (invert.lower() == 'true')
    traits['bit'] = int(element.get(NS_KARABO + 'bit', 0))
    return SingleBitModel(**traits)


@register_scene_writer(SingleBitModel)
def _single_bit_writer(write_func, model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'SingleBit')
    element.set(NS_KARABO + 'bit', str(model.bit))
    element.set(NS_KARABO + 'invert', str(model.invert).lower())
    return element


@register_scene_reader('DisplayTableElement', version=1)
@register_scene_reader('EditableTableElement', version=1)
def _table_element_reader(read_func, element):
    traits = read_empty_display_editable_widget(element)
    traits['column_schema'] = element.get(NS_KARABO + 'columnSchema', '')
    return TableElementModel(**traits)


@register_scene_writer(TableElementModel)
def _table_element_writer(write_func, model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, model.klass)
    element.set(NS_KARABO + 'columnSchema', model.column_schema)
    return element
