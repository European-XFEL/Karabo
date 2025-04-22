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
from enum import IntEnum
from xml.etree.ElementTree import SubElement

from traits.api import Float, Int, String

from karabo.common.savable import BaseSavableModel
from karabo.common.scenemodel.const import NS_KARABO
from karabo.common.scenemodel.io_utils import read_base_widget_data

KARABO_NoROI = 0
KARABO_RECT = 1
KARABO_CROSSHAIR = 2

KARABO_BASE_SAVABLE = ["x", "y", "width", "height", "keys", "parent_component"]

AXES_SET = ["x_grid", "y_grid", "x_log", "y_log", "x_invert", "y_invert"]
LABEL_UNITS = ["x_label", "y_label", "x_units", "y_units"]
BASELINE = ["offset", "step"]
VIEW_SET = ["background", "title"]
RANGE_SET = ["x_min", "x_max", "y_min", "y_max"]
TRANSFORM_SET = ["x_scale", "y_scale", "x_translate", "y_translate"]

KARABO_ROI_ITEMS = "roi_items"
KARABO_ROI_TYPE = "roi_type"
KARABO_CURVE_OPTIONS = "curve_options"


class PlotType(IntEnum):
    Curve = 1


class BaseROIData(BaseSavableModel):
    """The BaseROI Data Model being ``Savable`` and conering the ``type``"""

    roi_type = Int()
    name = String


class CrossROIData(BaseROIData):
    """The CrossROIData Model for CrossHair markers"""

    x = Float()
    y = Float()

    @property
    def coords(self):
        return self.x, self.y


class RectROIData(BaseROIData):
    """The RectROIData Model for rectangular regions"""

    x = Float()
    y = Float()
    w = Float()
    h = Float()

    @property
    def coords(self):
        return self.x, self.y, self.w, self.h


class BaseCurveOptions(BaseSavableModel):
    key = String()
    legend_name = String()
    plot_type = Int()

    def _legend_name_default(self):
        return self.key


class CurveOptions(BaseCurveOptions):
    pen_color = String()


def read_base_karabo_image_model(element):
    traits = read_base_widget_data(element)

    traits["aux_plots"] = int(element.get(NS_KARABO + "aux_plots", "0"))
    traits["colormap"] = element.get(NS_KARABO + "colormap", "viridis")
    traits["roi_tool"] = int(element.get(NS_KARABO + "roi_tool", 0))
    traits["roi_items"] = read_roi_info(element)
    traits["aspect_ratio"] = int(element.get(NS_KARABO + "aspect_ratio", 1))
    traits["color_levels"] = read_color_levels(element)
    show_scale = element.get(NS_KARABO + "show_scale", "1")
    traits["show_scale"] = bool(int(show_scale))

    traits.update(read_transforms(element))
    traits.update(read_basic_label(element))

    return traits


def write_base_karabo_image_model(model, element):
    element.set(NS_KARABO + "colormap", model.colormap)
    element.set(NS_KARABO + "aux_plots", str(model.aux_plots))
    element.set(NS_KARABO + "roi_tool", str(model.roi_tool))
    element.set(NS_KARABO + "aspect_ratio", str(model.aspect_ratio))
    write_color_levels(model, element)
    show_scale = str(int(model.show_scale))
    element.set(NS_KARABO + "show_scale", show_scale)

    # Save ROI configuration
    write_roi_info(model, element)
    write_transforms(model, element)
    write_basic_label(model, element)


def read_roi_info(element):
    """Extracts the ROIData list from the element XML"""
    roi_data = []

    for child_elem in element:
        if child_elem.tag != NS_KARABO + "roi":
            continue

        roi_type = int(child_elem.get("roi_type", 0))
        name = child_elem.get("name", "")
        traits = {"roi_type": roi_type, "name": name}
        if roi_type == 1:  # Rect:
            traits["x"] = float(child_elem.get("x"))
            traits["y"] = float(child_elem.get("y"))
            traits["w"] = float(child_elem.get("w"))
            traits["h"] = float(child_elem.get("h"))
            roi_data.append(RectROIData(**traits))
        elif roi_type == 2:  # Crosshair:
            traits["x"] = float(child_elem.get("x"))
            traits["y"] = float(child_elem.get("y"))
            roi_data.append(CrossROIData(**traits))

    return roi_data


def write_roi_info(model, element):
    """Method responsible for serializing the ROIs information.

    Each model has an ROIModel, which contains the models for each of the
    available ROI classes. For each of these models, we have have a list of
    ROI items that were drawn.
    """
    for roi in model.roi_items:
        roi_element = SubElement(element, NS_KARABO + "roi")
        for attribute in roi.class_visible_traits():
            roi_element.set(attribute, str(getattr(roi, attribute)))


def read_transforms(element):
    traits = {}
    for name in TRANSFORM_SET:
        value = element.get(NS_KARABO + name)
        if value is not None:
            traits[name] = float(value)

    return traits


def write_transforms(model, element):
    for name in TRANSFORM_SET:
        element.set(NS_KARABO + name, str(getattr(model, name)))


def read_axes_set(element):
    return {
        name: element.get(NS_KARABO + name, "false").lower() == "true"
        for name in AXES_SET
    }


def write_axes_set(model, element):
    for name in AXES_SET:
        element.set(NS_KARABO + name, str(getattr(model, name)))


def read_basic_label(element):
    return {name: element.get(NS_KARABO + name, "") for name in LABEL_UNITS}


def write_basic_label(model, element):
    for name in LABEL_UNITS:
        element.set(NS_KARABO + name, getattr(model, name))


def read_baseline(element):
    traits = {}
    traits["offset"] = float(element.get(NS_KARABO + "offset", 0.0))
    traits["step"] = float(element.get(NS_KARABO + "step", 1.0))
    return traits


def write_baseline(model, element):
    for name in BASELINE:
        element.set(NS_KARABO + name, str(getattr(model, name)))


def read_view_set(element):
    traits = {}
    traits["title"] = element.get(NS_KARABO + "title", "")
    traits["background"] = element.get(NS_KARABO + "background", "transparent")
    return traits


def write_view_set(model, element):
    element.set(NS_KARABO + "title", str(model.title))
    element.set(NS_KARABO + "background", str(model.background))


def read_range_set(element):
    traits = {
        name: float(element.get(NS_KARABO + name, "0.0")) for name in RANGE_SET
    }
    traits["x_autorange"] = (
            element.get(NS_KARABO + "x_autorange", "true").lower() == "true"
    )
    traits["y_autorange"] = (
            element.get(NS_KARABO + "y_autorange", "true").lower() == "true"
    )
    return traits


def write_range_set(model, element):
    for name in RANGE_SET:
        element.set(NS_KARABO + name, str(getattr(model, name)))
    element.set(NS_KARABO + "x_autorange", str(model.x_autorange))
    element.set(NS_KARABO + "y_autorange", str(model.y_autorange))


def build_model_config(model):
    """Build a configuration dictionary from a model

    This function removes the base savable trait settings!
    """
    config = {}
    for name in model.copyable_trait_names():
        if name in KARABO_BASE_SAVABLE:
            continue
        config[name] = getattr(model, name)
    return config


def build_graph_config(model):
    """Build a graph configuration dictionary from a model

    This function removes the base savable trait settings!
    """
    config = {}
    for name in model.copyable_trait_names():
        if name in KARABO_BASE_SAVABLE:
            continue
        elif name == KARABO_ROI_ITEMS:
            roi_items = []
            roi_data = getattr(model, KARABO_ROI_ITEMS)
            for roi in roi_data:
                traits = {}
                for subname in roi.copyable_trait_names():
                    traits[subname] = getattr(roi, subname)
                roi_items.append(traits)
            config[KARABO_ROI_ITEMS] = roi_items
        elif name == KARABO_CURVE_OPTIONS:
            curve_options = getattr(model, KARABO_CURVE_OPTIONS)
            options = {}
            for option in curve_options:
                key = getattr(option, "key")
                traits = {}
                for subname in option.copyable_trait_names():
                    traits[subname] = getattr(option, subname)

                options[key] = traits
            config[KARABO_CURVE_OPTIONS] = options
        else:
            # No special case, just write!
            config[name] = getattr(model, name)

    return config


def restore_graph_config(config):
    """Restore a graph configuration from a dictionary

    This function makes sure that certain data can be written on the
    traits model!
    """
    roi_data = config.get(KARABO_ROI_ITEMS, None)
    if roi_data is not None and roi_data:
        roi_items = []
        for roi in roi_data:
            roi_type = roi[KARABO_ROI_TYPE]
            if roi_type == KARABO_RECT:
                roi_items.append(RectROIData(**roi))
            elif roi_type == KARABO_CROSSHAIR:
                roi_items.append(CrossROIData(**roi))
        config[KARABO_ROI_ITEMS] = roi_items

    data = config.get(KARABO_CURVE_OPTIONS, None)
    if data is not None:
        config[KARABO_CURVE_OPTIONS] = [CurveOptions(**options)
                                        for options in data]
    return config


def read_histogram_model(element):
    traits = {}
    traits.update(read_basic_label(element))
    traits.update(read_axes_set(element))
    traits.update(read_range_set(element))
    traits["bins"] = int(element.get(NS_KARABO + "bins", 10))
    auto = element.get(NS_KARABO + "auto", "true")
    traits["auto"] = auto.lower() == "true"
    traits["start"] = float(element.get(NS_KARABO + "start", 0.0))
    traits["stop"] = float(element.get(NS_KARABO + "stop", 0.0))

    return traits


def write_histogram_model(model, element):
    write_basic_label(model, element)
    write_axes_set(model, element)
    write_range_set(model, element)
    element.set(NS_KARABO + "bins", str(model.bins))
    element.set(NS_KARABO + "start", str(model.start))
    element.set(NS_KARABO + "stop", str(model.stop))
    element.set(NS_KARABO + "auto", str(model.auto))

    return element


def read_curve_options(element) -> list:
    curve_options = [_read_options(child_elem) for child_elem in element if
                     child_elem.tag == NS_KARABO + KARABO_CURVE_OPTIONS]
    return curve_options


def write_curve_options(model, element):
    if not model.curve_options:
        return
    for options in model.curve_options:
        options_element = SubElement(element, NS_KARABO + KARABO_CURVE_OPTIONS)
        for name in options.copyable_trait_names():
            options_element.set(name, str(getattr(options, name)))


def _read_options(element):
    key = element.get("key")
    plot_type = int(element.get("plot_type", 0))
    name = element.get("legend_name")
    traits = {"plot_type": plot_type, "legend_name": name, "key": key}
    if plot_type == PlotType.Curve:
        traits["pen_color"] = element.get("pen_color")
        return CurveOptions(**traits)


def write_color_levels(model, element):
    color_levels = ",".join(str(x) for x in model.color_levels)
    element.set(NS_KARABO + "color_levels", color_levels)


def read_color_levels(element):
    """Read gracefully color levels for the image graphs"""
    try:
        levels = element.get(NS_KARABO + "color_levels", "")
        return [float(x) for x in levels.split(",")] if levels else []
    except Exception:
        return []
