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
