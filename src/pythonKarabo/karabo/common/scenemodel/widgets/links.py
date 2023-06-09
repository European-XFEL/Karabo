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

from traits.trait_types import Enum, Int, String

from karabo.common.scenemodel.bases import BaseWidgetObjectData
from karabo.common.scenemodel.const import (
    NS_KARABO, WIDGET_ELEMENT_TAG, SceneTargetWindow)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)
from karabo.common.scenemodel.widgets.simple import (
    _read_geometry_data, _write_class_and_geometry)


class BaseLinkModel(BaseWidgetObjectData):
    # The target of the link widget
    target = String
    # The text to be displayed
    text = String
    # A string describing the font
    font = String
    # A foreground color, CSS-style
    foreground = String
    # A background color, CSS-style
    background = String("transparent")
    # The line width of a frame around the text
    frame_width = Int(1)


class DeviceSceneLinkModel(BaseLinkModel):
    """A model for a DeviceSceneLink Widget"""
    # Deprecated ...
    target_window = Enum(*list(SceneTargetWindow))


class SceneLinkModel(BaseLinkModel):
    """A model for a scene link"""
    target_window = Enum(*list(SceneTargetWindow))


class WebLinkModel(BaseLinkModel):
    """A model for the weblink widget"""


def read_base_link(element):
    """Read the base link information from the element"""
    traits = {}
    traits["target"] = element.get(NS_KARABO + "target")
    traits["text"] = element.get(NS_KARABO + "text", "")
    traits["font"] = element.get(NS_KARABO + "font", "")
    traits["foreground"] = element.get(NS_KARABO + "foreground", "black")
    background = element.get(NS_KARABO + "background", "transparent")
    traits["background"] = background
    frame_width = element.get(NS_KARABO + "frameWidth", 1)
    traits["frame_width"] = int(frame_width)
    return traits


def write_base_link(model, element, klass):
    """Write the common base link information"""
    _write_class_and_geometry(model, element, klass)
    for name in ("background", "text", "font", "foreground", "target"):
        element.set(NS_KARABO + name, getattr(model, name))
    element.set(NS_KARABO + "frameWidth", str(model.frame_width))


@register_scene_reader("DeviceSceneLink")
def __device_scene_link_reader(element):
    traits = _read_geometry_data(element)
    traits.update(read_base_link(element))
    keys = element.get(NS_KARABO + "keys", "")
    if len(keys) > 0:
        traits["keys"] = keys.split(",")
    traits["target_window"] = SceneTargetWindow.Dialog
    return DeviceSceneLinkModel(**traits)


@register_scene_writer(DeviceSceneLinkModel)
def __device_scene_link_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_link(model, element, "DeviceSceneLink")
    element.set(NS_KARABO + "keys", ",".join(model.keys))
    # XXX: removal around Karabo 2.18.X
    element.set(NS_KARABO + "target_window", SceneTargetWindow.Dialog.value)
    return element


@register_scene_reader("WebLink", version=1)
def __web_link_reader(element):
    traits = _read_geometry_data(element)
    traits.update(read_base_link(element))

    return WebLinkModel(**traits)


@register_scene_writer(WebLinkModel)
def __web_link_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_link(model, element, "WebLink")
    return element


@register_scene_reader("SceneLink", version=1)
def __scene_link_reader(element):
    traits = _read_geometry_data(element)
    traits.update(read_base_link(element))
    # If unspecified, the default is 'dialog'
    target_window = element.get(NS_KARABO + "target_window", "dialog")
    traits["target_window"] = SceneTargetWindow(target_window)

    return SceneLinkModel(**traits)


@register_scene_writer(SceneLinkModel)
def __scene_link_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_link(model, element, "SceneLink")
    element.set(NS_KARABO + "target_window", model.target_window.value)

    return element
