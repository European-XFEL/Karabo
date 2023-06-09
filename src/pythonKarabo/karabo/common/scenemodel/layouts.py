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

from traits.api import CInt, Enum, Instance, Int

from .bases import BaseLayoutData, BaseLayoutModel, BaseSceneObjectData
from .const import NS_KARABO, NS_SVG
from .io_utils import set_numbers
from .registry import (
    read_element, register_scene_reader, register_scene_writer, write_element)

FIXED_LAYOUT_TAG = NS_SVG + "g"
FIXED_DATA_NAMES = ("x", "y", "height", "width")
GRID_DATA_NAMES = ("col", "colspan", "row", "rowspan")
LAYOUT_ATTRIBUTES = (
    ("x", NS_KARABO + "x"),
    ("y", NS_KARABO + "y"),
    ("height", NS_KARABO + "height"),
    ("width", NS_KARABO + "width"),
)


class BoxLayoutModel(BaseLayoutModel):
    """A layout which lays out its children either vertically or horizontally.
    """

    # The QBoxLayout::Direction value for this layout. An integer in [0, 3]
    direction = Enum(*list(range(4)))


class FixedLayoutModel(BaseLayoutModel):
    """A basic layout which merely groups objects together."""

    # XXX: What is this for?
    entire = Instance(BaseSceneObjectData)


class FixedLayoutChildData(BaseLayoutData):
    """Data for each child of a FixedLayoutModel"""

    # The X-coordinate of the child
    x = CInt
    # The Y-coordinate of the child
    y = CInt
    # The height of the child
    height = CInt
    # The width of the child
    width = CInt


class GridLayoutModel(BaseLayoutModel):
    """A layout which lays out its children in a grid."""


class GridLayoutChildData(BaseLayoutData):
    """Data for each child of a GridLayoutModel"""

    col = Int
    colspan = Int
    row = Int
    rowspan = Int


def _read_standard_layout_attributes(element):
    """Read standard layout attributes off of an element."""
    return {
        objname: float(element.get(xmlname))
        for objname, xmlname in LAYOUT_ATTRIBUTES
        if element.get(xmlname) is not None
    }


def _write_standard_layout_attributes(element, layout, layout_class_name):
    """Write the standard layout attributes to an element."""
    element.set(NS_KARABO + "class", layout_class_name)
    modelnames, xmlnames = zip(*LAYOUT_ATTRIBUTES)
    set_numbers(modelnames, layout, element, xmlnames=xmlnames)


def _read_fixed_layout_data(element):
    """Read layout child data specific to FixedLayoutModel.

    XXX: There is no guarantee that this data is actually present in a scene
    file! It might be a relic from the past that never got removed...
    """
    attrs = element.attrib
    ns = NS_KARABO if (NS_KARABO + "x") in attrs else ""
    traits = {
        name: float(attrs.get(ns + name))
        for name in FIXED_DATA_NAMES
        if (ns + name) in attrs
    }

    if len(traits) == 0:
        return None
    return FixedLayoutChildData(**traits)


def _read_grid_layout_data(element):
    """Read layout child data specific to GridLayoutModel"""
    traits = {
        name: int(element.get(NS_KARABO + name, "0"))
        for name in GRID_DATA_NAMES
    }
    return GridLayoutChildData(**traits)


def _write_grid_layout_data(element, layout_data):
    """Write layout child data specific to GridLayoutModel"""
    for name in GRID_DATA_NAMES:
        data = getattr(layout_data, name)
        element.set(NS_KARABO + name, str(data))


@register_scene_reader("BoxLayout", version=1)
def __box_layout_reader(element):
    """Read a BoxLayoutModel from a Version 1 format scene file."""
    traits = _read_standard_layout_attributes(element)
    traits["direction"] = int(element.get(NS_KARABO + "direction", "0"))
    layout = BoxLayoutModel(**traits)

    for child_elem in element:
        layout.children.append(read_element(child_elem))

    return layout


@register_scene_writer(BoxLayoutModel)
def __box_layout_writer(layout, root):
    """Write a BoxLayout to a scene file."""
    element = SubElement(root, FIXED_LAYOUT_TAG)
    element.set(NS_KARABO + "direction", str(layout.direction))
    _write_standard_layout_attributes(element, layout, "BoxLayout")

    for child in layout.children:
        write_element(model=child, parent=element)

    return element


@register_scene_reader("FixedLayout", xmltag=FIXED_LAYOUT_TAG, version=1)
def __fixed_layout_reader(element):
    """Read a FixedLayout from a Version 1 format scene file."""
    traits = _read_standard_layout_attributes(element)
    layout = FixedLayoutModel(**traits)

    for child_elem in element:
        child = read_element(child_elem)
        # XXX: Do to version 1 design quirks, some layout data is stored on
        # child elements.
        child.layout_data = _read_fixed_layout_data(child_elem)
        # XXX: Look for the strange 'entire' attribute on the child
        if child_elem.get(NS_KARABO + "entire") is not None:
            layout.entire = child
        layout.children.append(child)

    return layout


@register_scene_writer(FixedLayoutModel)
def __fixed_layout_writer(layout, root):
    """Write a FixedLayout to a scene file."""
    element = SubElement(root, FIXED_LAYOUT_TAG)
    _write_standard_layout_attributes(element, layout, "FixedLayout")

    for child in layout.children:
        child_elem = write_element(model=child, parent=element)
        # XXX: Handle the goofy 'entire' attribute
        if layout.entire is child:
            child_elem.set("entire", "True")

    return element


@register_scene_reader("GridLayout", version=1)
def __grid_layout_reader(element):
    """Read a Layout from a Version 1 format scene file."""
    traits = _read_standard_layout_attributes(element)
    layout = GridLayoutModel(**traits)

    for child_elem in element:
        child = read_element(child_elem)
        # XXX: Do to version 1 design quirks, some layout data is stored on
        # child elements.
        child.layout_data = _read_grid_layout_data(child_elem)
        layout.children.append(child)

    return layout


@register_scene_writer(GridLayoutModel)
def __grid_layout_writer(layout, root):
    """Write a GridLayout to a scene file."""
    element = SubElement(root, FIXED_LAYOUT_TAG)
    _write_standard_layout_attributes(element, layout, "GridLayout")

    for child in layout.children:
        child_elem = write_element(model=child, parent=element)
        # XXX: Handle the misplaced layout data
        _write_grid_layout_data(child_elem, child.layout_data)

    return element
