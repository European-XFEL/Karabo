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

from traits.api import CInt, Constant, Dict, Instance, Int, List, String

from karabo.common.api import BaseSavableModel, walk_traits_object
# avoid karabo.common.project.api due to circular imports...
from karabo.common.project.bases import BaseProjectObjectModel

from .bases import BaseSceneObjectData, BaseWidgetObjectData, XMLElementModel
from .const import (
    NS_KARABO, NS_SVG, SCENE_FILE_VERSION, SCENE_MIN_HEIGHT, SCENE_MIN_WIDTH,
    UNKNOWN_WIDGET_CLASS, WIDGET_ELEMENT_TAG)
from .io_utils import (
    read_unknown_display_editable_widget, set_numbers, write_base_widget_data)
from .registry import (
    add_temporary_defs, read_element, register_scene_reader,
    register_scene_writer, write_element)


class SceneModel(BaseProjectObjectModel):
    """An object representing the data for a Karabo GUI scene."""

    # The version of the file data (from the scene file)
    file_format_version = Int(SCENE_FILE_VERSION, transient=True)
    # Extra attributes from the SVG file that we want to preserve.
    extra_attributes = Dict(transient=True)
    # The width of the scene in pixels
    width = CInt(SCENE_MIN_WIDTH)
    # The height of the scene in pixels
    height = CInt(SCENE_MIN_HEIGHT)
    # All the objects in the scene
    children = List(Instance(BaseSceneObjectData))


class UnknownWidgetDataModel(BaseWidgetObjectData):
    """A model object for widgets from the future!

    As new widgets are added in the future, code which has not yet been
    upgraded needs to be able to read and write them without blowing up.
    """

    # The value of the `krb:widget` attribute
    klass = String
    # Attributes which are not part of `BaseWidgetObjectData`
    attributes = Dict
    # The data of the SVG element, if there is any
    data = String


class UnknownXMLDataModel(XMLElementModel):
    """A model object to hold SVG data that we don't understand."""

    # The xml tag
    tag = String
    # The element attributes aside from 'id'
    attributes = Dict
    # The element data
    data = String
    # The element's children
    children = List(Instance(BaseSceneObjectData))

    # XXX: These are needed by the GUI, but they will always be zero
    x = Constant(0)
    y = Constant(0)
    height = Constant(0)
    width = Constant(0)

    def generate_id(self):
        return self.randomize("unknown")


def _read_extra_attributes(element):
    """Read all the attributes that we don't explicitly write."""
    our_names = ("height", "width", NS_KARABO + "version", NS_KARABO + "uuid")
    attributes = {}
    for name, value in element.items():
        if name not in our_names:
            attributes[name] = value

    return attributes


@register_scene_reader("Scene", xmltag="svg", version=1)
@register_scene_reader("Scene", xmltag=NS_SVG + "svg", version=1)
def __scene_reader(element):
    traits = {
        "file_format_version": int(element.get(NS_KARABO + "version", 1)),
        "uuid": element.get(NS_KARABO + "uuid"),
        "width": float(element.get("width", 0)),
        "height": float(element.get("height", 0)),
        "extra_attributes": _read_extra_attributes(element),
    }
    # This attribute is not guaranteed to be there...
    if traits["uuid"] is None:
        del traits["uuid"]

    scene = SceneModel(**traits)

    # We get the first level defs first
    defs = []
    for d in element.findall(f"./{NS_SVG}defs"):
        defs.append(read_element(d))
        element.remove(d)
    # Record the defs on the reader registry
    add_temporary_defs(defs)

    # Now we iterate over the children elements
    for child in element:
        scene.children.append(read_element(child))

    def visitor(model):
        if isinstance(model, BaseSavableModel):
            model.initialized = True

    # Mark everything as initialized, otherwise `modified` won't work.
    walk_traits_object(scene, visitor)

    return scene


@register_scene_writer(SceneModel)
def __scene_writer(scene, root):
    for child in scene.children:
        write_element(model=child, parent=root)

    root.set(NS_KARABO + "uuid", scene.uuid)
    set_numbers(("height", "width"), scene, root)
    for name, value in scene.extra_attributes.items():
        root.set(name, value)

    return root


@register_scene_reader("Unknown", xmltag="*")
def __unknown_xml_data_reader(element):
    return UnknownXMLDataModel(
        tag=element.tag,
        id=element.get("id", ""),
        attributes={k: v for k, v in element.attrib.items() if k != "id"},
        data=element.text or "",
        children=[read_element(el) for el in element],
    )


@register_scene_writer(UnknownXMLDataModel)
def __unknown_xml_data_writer(model, parent):
    element = SubElement(parent, model.tag)
    if model.data:
        element.text = model.data
    if model.id:
        element.set("id", model.id)
    for name, value in model.attributes.items():
        element.set(name, value)
    for child in model.children:
        write_element(model=child, parent=element)

    return element


@register_scene_reader(UNKNOWN_WIDGET_CLASS)
def __unknown_widget_data_reader(element):
    traits = read_unknown_display_editable_widget(element)
    attributes = {k: element.get(k) for k in element.attrib if k not in traits}
    return UnknownWidgetDataModel(
        attributes=attributes, data=element.text or "", **traits
    )


@register_scene_writer(UnknownWidgetDataModel)
def __unknown_widget_data_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, model.klass)
    if model.data:
        element.text = model.data
    for name, value in model.attributes.items():
        element.set(name, value)
    return element
