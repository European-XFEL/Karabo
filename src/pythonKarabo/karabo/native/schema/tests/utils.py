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
from karabo.native import AccessMode, Configurable, Int32, String, VectorHash


class Row(Configurable):
    anInt = Int32(defaultValue=33)
    aString = String(defaultValue="Spaghetti")


class WithTable(Configurable):
    """A Configurable with a Table Element."""
    table = VectorHash(
        rows=Row,
        displayedName="Table",
        accessMode=AccessMode.READONLY)
