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
import warnings
from xml.etree.ElementTree import SubElement

from traits.api import Bool, Enum, Float, Instance, Int, List, String

from karabo.common.scenemodel.bases import BaseWidgetObjectData
from karabo.common.scenemodel.const import NS_KARABO, WIDGET_ELEMENT_TAG
from karabo.common.scenemodel.io_utils import (
    read_base_widget_data, write_base_widget_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)

from .graph_utils import (
    BaseROIData, read_axes_set, read_baseline, read_basic_label,
    read_histogram_model, read_range_set, read_roi_info, read_view_set,
    write_axes_set, write_baseline, write_basic_label, write_histogram_model,
    write_range_set, write_roi_info, write_view_set)


def write_base_plot(model, element, klass):
    """Write the base of all graph plots

    This method writes axes, labels, ranges and view (background, title) to
    the element.
    """
    write_base_widget_data(model, element, klass)
    write_basic_label(model, element)
    write_axes_set(model, element)
    write_range_set(model, element)
    write_view_set(model, element)


def read_base_plot(element):
    """Read the base of all graph plots"""
    traits = read_base_widget_data(element)
    traits.update(read_basic_label(element))
    traits.update(read_axes_set(element))
    traits.update(read_range_set(element))
    traits.update(read_view_set(element))
    return traits


class BasePlotModel(BaseWidgetObjectData):
    """A base model for the plot graphs"""

    title = String
    background = String("transparent")
    x_label = String
    y_label = String
    x_units = String
    y_units = String
    x_autorange = Bool(True)
    y_autorange = Bool(True)
    x_grid = Bool(False)
    y_grid = Bool(False)
    x_log = Bool(False)
    y_log = Bool(False)
    x_invert = Bool(False)
    y_invert = Bool(False)
    x_min = Float(0.0)
    x_max = Float(0.0)
    y_min = Float(0.0)
    y_max = Float(0.0)


class ScatterGraphModel(BasePlotModel):
    """A model for the Scatter Graph"""

    maxlen = Int(100)
    psize = Float(7.0)


class XYPlotModel(ScatterGraphModel):
    """a legacy model"""

    def __init__(self, **traits):
        super().__init__(**traits)
        msg = f"{type(self).__name__} is deprecated, use ScatterGraphModel"
        warnings.warn(msg, DeprecationWarning)


class MultiCurveGraphModel(BasePlotModel):
    """A model for the MultiCurve Graph"""


class MultiCurvePlotModel(MultiCurveGraphModel):
    """a legacy model"""

    def __init__(self, **traits):
        super().__init__(**traits)
        msg = f"{type(self).__name__} is deprecated, use MultiCurveGraphModel"
        warnings.warn(msg, DeprecationWarning)


class VectorScatterGraphModel(BasePlotModel):
    """A model for the VectorScatter Graph"""

    psize = Float(7.0)


class VectorXYGraphModel(BasePlotModel):
    """A model for the VectorXYGraph"""

    x_grid = Bool(True)
    y_grid = Bool(True)


class XYVectorModel(VectorXYGraphModel):
    """a legacy model"""

    def __init__(self, **traits):
        super().__init__(**traits)
        msg = f"{type(self)} is deprecate, use VectorXYGraphModel"
        warnings.warn(msg, DeprecationWarning)


class VectorBarGraphModel(BasePlotModel):
    """A model for the Vector Bar Graph"""

    bar_width = Float(0.1)


class NDArrayGraphModel(BasePlotModel):
    """A model for the NDArray Graph"""

    half_samples = Int(6000)
    roi_items = List(Instance(BaseROIData))
    roi_tool = Int(0)
    offset = Float(0.0)
    step = Float(1.0)
    x_grid = Bool(True)
    y_grid = Bool(True)


class VectorHistGraphModel(BasePlotModel):
    """A base model for histograms"""

    bins = Int(10)
    auto = Bool(True)
    start = Float(0.0)
    stop = Float(0.0)


class VectorFillGraphModel(BasePlotModel):
    """A model for the Vector Fill Graph"""


class VectorGraphModel(BasePlotModel):
    """A model for the Vector Graph"""

    half_samples = Int(6000)
    roi_items = List(Instance(BaseROIData))
    roi_tool = Int(0)
    offset = Float(0.0)
    step = Float(1.0)
    x_grid = Bool(True)
    y_grid = Bool(True)


class DisplayPlotModel(VectorGraphModel):
    """a legacy model"""

    def __init__(self, **traits):
        super().__init__(**traits)
        msg = f"{type(self).__name__} is deprecated, use VectorGraphModel"
        warnings.warn(msg, DeprecationWarning)


class TrendGraphModel(BasePlotModel):
    """Trendline graph model"""

    x_grid = Bool(True)
    y_grid = Bool(True)


class DisplayTrendlineModel(TrendGraphModel):
    """a legacy model"""

    def __init__(self, **traits):
        super().__init__(**traits)
        msg = f"{type(self).__name__} is deprecated, use TrendGraphModel"
        warnings.warn(msg, DeprecationWarning)


class LinePlotModel(DisplayTrendlineModel):
    """A legacy model for line plot objects"""

    # Note: LinePlotModel is a class model but was never serialized
    # under its own name. It was the ghost writer of DisplayTrendline.
    # The original model had a trait `boxes`. and other functions.
    # this are removed. It should not have been used in device code.
    # if they were used, the code should fail.

    klass = Enum("DisplayTrendline", "XYVector")


class StateGraphModel(BasePlotModel):
    """State graph model"""

    x_grid = Bool(True)
    y_grid = Bool(True)


class AlarmGraphModel(BasePlotModel):
    """Alarm graph model"""

    x_grid = Bool(True)
    y_grid = Bool(True)


@register_scene_reader("XYPlot")  # deprecated matplotlib model
@register_scene_reader("ScatterGraph")
def _scatter_graph_reader(element):
    traits = read_base_plot(element)
    traits["maxlen"] = int(element.get(NS_KARABO + "maxlen", 100))
    traits["psize"] = float(element.get(NS_KARABO + "psize", 7))

    return ScatterGraphModel(**traits)


@register_scene_writer(XYPlotModel)
@register_scene_writer(ScatterGraphModel)
def _scatter_graph_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_plot(model, element, "ScatterGraph")
    element.set(NS_KARABO + "maxlen", str(model.maxlen))
    element.set(NS_KARABO + "psize", str(model.psize))

    return element


@register_scene_reader("VectorScatterGraph")
def _vector_scatter_graph_reader(element):
    traits = read_base_plot(element)
    traits["psize"] = float(element.get(NS_KARABO + "psize", 7))

    return VectorScatterGraphModel(**traits)


@register_scene_writer(VectorScatterGraphModel)
def _vector_scatter_graph_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_plot(model, element, "VectorScatterGraph")
    element.set(NS_KARABO + "psize", str(model.psize))

    return element


@register_scene_reader("XYVector")  # deprecated Qwt model
@register_scene_reader("VectorXYGraph")
def _vector_xy_graph_reader(element):
    traits = read_base_plot(element)

    return VectorXYGraphModel(**traits)


@register_scene_writer(XYVectorModel)
@register_scene_writer(VectorXYGraphModel)
def _vector_xy_graph_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_plot(model, element, "VectorXYGraph")

    return element


@register_scene_reader("VectorBarGraph")
def _vector_bar_graph_reader(element):
    traits = read_base_plot(element)
    traits["bar_width"] = float(element.get(NS_KARABO + "bar_width", 0.1))

    return VectorBarGraphModel(**traits)


@register_scene_writer(VectorBarGraphModel)
def _vector_bar_graph_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_plot(model, element, "VectorBarGraph")
    element.set(NS_KARABO + "bar_width", str(model.bar_width))

    return element


@register_scene_reader("VectorHistGraph")
def _vector_hist_graph_reader(element):
    traits = read_base_widget_data(element)
    traits.update(read_histogram_model(element))
    return VectorHistGraphModel(**traits)


@register_scene_writer(VectorHistGraphModel)
def _vector_hist_graph_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "VectorHistGraph")
    write_histogram_model(model, element)

    return element


@register_scene_reader("NDArrayGraph")
def _ndarray_graph_reader(element):
    traits = read_base_plot(element)
    traits.update(read_baseline(element))
    traits["roi_items"] = read_roi_info(element)
    traits["roi_tool"] = int(element.get(NS_KARABO + "roi_tool", 0))
    traits["half_samples"] = int(element.get(NS_KARABO + "half_samples", 6000))

    return NDArrayGraphModel(**traits)


@register_scene_writer(NDArrayGraphModel)
def _ndarray_graph_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_plot(model, element, "NDArrayGraph")
    write_roi_info(model, element)
    write_baseline(model, element)
    element.set(NS_KARABO + "roi_tool", str(model.roi_tool))
    element.set(NS_KARABO + "half_samples", str(model.half_samples))

    return element


@register_scene_reader("VectorFillGraph")
def _vector_fill_graph_reader(element):
    traits = read_base_plot(element)

    return VectorFillGraphModel(**traits)


@register_scene_writer(VectorFillGraphModel)
def _vector_fill_graph_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_plot(model, element, "VectorFillGraph")

    return element


@register_scene_reader("DisplayPlot")  # deprecated Qwt model
@register_scene_reader("VectorGraph")
def _vector_graph_reader(element):
    traits = read_base_plot(element)
    traits.update(read_baseline(element))
    traits["roi_items"] = read_roi_info(element)
    traits["roi_tool"] = int(element.get(NS_KARABO + "roi_tool", 0))
    traits["half_samples"] = int(element.get(NS_KARABO + "half_samples", 6000))

    return VectorGraphModel(**traits)


@register_scene_writer(DisplayPlotModel)
@register_scene_writer(VectorGraphModel)
def _vector_graph_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_plot(model, element, "VectorGraph")
    write_roi_info(model, element)
    write_baseline(model, element)
    element.set(NS_KARABO + "roi_tool", str(model.roi_tool))
    element.set(NS_KARABO + "half_samples", str(model.half_samples))

    return element


@register_scene_reader("DisplayTrendline")  # deprecated Qwt model
@register_scene_reader("DisplayTrendGraph")
def _trend_graph_reader(element):
    traits = read_base_plot(element)

    return TrendGraphModel(**traits)


@register_scene_writer(DisplayTrendlineModel)  # deprecated Qwt model
@register_scene_writer(TrendGraphModel)
def _trend_graph_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_plot(model, element, "DisplayTrendGraph")

    return element


@register_scene_writer(LinePlotModel)  # deprecated Qwt model
def _line_plot_writer(model, parent):
    # Delegate the co writer to the respective new models
    WRITER_MAP = {
        "DisplayTrendline": _trend_graph_writer,
        "XYVector": _vector_xy_graph_writer,
    }

    # The model will always have a `klass` due to its Enum traits
    writer = WRITER_MAP.get(model.klass)
    return writer(model, parent)


@register_scene_reader("DisplayStateGraph")
def _state_graph_reader(element):
    traits = read_base_plot(element)

    return StateGraphModel(**traits)


@register_scene_writer(StateGraphModel)
def _state_graph_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_plot(model, element, "DisplayStateGraph")

    return element


@register_scene_reader("DisplayAlarmGraph")
def _alarm_graph_reader(element):
    traits = read_base_plot(element)

    return AlarmGraphModel(**traits)


@register_scene_writer(AlarmGraphModel)
def _alarm_graph_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_plot(model, element, "DisplayAlarmGraph")

    return element


@register_scene_reader("MultiCurvePlot")  # deprecated matplotlib model
@register_scene_reader("MultiCurveGraph")
def _multi_graph_reader(element):
    traits = read_base_plot(element)

    return MultiCurveGraphModel(**traits)


@register_scene_writer(MultiCurvePlotModel)
@register_scene_writer(MultiCurveGraphModel)
def _multi_graph_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_plot(model, element, "MultiCurveGraph")

    return element
