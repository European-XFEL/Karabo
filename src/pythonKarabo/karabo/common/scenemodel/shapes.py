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

from traits.api import CInt, Property, String

from .bases import BaseShapeObjectData
from .const import ARROW_MIN_SIZE, NS_KARABO, NS_SVG, SVG_GROUP_TAG
from .io_utils import get_numbers, set_numbers
from .registry import register_scene_reader, register_scene_writer


class LineModel(BaseShapeObjectData):
    """A line which can appear in a scene"""

    # The X-coordinate of the first point
    x1 = CInt
    # The Y-coordinate of the first point
    y1 = CInt
    # The X-coordinate of the second point
    x2 = CInt
    # The Y-coordinate of the second point
    y2 = CInt

    # Add x and y to follow the generic object interface
    x = Property(CInt)
    y = Property(CInt)

    def _get_x(self):
        return min([self.x1, self.x2])

    def _set_x(self, x):
        offset = x - self.x
        self.x1 += offset
        self.x2 += offset

    def _get_y(self):
        return min([self.y1, self.y2])

    def _set_y(self, y):
        offset = y - self.y
        self.y1 += offset
        self.y2 += offset


class PathModel(BaseShapeObjectData):
    """An arbitrary path object for scenes"""

    # A blob of SVG data...
    svg_data = String


class ArrowPolygonModel(LineModel):
    """Model for Arrow with polygon head"""
    # Points to draw the arrowhead as polygon. The third point in x2, y2.
    hx1 = CInt
    hy1 = CInt
    hx2 = CInt
    hy2 = CInt

    width = Property(CInt)
    height = Property(CInt)

    def _get_width(self):
        return max(abs(self.x1 - self.x2), ARROW_MIN_SIZE)

    def _get_height(self):
        return max(abs(self.y1 - self.y2), ARROW_MIN_SIZE)


class RectangleModel(BaseShapeObjectData):
    """A rectangle which can appear in a scene"""

    # The X-coordinate of the rect
    x = CInt
    # The Y-coordinate of the rect
    y = CInt
    # The height of the rect
    height = CInt
    # The width of the rect
    width = CInt


def _convert_measurement(measure):
    """Convert a measurement value to pixels"""
    scales = {
        "px": 1,
        "pt": 1.25,
        "pc": 15,
        "mm": 3.543307,
        "cm": 35.43307,
        "in": 90,
    }
    scale = scales.get(measure[-2:])
    if scale is not None:
        return scale * float(measure[:-2])
    else:
        return float(measure)


def _read_base_shape_data(element):
    """Read the style attributes common to all "shape" elements"""
    # Break up a style attribute if that's where the style info is at.
    d = element.attrib.copy()
    if "style" in d:
        d.update(s.split(":") for s in d["style"].split(";"))

    # Read all the trait values
    converters = {
        "stroke": str,
        "stroke-opacity": float,
        "stroke-linecap": str,
        "stroke-dashoffset": _convert_measurement,
        "stroke-width": _convert_measurement,
        "stroke-dasharray": str,
        "stroke-style": int,
        "stroke-linejoin": str,
        "stroke-miterlimit": float,
        "fill": str,
        "fill-opacity": float,
    }
    traits = {
        name.replace("-", "_"): converters[name](d[name])
        for name in converters
        if name in d
    }

    # Replicate behavior of old (pre-1.5) scene view
    if "fill" not in traits:
        traits["fill"] = "black"

    # Convert the dash array to a proper value
    if "stroke_dasharray" in traits:
        dashes = traits.pop("stroke_dasharray")
        if dashes.lower() != "none":
            dashlist = dashes.split(",") if "," in dashes else dashes.split()
            penwidth = traits.get("stroke_width", 1.0)
            traits["stroke_dasharray"] = [
                _convert_measurement(d) / penwidth for d in dashlist
            ]

    return traits


def _write_base_shape_data(model, element):
    """Write out the style attributes common to all "shape" elements"""
    write = element.set

    write("stroke", model.stroke)
    if model.stroke != "none":
        write("stroke-opacity", str(model.stroke_opacity))
        write("stroke-linecap", model.stroke_linecap)
        write("stroke-dashoffset", str(model.stroke_dashoffset))
        write("stroke-width", str(model.stroke_width))
        write(
            "stroke-dasharray",
            " ".join(
                str(x * model.stroke_width) for x in model.stroke_dasharray
            ),
        )
        write("stroke-style", str(model.stroke_style))
        write("stroke-linejoin", model.stroke_linejoin)
        write("stroke-miterlimit", str(model.stroke_miterlimit))

    write("fill", model.fill)
    if model.fill != "none":
        write("fill-opacity", str(model.fill_opacity))


def _write_arrow_data(model, element):
    _write_base_shape_data(model, element)
    set_numbers(("x", "y", "width", "height", "x1", "y1", "x2", "y2", "hx1",
                 "hy1", "hx2", "hy2", ), model, element)


@register_scene_reader("Line", xmltag=NS_SVG + "line", version=1)
def __line_reader(element):
    """A reader for Line objects in Version 1 scenes"""
    # Check if it is an older arrow with XML defs arrow head.
    if "marker-end" in element.attrib:
        return __arrow_reader(element)

    traits = _read_base_shape_data(element)
    traits.update(get_numbers(("x1", "y1", "x2", "y2"), element))
    return LineModel(**traits)


@register_scene_writer(LineModel)
def __line_writer(model, parent):
    """A writer for LineModel objects"""
    element = SubElement(parent, NS_SVG + "line")
    _write_base_shape_data(model, element)
    set_numbers(("x1", "y1", "x2", "y2"), model, element)
    return element


# Don't register because this is called from the line reader
def __arrow_reader(element):
    traits = _read_base_shape_data(element)
    traits.update(get_numbers(("x1", "y1", "x2", "y2"), element))
    return ArrowPolygonModel(**traits)


def __set_points(model, polygon_element):
    """ Sets the arrowhead points as string. eg 'x1,y1 x2,y2 x3,y3' """
    points = (f"{model.x2},{model.y2} "
              f"{model.hx1},{model.hy1} "
              f"{model.hx2},{model.hy2} ")
    polygon_element.set("points", points)


def __get_points(polygon_element):
    """Reads the polygon element's points . Point is stored as
    'x1,y1 x2,y2 x3,y3'  """
    points = polygon_element.get("points").strip()
    _, hp1, hp2 = points.split(" ")
    hx1, hy1 = hp1.split(",")
    hx2, hy2 = hp2.split(",")
    return {"hx1": hx1, "hy1": hy1, "hx2": hx2, "hy2": hy2}


@register_scene_reader("ArrowPolygonModel", xmltag=SVG_GROUP_TAG,
                       version=2)
def arrow_polygon_reader(element):
    line = element.find(NS_SVG + "line")
    polygon = element.find(NS_SVG + "polygon")
    # line and polygon are None if the model is written  by UnknownWidget,
    # from older KaraboGUI.
    if line is None:
        traits = _read_base_shape_data(element)
    else:
        traits = _read_base_shape_data(line)

    if polygon is None:
        hx1 = element.attrib["hx1"]
        hx2 = element.attrib["hx2"]
        hy1 = element.attrib["hy1"]
        hy2 = element.attrib["hy2"]
        points = {"hx1": hx1, "hy1": hy1, "hx2": hx2, "hy2": hy2}
    else:
        points = __get_points(polygon_element=polygon)
    traits.update(points)
    traits.update(get_numbers(("x", "y", "x1", "y1", "x2", "y2"), element))
    return ArrowPolygonModel(**traits)


@register_scene_writer(ArrowPolygonModel)
def arrow_polygon_writer(model, parent):
    element = SubElement(parent, NS_SVG + "g")
    element.set(NS_KARABO + "class", "ArrowPolygonModel")

    # Write line and polygon(arrowhead) as separate elements. This enables
    # the reading of the arrow in an SVG editor like Inkscape.
    line = SubElement(element, NS_SVG + "line")
    _write_base_shape_data(model, line)
    set_numbers(("x1", "y1", "x2", "y2"), model, line)

    polygon_element = SubElement(element, NS_SVG + "polygon")
    __set_points(model=model, polygon_element=polygon_element)
    model.fill = model.stroke
    # Write all the model values to element also, so that in KaraboGUI
    # older  than 2.21 the unsupported widget is shown.
    _write_arrow_data(model, element)
    _write_base_shape_data(model, polygon_element)
    return element


@register_scene_reader("Path", xmltag=NS_SVG + "path", version=1)
def __path_reader(element):
    """A reader for Path objects in Version 1 scenes"""
    traits = _read_base_shape_data(element)
    traits["svg_data"] = element.get("d")
    return PathModel(**traits)


@register_scene_writer(PathModel)
def __path_writer(model, parent):
    """A writer for PathModel objects"""
    element = SubElement(parent, NS_SVG + "path")
    _write_base_shape_data(model, element)
    element.set("d", model.svg_data)
    return element


@register_scene_reader("Rectangle", xmltag=NS_SVG + "rect", version=1)
def __rectangle_reader(element):
    """A reader for Rectangle objects in Version 1 scenes"""
    traits = _read_base_shape_data(element)
    traits.update(get_numbers(("x", "y", "width", "height"), element))
    return RectangleModel(**traits)


@register_scene_writer(RectangleModel)
def __rectangle_writer(model, parent):
    """A writer for RectangleModel objects"""
    element = SubElement(parent, NS_SVG + "rect")
    _write_base_shape_data(model, element)
    set_numbers(("x", "y", "width", "height"), model, element)
    return element
