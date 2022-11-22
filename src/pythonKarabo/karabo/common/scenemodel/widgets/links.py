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
    frame_width = Int(0)  # XXX: Make the default 1


class DeviceSceneLinkModel(BaseLinkModel):
    """A model for a DeviceSceneLink Widget"""
    frame_width = Int(1)
    # Deprecated ...
    target_window = Enum(*list(SceneTargetWindow))


class SceneLinkModel(BaseLinkModel):
    """A model for a scene link"""
    target_window = Enum(*list(SceneTargetWindow))


class WebLinkModel(BaseLinkModel):
    """A model for the weblink widget"""
    frame_width = Int(1)


@register_scene_reader("DeviceSceneLink")
def _device_scene_link_reader(element):
    traits = _read_geometry_data(element)
    keys = element.get(NS_KARABO + "keys", "")
    if len(keys) > 0:
        traits["keys"] = keys.split(",")
    # If unspecified, the default is 'scene`
    traits["target"] = element.get(NS_KARABO + "target", "")
    traits["text"] = element.get(NS_KARABO + "text", "")
    traits["font"] = element.get(NS_KARABO + "font")
    traits["foreground"] = element.get(NS_KARABO + "foreground", "black")
    bg = element.get(NS_KARABO + "background")
    if bg is not None:
        traits["background"] = bg
    fw = element.get(NS_KARABO + "frameWidth")
    if fw is not None:
        traits["frame_width"] = int(fw)
    traits["target_window"] = SceneTargetWindow.Dialog

    return DeviceSceneLinkModel(**traits)


@register_scene_writer(DeviceSceneLinkModel)
def _device_scene_link_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    _write_class_and_geometry(model, element, "DeviceSceneLink")
    element.set(NS_KARABO + "keys", ",".join(model.keys))
    for name in ("text", "font", "foreground"):
        element.set(NS_KARABO + name, getattr(model, name))

    element.set(NS_KARABO + "frameWidth", str(model.frame_width))
    if model.background != "":
        element.set(NS_KARABO + "background", model.background)
    element.set(NS_KARABO + "target", model.target)

    # XXX: removal around Karabo 2.18.X
    element.set(NS_KARABO + "target_window", SceneTargetWindow.Dialog.value)
    return element


@register_scene_reader("WebLink", version=1)
def __web_link_reader(element):
    traits = _read_geometry_data(element)
    traits["target"] = element.get(NS_KARABO + "target")
    traits["text"] = element.get(NS_KARABO + "text", "")
    traits["font"] = element.get(NS_KARABO + "font", "")
    traits["foreground"] = element.get(NS_KARABO + "foreground", "black")

    bg = element.get(NS_KARABO + "background")
    if bg is not None:
        traits["background"] = bg
    fw = element.get(NS_KARABO + "frameWidth")
    if fw is not None:
        traits["frame_width"] = int(fw)

    return WebLinkModel(**traits)


@register_scene_writer(WebLinkModel)
def __web_link_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    _write_class_and_geometry(model, element, "WebLink")
    element.set(NS_KARABO + "target", model.target)

    for name in ("text", "font", "foreground"):
        element.set(NS_KARABO + name, getattr(model, name))

    element.set(NS_KARABO + "frameWidth", str(model.frame_width))
    if model.background != "":
        element.set(NS_KARABO + "background", model.background)

    return element


@register_scene_reader("SceneLink", version=1)
def __scene_link_reader(element):
    traits = _read_geometry_data(element)
    traits["target"] = element.get(NS_KARABO + "target")
    # If unspecified, the default is 'mainwin'
    target_window = element.get(NS_KARABO + "target_window", "mainwin")
    traits["target_window"] = SceneTargetWindow(target_window)
    traits["text"] = element.get(NS_KARABO + "text", "")
    traits["font"] = element.get(NS_KARABO + "font", "")
    traits["foreground"] = element.get(NS_KARABO + "foreground", "black")

    bg = element.get(NS_KARABO + "background")
    if bg is not None:
        traits["background"] = bg
    fw = element.get(NS_KARABO + "frameWidth")
    if fw is not None:
        traits["frame_width"] = int(fw)

    return SceneLinkModel(**traits)


@register_scene_writer(SceneLinkModel)
def __scene_link_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    _write_class_and_geometry(model, element, "SceneLink")
    element.set(NS_KARABO + "target", model.target)
    element.set(NS_KARABO + "target_window", model.target_window.value)

    for name in ("text", "font", "foreground"):
        element.set(NS_KARABO + name, getattr(model, name))

    if model.background != "":
        element.set(NS_KARABO + "background", model.background)

    return element
