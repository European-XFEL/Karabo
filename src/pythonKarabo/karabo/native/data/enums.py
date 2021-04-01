from enum import Enum, IntEnum, unique
from functools import total_ordering

__all__ = ['AccessLevel', 'AccessMode', 'ArchivePolicy', 'Assignment',
           'NodeType', 'LeafType', 'DaqDataType', 'MetricPrefix',
           'Unit', 'DimensionType', 'EncodingType', 'DaqPolicy']


@total_ordering
@unique
class AccessLevel(Enum):
    OBSERVER = 0
    USER = 1
    OPERATOR = 2
    EXPERT = 3
    ADMIN = 4
    GOD = 5

    def __gt__(self, other):
        if self.__class__ is other.__class__:
            return self.value > other.value
        return NotImplemented


@unique
class AccessMode(Enum):
    UNDEFINED = -1
    INITONLY = 1
    READONLY = 2
    RECONFIGURABLE = 4


@unique
class ArchivePolicy(Enum):
    EVERY_EVENT = 0
    EVERY_100MS = 1
    EVERY_1S = 2
    EVERY_5S = 3
    EVERY_10S = 4
    EVERY_1MIN = 5
    EVERY_10MIN = 6
    NO_ARCHIVING = 7


@unique
class Assignment(Enum):
    OPTIONAL = 0
    MANDATORY = 1
    INTERNAL = 2


@unique
class NodeType(IntEnum):
    Leaf = 0
    Node = 1
    ChoiceOfNodes = 2
    ListOfNodes = 3

    def __str__(self):
        return str(self.value)


@unique
class LeafType(IntEnum):
    Property = 0
    Command = 1
    State = 2
    AlarmCondition = 3


@unique
class DaqDataType(IntEnum):
    PULSE = 0
    TRAIN = 1
    PULSEMASTER = 10
    TRAINMASTER = 11


@unique
class MetricPrefix(Enum):
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


@unique
class Unit(Enum):
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


@unique
class DimensionType(IntEnum):
    """Dimension type used for image data
    """
    UNDEFINED = 0
    STACK = -1
    DATA = 1


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
    CMYK = 5
    YUV = 6
    BAYER = 7
    JPEG = 8
    PNG = 9
    BMP = 10
    TIFF = 11


@unique
class DaqPolicy(IntEnum):
    """DAQ storage policy
    """
    UNSPECIFIED = -1
    OMIT = 0
    SAVE = 1
