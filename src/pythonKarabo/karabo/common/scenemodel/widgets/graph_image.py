# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
from xml.etree.ElementTree import SubElement

from traits.api import Bool, Float, Instance, Int, String
from traits.trait_types import List

from karabo.common.scenemodel.bases import BaseWidgetObjectData
from karabo.common.scenemodel.const import NS_KARABO, WIDGET_ELEMENT_TAG
from karabo.common.scenemodel.io_utils import (
    read_base_widget_data, write_base_widget_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)

from .graph_utils import (
    BaseROIData, read_base_karabo_image_model, read_basic_label,
    read_color_levels, read_roi_info, write_base_karabo_image_model,
    write_basic_label, write_color_levels, write_roi_info)


class KaraboImageModel(BaseWidgetObjectData):
    """The KaraboImageModel Base"""

    aux_plots = Int(0)
    colormap = String("none")

    roi_items = List(Instance(BaseROIData))
    roi_tool = Int(0)

    x_scale = Float(1.0)
    x_translate = Float(0.0)
    x_label = String("X-axis")
    x_units = String("pixels")

    y_scale = Float(1.0)
    y_translate = Float(0.0)
    y_label = String("Y-axis")
    y_units = String("pixels")

    show_scale = Bool(True)
    aspect_ratio = Int(1)
    color_levels = List(float)


class ImageGraphModel(KaraboImageModel):
    """A model of the Image Graph"""
    undock = Bool(False, transient=True)


class DetectorGraphModel(KaraboImageModel):
    """A model of the DetectorGraph"""


class VectorRollGraphModel(BaseWidgetObjectData):
    """A model of the VectorRoll Graph"""

    # Image trait base
    aux_plots = Int(0)
    colormap = String("none")
    roi_items = List(Instance(BaseROIData))
    roi_tool = Int(0)
    x_label = String("X-axis")
    x_units = String("pixels")
    y_label = String("Y-axis")
    y_units = String("pixels")
    # Extras
    maxlen = Int(100)
    color_levels = List(float)


class WebCamGraphModel(BaseWidgetObjectData):
    """A model of the WebCam Graph"""
    colormap = String("none")
    undock = Bool(False, transient=True)


@register_scene_reader("ImageGraph")
def _image_graph_reader(element):
    traits = read_base_karabo_image_model(element)

    return ImageGraphModel(**traits)


@register_scene_writer(ImageGraphModel)
def _image_graph_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "ImageGraph")
    write_base_karabo_image_model(model, element)

    return element


@register_scene_reader("DetectorGraph")
def _detector_graph_reader(element):
    traits = read_base_karabo_image_model(element)

    return DetectorGraphModel(**traits)


@register_scene_writer(DetectorGraphModel)
def _detector_graph_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "DetectorGraph")
    write_base_karabo_image_model(model, element)

    return element


@register_scene_reader("VectorRollGraph")
def _vector_roll_graph_reader(element):
    traits = read_base_widget_data(element)
    traits["aux_plots"] = int(element.get(NS_KARABO + "aux_plots", "0"))
    traits["colormap"] = element.get(NS_KARABO + "colormap", "viridis")
    traits["roi_tool"] = int(element.get(NS_KARABO + "roi_tool", 0))
    traits["roi_items"] = read_roi_info(element)
    traits.update(read_basic_label(element))
    traits["maxlen"] = int(element.get(NS_KARABO + "maxlen", 100))
    color_levels = read_color_levels(element)
    traits["color_levels"] = color_levels
    return VectorRollGraphModel(**traits)


@register_scene_writer(VectorRollGraphModel)
def _vector_roll_graph_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "VectorRollGraph")
    element.set(NS_KARABO + "colormap", model.colormap)
    element.set(NS_KARABO + "aux_plots", str(model.aux_plots))
    element.set(NS_KARABO + "roi_tool", str(model.roi_tool))
    # Save ROI configuration
    write_basic_label(model, element)
    write_roi_info(model, element)
    element.set(NS_KARABO + "maxlen", str(model.maxlen))
    write_color_levels(model, element)

    return element


@register_scene_reader("WebCamGraph")
def _webcam_graph_reader(element):
    traits = read_base_widget_data(element)
    traits["colormap"] = element.get(NS_KARABO + "colormap", "viridis")

    return WebCamGraphModel(**traits)


@register_scene_writer(WebCamGraphModel)
def _webcam_graph_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "WebCamGraph")
    element.set(NS_KARABO + "colormap", model.colormap)

    return element
