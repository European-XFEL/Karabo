from xml.etree.ElementTree import SubElement

from traits.api import (HasTraits, Bool, Dict, Enum, Float, Instance, Int,
                        List, String)

from .bases import BaseWidgetObjectData
from .const import NS_KARABO, NS_SVG
from .exceptions import SceneWriterException
from .io_utils import get_numbers, set_numbers
from .registry import register_scene_reader, register_scene_writer

VACUUM_WIDGETS = (
    'CryoCoolerWidget', 'HydraulicValveWidget', 'MaxiGaugeWidget',
    'MembranePumpWidget', 'MotorWidget', 'PressureGaugeWidget',
    'PressureSwitchWidget', 'RightAngleValveWidget', 'ShutOffValveWidget',
    'TemperatureProbeWidget', 'TurboPumpWidget', 'ValveWidget'
)


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


class DisplayAlignedImageModel(BaseWidgetObjectData):
    """ A model for DisplayAlignedImage"""


class DisplayCommandModel(BaseWidgetObjectData):
    """ A model for DisplayCommand"""


class DisplayIconsetModel(BaseWidgetObjectData):
    """ A model for DisplayIconset"""


class DisplayImageModel(BaseWidgetObjectData):
    """ A model for DisplayImage"""


class DisplayImageElementModel(BaseWidgetObjectData):
    """ A model for DisplayImageElement"""


class DisplayLabelModel(BaseWidgetObjectData):
    """ A model for DisplayLabel"""


class DisplayPlotModel(BaseWidgetObjectData):
    """ A model for DisplayPlot"""


class DisplayStateColorModel(BaseWidgetObjectData):
    """ A model for DisplayStateColor
    """
    # The text shown on the widget
    text = String
    # The possible colors for the states
    colors = Dict


class DoubleLineEditModel(BaseWidgetObjectData):
    """ A model for DoubleLineEdit"""


class EditableListModel(BaseWidgetObjectData):
    """ A model for EditableList"""


class EditableListElementModel(BaseWidgetObjectData):
    """ A model for EditableListElement"""


class EditableSpinBoxModel(BaseWidgetObjectData):
    """ A model for EditableSpinBox"""


class EvaluatorModel(BaseWidgetObjectData):
    """ A model for Evaluator
    """
    # The expression which is evaluated
    expression = String


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


class FloatSpinBoxModel(BaseWidgetObjectData):
    """ A model for FloatSpinBox
    """
    # The step size
    step = Float


class HexadecimalModel(BaseWidgetObjectData):
    """ A model for Hexadecimal"""


class IconData(HasTraits):
    """ A base class for Icon (Item) data.
    """
    # XXX: Not sure what this is...
    equal = Bool
    # The value of the icon??
    value = String
    # A URL for an icon
    image = String


class BaseIconsModel(BaseWidgetObjectData):
    """ A base class for Icons widgets.
    """
    # The icons shown here
    values = List(Instance(IconData))


class DigitIconsModel(BaseIconsModel):
    """ A model for DigitIcons"""


class SelectionIconsModel(BaseIconsModel):
    """ A model for SelectionIcons"""


class TextIconsModel(BaseIconsModel):
    """ A model for TextIcons"""


class IntLineEditModel(BaseWidgetObjectData):
    """ A model for IntLineEdit"""


class KnobModel(BaseWidgetObjectData):
    """ A model for Knob"""


class LineEditModel(BaseWidgetObjectData):
    """ A model for DisplayLineEdit/EditableLineEdit
    """
    # The actual type of the widget
    klass = Enum('DisplayLineEdit', 'EditableLineEdit')


class PlotCurveModel(HasTraits):
    """ A model for plot curve data
    """
    # The device with the data
    device = String
    # The property name for that data
    path = String
    # The encoded pickle of the curve object
    curve_object_data = String


class LinePlotModel(BaseWidgetObjectData):
    """ A model for line plot objects
    """
    # The actual type of the widget
    klass = Enum('DisplayTrendline', 'XYVector')
    # The plots for this object
    boxes = List(Instance(PlotCurveModel))


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
    # Which bit is being edited
    bit = Int


class SliderModel(BaseWidgetObjectData):
    """ A model for Slider"""


class TableElementModel(BaseWidgetObjectData):
    """ A model for TableElement
    """
    # The schema which defines the table
    # XXX: This is stored as text for now...
    column_schema = String
    # The actual type of the widget
    klass = Enum('DisplayTableElement', 'EditableTableElement')


class VacuumWidgetModel(BaseWidgetObjectData):
    """ A model for VacuumWidget objects"""
    # The actual type of the widget
    klass = Enum(*VACUUM_WIDGETS)


class XYPlotModel(BaseWidgetObjectData):
    """ A model for XYPlot"""


def _read_base_widget_data(element):
    """ Read the attributes common to all "widget" elements
    """
    traits = get_numbers(('x', 'y', 'width', 'height'), element)
    traits['parent_component'] = element.get(NS_KARABO + 'class')
    keys = element.get(NS_KARABO + 'keys', '')
    if len(keys) > 0:
        traits['keys'] = keys.split(',')
    return traits


def _read_empty_display_editable_widget(element):
    """ Read the attributes common to all widgets with Display/Editable forms.
    """
    traits = _read_base_widget_data(element)
    traits['klass'] = element.get(NS_KARABO + 'widget')
    return traits


def _read_icon_elements(parent, tag):
    """ Read the icons for an icons widget.
    """
    icons = []
    for sub in parent:
        if sub.tag != tag:
            continue
        traits = {
            'image': sub.get('image', '')
        }
        if sub.get('equal') is not None:
            traits['value'] = sub.text
            traits['equal'] = True if sub.get('equal') == 'true' else False
        icons.append(IconData(**traits))
    return icons


def _write_base_widget_data(model, element, widget_class_name):
    """ Write out the attributes common to all "widget" elements
    """
    if len(model.parent_component) == 0:
        msg = "Widget {} has no parent component!".format(widget_class_name)
        raise SceneWriterException(msg)
    element.set(NS_KARABO + 'class', model.parent_component)
    element.set(NS_KARABO + 'widget', widget_class_name)
    element.set(NS_KARABO + 'keys', ",".join(model.keys))
    set_numbers(('x', 'y', 'width', 'height'), model, element)


def _write_icon_elements(icons, parent, tag):
    """ Write out the sub elements of an icons widget.
    """
    for ic in icons:
        sub = SubElement(parent, tag)
        if ic.image:
            sub.set('image', ic.image)
        if ic.value:
            sub.text = ic.value
            if ic.equal:
                sub.set('equal', str(ic.equal).lower())


@register_scene_reader('DisplayStateColor', version=1)
def _display_state_color_reader(read_func, element):
    traits = _read_base_widget_data(element)
    traits['text'] = element.get(NS_KARABO + 'staticText', '')
    colors = {}
    for sub in element:
        if sub.tag != (NS_KARABO + 'sc'):
            continue
        red = int(sub.get('red'))
        green = int(sub.get('green'))
        blue = int(sub.get('blue'))
        alpha = int(sub.get('alpha'))
        colors[sub.text] = (red, green, blue, alpha)
    traits['colors'] = colors
    return DisplayStateColorModel(**traits)


@register_scene_writer(DisplayStateColorModel)
def _display_state_color_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    _write_base_widget_data(model, element, 'DisplayStateColor')
    element.set(NS_KARABO + 'staticText', model.text)
    for name, color in model.colors.items():
        sub = SubElement(element, NS_KARABO + "sc")
        sub.text = name
        red, green, blue, alpha = color
        sub.set('red', str(red))
        sub.set('green', str(green))
        sub.set('blue', str(blue))
        sub.set('alpha', str(alpha))
    return element


@register_scene_reader('Evaluator', version=1)
def _evaluator_reader(read_func, element):
    traits = _read_base_widget_data(element)
    traits['expression'] = element.get('expression')
    return EvaluatorModel(**traits)


@register_scene_writer(EvaluatorModel)
def _evaluator_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    _write_base_widget_data(model, element, 'Evaluator')
    element.set('expression', model.expression)
    return element


@register_scene_reader('FloatSpinBox', version=1)
def _float_spin_box_reader(read_func, element):
    traits = _read_base_widget_data(element)
    traits['step'] = float(element.get(NS_KARABO + 'step', 1.0))
    return FloatSpinBoxModel(**traits)


@register_scene_writer(FloatSpinBoxModel)
def _float_spin_box_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    _write_base_widget_data(model, element, 'FloatSpinBox')
    element.set(NS_KARABO + 'step', str(model.step))
    return element


@register_scene_reader('DisplayTrendline', version=1)
@register_scene_reader('XYVector', version=1)
def _line_plot_reader(read_func, element):
    traits = _read_empty_display_editable_widget(element)
    boxes = []
    for child_elem in element:
        assert child_elem.tag == NS_KARABO + 'box'
        box_traits = {
            'device': child_elem.get("device"),
            'path': child_elem.get("path"),
            'curve_object_data': child_elem.text,
        }
        boxes.append(PlotCurveModel(**box_traits))
    traits['boxes'] = boxes
    return LinePlotModel(**traits)


@register_scene_writer(LinePlotModel)
def _line_plot_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    _write_base_widget_data(model, element, model.klass)
    for box in model.boxes:
        elem = SubElement(element, NS_KARABO + 'box')
        elem.set("device", box.device)
        elem.set("path", box.path)
        elem.text = box.curve_object_data
    return element


@register_scene_reader('Monitor', version=1)
def _monitor_reader(read_func, element):
    traits = _read_base_widget_data(element)
    traits['filename'] = element.get('filename', '')
    traits['interval'] = float(element.get('interval'))
    return MonitorModel(**traits)


@register_scene_writer(MonitorModel)
def _monitor_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    _write_base_widget_data(model, element, 'Monitor')
    element.set('interval', str(model.interval))
    if len(model.filename) > 0:
        element.set('filename', model.filename)
    return element


@register_scene_reader('SingleBit', version=1)
def _single_bit_reader(read_func, element):
    traits = _read_base_widget_data(element)
    traits['bit'] = int(element.get(NS_KARABO + 'bit', 0))
    return SingleBitModel(**traits)


@register_scene_writer(SingleBitModel)
def _single_bit_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    _write_base_widget_data(model, element, 'SingleBit')
    element.set(NS_KARABO + 'bit', str(model.bit))
    return element


@register_scene_reader('DisplayTableElement', version=1)
@register_scene_reader('EditableTableElement', version=1)
def _table_element_reader(read_func, element):
    traits = _read_empty_display_editable_widget(element)
    traits['column_schema'] = element.get(NS_KARABO + 'columnSchema', '')
    return TableElementModel(**traits)


@register_scene_writer(TableElementModel)
def _table_element_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    _write_base_widget_data(model, element, model.klass)
    element.set(NS_KARABO + 'columnSchema', model.column_schema)
    return element


@register_scene_writer(VacuumWidgetModel)
def _vacuum_widget_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    _write_base_widget_data(model, element, model.klass)
    return element


def _build_empty_widget_readers_and_writers():
    """ Build readers and writers for the empty widget classes
    """
    def _build_reader_func(klass):
        def reader(read_func, element):
            traits = _read_empty_display_editable_widget(element)
            return klass(**traits)
        return reader

    def _build_writer_func(name):
        def writer(write_func, model, parent):
            element = SubElement(parent, NS_SVG + 'rect')
            _write_base_widget_data(model, element, name)
            return element
        return writer

    names = ('BitfieldModel', 'DisplayAlignedImageModel',
             'DisplayCommandModel', 'DisplayIconsetModel', 'DisplayImageModel',
             'DisplayImageElementModel', 'DisplayLabelModel',
             'DisplayPlotModel', 'DoubleLineEditModel', 'EditableListModel',
             'EditableListElementModel', 'EditableSpinBoxModel',
             'HexadecimalModel', 'IntLineEditModel', 'KnobModel',
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
            traits = _read_empty_display_editable_widget(element)
            return klass(**traits)
        return reader

    def _writer_func(write_func, model, parent):
        element = SubElement(parent, NS_SVG + 'rect')
        _write_base_widget_data(model, element, model.klass)
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


def _build_icon_widget_readers_and_writers():
    """ Build readers and writers for the icon widget classes
    """
    def _build_reader_func(klass, tag):
        def reader(read_func, element):
            traits = _read_base_widget_data(element)
            traits['values'] = _read_icon_elements(element, NS_KARABO + tag)
            return klass(**traits)
        return reader

    def _build_writer_func(name, tag):
        def writer(write_func, model, parent):
            element = SubElement(parent, NS_SVG + 'rect')
            _write_base_widget_data(model, element, name)
            _write_icon_elements(model.values, element, NS_KARABO + tag)
            return element
        return writer

    widgets = (('DigitIconsModel', 'value'), ('SelectionIconsModel', 'option'),
               ('TextIconsModel', 're'))
    for name, tag in widgets:
        klass = globals()[name]
        file_name = name[:-len('Model')]
        reader = _build_reader_func(klass, tag)
        register_scene_reader(file_name, version=1)(reader)
        register_scene_writer(klass)(_build_writer_func(file_name, tag))


def _build_vacuum_widget_readers():
    """ Build readers for all the possible vacuum widgets.
    """
    def _reader(read_func, element):
        traits = _read_empty_display_editable_widget(element)
        return VacuumWidgetModel(**traits)

    for widget in VACUUM_WIDGETS:
        register_scene_reader(widget, version=1)(_reader)

# Call the builders to register all the readers and writers
_build_empty_widget_readers_and_writers()
_build_empty_display_editable_readers_and_writers()
_build_icon_widget_readers_and_writers()
_build_vacuum_widget_readers()
