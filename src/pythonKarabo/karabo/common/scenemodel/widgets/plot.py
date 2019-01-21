from xml.etree.ElementTree import SubElement

from traits.api import Bool, Enum, Instance, Int, List, String

from karabo.common.api import BaseSavableModel
from karabo.common.scenemodel.bases import BaseWidgetObjectData
from karabo.common.scenemodel.const import NS_KARABO, WIDGET_ELEMENT_TAG
from karabo.common.scenemodel.io_utils import (
    read_base_widget_data, read_empty_display_editable_widget,
    write_base_widget_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)


class PlotCurveModel(BaseSavableModel):
    """ A model for plot curve data
    """
    # The device with the data
    device = String
    # The property name for that data
    path = String
    # The encoded pickle of the curve object
    curve_object_data = String

    def __hash__(self):
        """ Return a unique hash value for this object.
            Device + Path is the uniqueness criteria.
        """
        return hash(self.device) ^ hash(self.path)


class LinePlotModel(BaseWidgetObjectData):
    """ A model for line plot objects
    """
    # The actual type of the widget
    klass = Enum('DisplayTrendline', 'XYVector')
    # The plots for this object
    boxes = List(Instance(PlotCurveModel))

    def add_curve(self, curve_model):
        """Maybe add a curve (box) to the model, but only if it is not a
        duplicate of an existing curve in the model. If it is an existing
        curve, then update the data.
        """
        existing = self._find_all_matching_boxes(curve_model)
        if not existing:
            self.boxes.append(curve_model)
        else:
            assert len(existing) == 1
            existing[0].curve_object_data = curve_model.curve_object_data

    def _find_all_matching_boxes(self, curve_model):
        matches = []
        model_hash = hash(curve_model)
        for box in self.boxes:
            if hash(box) == model_hash:
                matches.append(box)
        return matches


class SparklineModel(BaseWidgetObjectData):
    """ A model for a Sparkline
    """
    # Time span displayed by the sparkline
    time_base = Int(600)
    # If True, show the current value next to the line
    show_value = Bool(False)
    # If True, show the current alarms if present
    alarm_range = Bool(True)
    # String format for the value when it is shown
    show_format = String("0.2f")


@register_scene_reader('DisplayTrendline', version=1)
@register_scene_reader('XYVector', version=1)
def _line_plot_reader(read_func, element):
    traits = read_empty_display_editable_widget(element)
    boxes = []
    for child_elem in element:
        assert child_elem.tag == NS_KARABO + 'box'
        box_traits = {
            'device': child_elem.get("device"),
            'path': child_elem.get("path"),
            'curve_object_data': child_elem.text,
        }
        model = PlotCurveModel(**box_traits)
        model.initialized = True  # Satisfy BaseSavableModel
        boxes.append(model)
    traits['boxes'] = boxes
    return LinePlotModel(**traits)


@register_scene_writer(LinePlotModel)
def _line_plot_writer(write_func, model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, model.klass)
    for box in model.boxes:
        elem = SubElement(element, NS_KARABO + 'box')
        elem.set("device", box.device)
        elem.set("path", box.path)
        elem.text = box.curve_object_data
    return element


@register_scene_reader('DisplaySparkline', version=2)
def _display_sparkline_reader(read_func, element):
    traits = read_base_widget_data(element)
    traits['time_base'] = int(element.get(NS_KARABO + 'time_base', 600))
    traits['show_format'] = element.get(NS_KARABO + 'show_format', '0.2f')
    show_value = element.get(NS_KARABO + 'show_value', 'false')
    traits['show_value'] = (show_value.lower() == 'true')
    alarm_range = element.get(NS_KARABO + 'alarm_range', 'true')
    traits['alarm_range'] = (alarm_range.lower() == 'true')
    return SparklineModel(**traits)


@register_scene_writer(SparklineModel)
def _display_sparkline_writer(write_func, model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, 'DisplaySparkline')
    element.set(NS_KARABO + 'time_base', str(model.time_base))
    element.set(NS_KARABO + 'show_value', str(model.show_value).lower())
    element.set(NS_KARABO + 'alarm_range', str(model.alarm_range).lower())
    element.set(NS_KARABO + 'show_format', str(model.show_format))
    return element
