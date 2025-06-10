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
from xml.etree.ElementTree import Element, parse, tostring

from .const import NS_KARABO, NS_SVG, SCENE_FILE_VERSION
from .exceptions import SceneWriterException
from .model import SceneModel
from .registry import read_element, set_reader_registry_version, write_element


def read_scene(filename_or_fileobj):
    """Read a scene and return it.
    filename_or_fileobj is either a string containing a filename, or a
    file-like object which can be read from (eg- a TextIO instance).
    If ``filename_or_fileobj`` is None, an empty MacroModel is returned.
    """
    if filename_or_fileobj is None:
        return SceneModel()

    tree = parse(filename_or_fileobj)
    root = tree.getroot()

    # Old files have no version, so '1' is the default.
    # The version number decides which readers are used for the file
    set_reader_registry_version(int(root.get(NS_KARABO + "version", "1")))
    return read_element(root)


def write_scene(scene):
    """Write Scene object `scene` to a string."""
    if scene.svg_data is not None:
        raise SceneWriterException(
            f"Scene {scene.simple_name} still contains svg_data.")
    root = Element(NS_SVG + "svg")
    # We always WRITE the most recent version.
    root.set(NS_KARABO + "version", str(SCENE_FILE_VERSION))
    return _writer_core(scene, root)


def write_single_model(model):
    """Write a scene model object as an SVG containing only that object."""
    root = Element(NS_SVG + "svg")
    return _writer_core(model, root)


def _writer_core(model, root):
    write_element(model, parent=root)
    return tostring(root, encoding="unicode")
