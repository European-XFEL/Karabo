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
from lxml import etree


class ProjectDBError(Exception):
    pass


def to_string(xml_rep):
    """returns a string serialization of an xml element

    the output is listed on multiple lines and unicode encoded"""
    return etree.tostring(
        xml_rep,
        pretty_print=True,
        encoding="unicode",
        xml_declaration=False)


def make_xml_if_needed(xml_rep):
    """
    Returns an etree xml object from xml_rep
    :param xml_rep: the xml
    :return: a root node for the xml object
    :raises: ValueError if the object passed is not of type str or type
                etree.ElementBase
    """
    if isinstance(xml_rep, etree._Element):
        return xml_rep
    if isinstance(xml_rep, bytes):
        xml_rep = xml_rep.decode('utf-8')
    if isinstance(xml_rep, str):
        try:
            return etree.fromstring(xml_rep)
        except etree.XMLSyntaxError as e:
            raise ValueError(
                f"XML syntax error encountered while parsing!: {e}")

    raise ValueError(f"Cannot handle type {type(xml_rep)}: {xml_rep}")


def make_str_if_needed(xml_rep):
    """
    Returns a string representation of xml_rep
    :param xml_rep: the xml
    :return: a string representation of xml_rep
    :raises: ValueError if the object passed is not of type str or type
                etree.ElementBase
    """
    if isinstance(xml_rep, bytes):
        xml_rep = xml_rep.decode('utf-8')
    if isinstance(xml_rep, str):
        return xml_rep
    if isinstance(xml_rep, etree._Element):
        return to_string(xml_rep)

    raise ValueError(f"Cannot handle type {type(xml_rep)}")
