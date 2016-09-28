from xml.etree.ElementTree import SubElement

from traits.api import HasStrictTraits, Enum, Instance, List, String

from karabo.common.scenemodel.bases import BaseWidgetObjectData
from karabo.common.scenemodel.const import NS_KARABO, NS_SVG
from karabo.common.scenemodel.io_utils import (
    read_empty_display_editable_widget, write_base_widget_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)


class PlotCurveModel(HasStrictTraits):
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

    def _boxes_items_changed(self, event):
        """ Watch for duplicate boxes being added. When found, remove them
        after updating the existing box.
        """
        def _find_all_matching_boxes(model):
            matches = []
            model_hash = hash(model)
            for box in self.boxes:
                if hash(box) == model_hash:
                    matches.append(box)
            return matches

        for model in event.added:
            existing = _find_all_matching_boxes(model)
            if existing:
                existing[0].curve_object_data = model.curve_object_data
                if len(existing) > 1:
                    self.boxes.remove(model)


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
        boxes.append(PlotCurveModel(**box_traits))
    traits['boxes'] = boxes
    return LinePlotModel(**traits)


@register_scene_writer(LinePlotModel)
def _line_plot_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    write_base_widget_data(model, element, model.klass)
    for box in model.boxes:
        elem = SubElement(element, NS_KARABO + 'box')
        elem.set("device", box.device)
        elem.set("path", box.path)
        elem.text = box.curve_object_data
    return element
