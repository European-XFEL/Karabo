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

from traits.api import Any, CInt, Dict, Float, Instance, List, Property, String

from .bases import BaseSceneObjectData, BaseShapeObjectData, XMLElementModel
from .const import ARROW_HEAD, NS_SVG
from .io_utils import (
    convert_number_or_string, get_defs_id, get_numbers, is_empty, set_numbers)
from .registry import (
    find_def, read_element, register_scene_reader, register_scene_writer,
    write_element)


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


class MarkerModel(XMLElementModel):
    markerHeight = Float
    markerWidth = Float
    markerUnits = String("strokeWidth")
    preserveAspectRatio = String
    orient = Any("auto")  # can be string or number
    refX = Any  # can be string or number
    refY = Any  # can be string or number

    children = List(BaseSceneObjectData)

    def generate_id(self):
        return self.randomize("marker")


class ArrowModel(LineModel):

    marker = Instance(MarkerModel)

    def _marker_default(self):
        arrow_path = PathModel(svg_data=ARROW_HEAD["path"], fill=self.stroke)
        return MarkerModel(
            markerHeight=ARROW_HEAD["height"],
            markerWidth=ARROW_HEAD["width"],
            refX=ARROW_HEAD["ref_x"],
            refY=ARROW_HEAD["ref_y"],
            children=[arrow_path],
        )

    def _stroke_changed(self, stroke):
        arrow_path = self.marker.children[0]
        arrow_path.fill = stroke


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


class XMLDefsModel(XMLElementModel):
    """<defs>"""

    # The element attributes aside from 'id'
    attributes = Dict
    # The element's children
    children = List(Instance(XMLElementModel))


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


@register_scene_reader("Line", xmltag=NS_SVG + "line", version=1)
def __line_reader(element):
    """A reader for Line objects in Version 1 scenes"""
    # # Check if it is an arrow:
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
    """A reader for Line objects in Version 1 scenes"""
    traits = _read_base_shape_data(element)
    traits.update(get_numbers(("x1", "y1", "x2", "y2"), element))

    # Retrieve marker model from the registry
    id_ = get_defs_id(element.get("marker-end"))
    traits["marker"] = find_def(id_)

    return ArrowModel(**traits)


@register_scene_writer(ArrowModel)
def __arrow_writer(model, parent):
    """A writer for LineModel objects"""
    # Write the defs model first, it's the same level as the arrow
    defs_element = SubElement(parent, NS_SVG + "defs")
    write_defs_model(XMLDefsModel(children=[model.marker]), defs_element)

    element = SubElement(parent, NS_SVG + "line")
    _write_base_shape_data(model, element)
    set_numbers(("x1", "y1", "x2", "y2"), model, element)

    # Write marker-end attribute
    element.set("marker-end", f"url(#{model.marker.id})")

    return element


@register_scene_reader("Marker", xmltag=NS_SVG + "marker", version=1)
def __marker_reader(element):
    """A reader for Markers"""

    d = element.attrib.copy()

    # Read all the trait values
    converters = {
        "id": str,
        "markerHeight": float,
        "markerUnits": str,
        "markerWidth": float,
        "preserveAspectRatio": str,
        "orient": convert_number_or_string,
        "refX": convert_number_or_string,
        "refY": convert_number_or_string,
    }

    traits = {
        name: converters[name](d[name]) for name in converters if name in d
    }

    # Add children definitions
    traits["children"] = [read_element(el) for el in element]

    return MarkerModel(**traits)


@register_scene_writer(MarkerModel)
def __marker_writer(model, parent):
    """A writer for PathModel objects"""
    element = SubElement(parent, NS_SVG + "marker")
    attribs = (
        "id",
        "markerHeight",
        "markerUnits",
        "markerWidth",
        "preserveAspectRatio",
        "orient",
        "refX",
        "refY",
    )

    # Write attributes
    for attr, value in model.trait_get(attribs).items():
        if not is_empty(value):
            element.set(attr, str(value))

    # Write children elements
    for child in model.children:
        write_element(child, element)
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


@register_scene_reader("XML Defs", xmltag=NS_SVG + "defs")
def __xml_defs_reader(element):
    return XMLDefsModel(
        id=element.get("id", ""),
        attributes={k: v for k, v in element.attrib.items() if k != "id"},
        children=[read_element(el) for el in element],
    )


# Writer is not registered as we write the defs on the shape writer
def write_defs_model(model, element):
    assert isinstance(model, XMLDefsModel)
    if model.id:
        element.set("id", model.id)
    for name, value in model.attributes.items():
        element.set(name, value)
    for child in model.children:
        write_element(model=child, parent=element)

    return element
