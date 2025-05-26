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
from enum import IntEnum, StrEnum, unique

from karabo.common.api import (
    KARABO_SCHEMA_ACCESS_MODE, KARABO_SCHEMA_ARCHIVE_POLICY,
    KARABO_SCHEMA_ASSIGNMENT, KARABO_SCHEMA_METRIC_PREFIX_SYMBOL,
    KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, KARABO_SCHEMA_UNIT_SYMBOL)


@unique
class AccessLevel(IntEnum):
    OBSERVER = 0
    OPERATOR = 1
    EXPERT = 2

    @classmethod
    def fromAttributes(cls, attrs):
        """returns an AccessLevel from schema attributes

        :param attrs: The Schema attributes can be obtained with
                      `schema.hash["path.to.property", ...]`
        :return: an AccessLevel or None if missing
        """
        al = attrs.get(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)
        return cls(al) if al is not None else None


@unique
class AccessMode(IntEnum):
    UNDEFINED = -1
    INITONLY = 1
    READONLY = 2
    RECONFIGURABLE = 4

    @classmethod
    def fromAttributes(cls, attrs):
        """returns an AccessMode from schema attributes

        :param attrs: The Schema attributes can be obtained with
                      `schema.hash["path.to.property", ...]`
        :return: an AccessMode or None if missing
        """
        am = attrs.get(KARABO_SCHEMA_ACCESS_MODE)
        return cls(am) if am is not None else None


@unique
class ArchivePolicy(IntEnum):
    EVERY_EVENT = 0
    NO_ARCHIVING = 1

    @classmethod
    def fromAttributes(cls, attrs):
        """returns an ArchivePolicy from schema attributes

        :param attrs: The Schema attributes can be obtained with
                      `schema.hash["path.to.property", ...]`
        :return: an ArchivePolicy or None if missing
        """
        ap = attrs.get(KARABO_SCHEMA_ARCHIVE_POLICY)
        return cls(ap) if ap is not None else None


@unique
class Assignment(IntEnum):
    OPTIONAL = 0
    MANDATORY = 1
    INTERNAL = 2

    @classmethod
    def fromAttributes(cls, attrs):
        """returns an Assignment from schema attributes

        :param attrs: The Schema attributes can be obtained with
                      `schema.hash["path.to.property", ...]`
        :return: an Assignment or None if missing
        """
        a = attrs.get(KARABO_SCHEMA_ASSIGNMENT)
        return cls(a) if a is not None else None


@unique
class NodeType(IntEnum):
    Leaf = 0
    Node = 1

    def __str__(self):
        return str(self.value)


@unique
class DaqDataType(IntEnum):
    PULSE = 0
    TRAIN = 1
    PULSEMASTER = 10
    TRAINMASTER = 11


@unique
class MetricPrefix(StrEnum):
    """ This are all the defined prefixes in the SI system """
    YOTTA = "Y"
    ZETTA = "Z"
    EXA = "E"
    PETA = "P"
    TERA = "T"
    GIGA = "G"
    MEGA = "M"
    KILO = "k"
    HECTO = "h"
    DECA = "da"
    NONE = ""
    DECI = "d"
    CENTI = "c"
    MILLI = "m"
    MICRO = "u"
    NANO = "n"
    PICO = "p"
    FEMTO = "f"
    ATTO = "a"
    ZEPTO = "z"
    YOCTO = "y"

    @classmethod
    def fromAttributes(cls, attrs):
        """returns a MetricPrefix from schema attributes

        :param attrs: The Schema attributes can be obtained with
                      `schema.hash["path.to.property", ...]`
        :return: a MetricPrefix or None if missing
        """
        mp = attrs.get(KARABO_SCHEMA_METRIC_PREFIX_SYMBOL)
        return cls(mp) if mp is not None else None


@unique
class Unit(StrEnum):
    """ A fair collections of units from the SI system """
    NUMBER = ""
    COUNT = "#"
    METER = "m"
    GRAM = "g"
    SECOND = "s"
    AMPERE = "A"
    KELVIN = "K"
    MOLE = "mol"
    CANDELA = "cd"
    HERTZ = "Hz"
    RADIAN = "rad"
    DEGREE = "deg"
    STERADIAN = "sr"
    NEWTON = "N"
    PASCAL = "Pa"
    JOULE = "J"
    ELECTRONVOLT = "eV"
    WATT = "W"
    COULOMB = "C"
    VOLT = "V"
    FARAD = "F"
    OHM = "Ω"
    SIEMENS = "S"
    WEBER = "Wb"
    TESLA = "T"
    HENRY = "H"
    DEGREE_CELSIUS = "degC"
    LUMEN = "lm"
    LUX = "lx"
    BECQUEREL = "Bq"
    GRAY = "Gy"
    SIEVERT = "Sv"
    KATAL = "kat"
    MINUTE = "min"
    HOUR = "h"
    DAY = "d"
    YEAR = "a"
    BAR = "bar"
    PIXEL = "px"
    BYTE = "B"
    BIT = "bit"
    METER_PER_SECOND = "m/s"
    VOLT_PER_SECOND = "V/s"
    AMPERE_PER_SECOND = "A/s"
    PERCENT = "%"
    NOT_ASSIGNED = "N_A"

    # Karabo 3
    REVOLUTIONS_PER_MINUTE = "rpm"

    @classmethod
    def fromAttributes(cls, attrs):
        """returns an Unit from schema attributes

        :param attrs: The Schema attributes can be obtained with
                      `schema.hash["path.to.property", ...]`
        :return: an Unit or None if missing
        """
        us = attrs.get(KARABO_SCHEMA_UNIT_SYMBOL)
        return cls(us) if us is not None else None


@unique
class EncodingType(IntEnum):
    """Encoding type used for image data
    """
    UNDEFINED = -1
    GRAY = 0
    RGB = 1
    RGBA = 2
    BGR = 3
    BGRA = 4
    BAYER_RG = 5
    BAYER_BG = 6
    BAYER_GR = 7
    BAYER_GB = 8
    YUV444 = 9
    YUV422_YUYV = 10
    YUV422_UYVY = 11
    JPEG = 12
