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
from xml.etree.ElementTree import fromstring

from ..registry import (
    read_element, register_scene_reader, set_reader_registry_version)

XML_DATA = "<xml><testy /></xml>"


@register_scene_reader("Root", xmltag="xml", version=1)
def _root_reader(element):
    ret = None
    for child in element:
        ret = read_element(child)
    return ret


@register_scene_reader("Test", xmltag="testy", version=3)
def _version_three_testy_reader(element):
    return 3


@register_scene_reader("Test", xmltag="testy", version=1)
def _version_one_testy_reader(element):
    return 1


@register_scene_reader("Test", xmltag="testy", version=2)
def _version_two_testy_reader(element):
    return 2


@register_scene_reader("First", xmltag="first", version=1)
@register_scene_reader("Second", xmltag="second", version=1)
def _doubly_registered_reader(element):
    return element.get("type")


def test_get_reader_simple():
    root = fromstring(XML_DATA)

    for version in (1, 2, 3):
        set_reader_registry_version(version)
        model = read_element(root)
        assert model == version


def test_get_reader_version_edges():
    root = fromstring(XML_DATA)

    # 'testy' has no version 4 reader... make sure we get version 3
    set_reader_registry_version(version=4)
    result = read_element(root)
    assert result == 3


def test_multiple_decorators():
    root = fromstring("""<second type="Second"/>""")
    set_reader_registry_version(version=1)
    result = read_element(root)
    assert result == "Second"

    root = fromstring("""<first type="First"/>""")
    set_reader_registry_version(version=1)
    result = read_element(root)
    assert result == "First"
