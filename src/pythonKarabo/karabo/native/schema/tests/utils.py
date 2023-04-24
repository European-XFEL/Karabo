# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
