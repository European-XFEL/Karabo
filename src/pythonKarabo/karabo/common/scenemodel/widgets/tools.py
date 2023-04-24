# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import base64
import re
from random import randint
from xml.etree.ElementTree import SubElement

from traits.api import String

from karabo.common.scenemodel.bases import BaseWidgetObjectData
from karabo.common.scenemodel.const import IMAGE_ELEMENT_KEY, IMAGE_ELEMENT_TAG
from karabo.common.scenemodel.io_utils import (
    read_base_widget_data, write_base_widget_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)


def convert_from_svg_image(href):
    """converts a `href` attribute to format and bytes

    takes the xlink:href attribute of an svg:image tag
    and returns the format and byte data.
    """
    try:
        matches = re.search(
            r"^data:image/(svg|png|jpg|jpeg);base64,(.+)", href
        )
        if not matches or len(matches.groups()) != 2:
            return "svg", b""
        data = base64.b64decode(matches.group(2))
        image_format = matches.group(1)
    except Exception:
        # Note: Make sure that we can always read an image, even if it gets
        # modified externally in devices. Leave a trace ...
        print(f"Malformed svg image found: {href[:80]}")
        image_format = "svg"
        data = b""
    return image_format, data


def convert_to_svg_image(image_format, image_bytes):
    """Convert the image bytes to an embedded href format expected by
    svg:image."""
    data = base64.b64encode(image_bytes).decode("ascii")
    svg_image = f"data:image/{image_format};base64,{data}"
    return svg_image


class ImageRendererModel(BaseWidgetObjectData):
    # The base model to save image data to the scene
    id = String
    image = String

    def _id_default(self):
        return f"image{randint(0, 1e6)}"


@register_scene_reader("ImageRenderer")
def _svg_image_data_reader(element):
    traits = read_base_widget_data(element)
    traits["id"] = element.get("id")
    traits["image"] = element.get(IMAGE_ELEMENT_KEY)
    return ImageRendererModel(**traits)


@register_scene_writer(ImageRendererModel)
def _svg_image_data_writer(model, parent):
    element = SubElement(parent, IMAGE_ELEMENT_TAG)
    write_base_widget_data(model, element, "ImageRenderer")
    element.set("id", model.id)
    image = model.image
    if image is None or len(image) == 0:
        # Save an image at all cost ...
        image = convert_to_svg_image("svg", b"")
    element.set(IMAGE_ELEMENT_KEY, image)
    # Keep the quality and the exact resize as in the GUI
    element.set("preserveAspectRatio", "none")

    return element
