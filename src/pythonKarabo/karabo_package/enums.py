from enum import Enum

__all__ = ["AccessLevel", "AccessMode", "Assignment", "MetricPrefix", "Unit"]

ChannelSpaceType = (
    "u_8_1", "s_8_1", "u_10_2", "s_10_2", "u_12_2", "s_12_2", "u_12_1p5",
    "s_12_1p5", "u_16_2", "s_16_2", "f_16_2", "u_32_4", "s_32_4", "f_32_4",
    "u_64_8", "s_64_8", "f_64_8", "u_16_1", "s_16_1", "u_32_1", "s_32_1",
    "f_32_1", "u_64_1", "s_64_1", "f_64_1")


class AccessLevel(Enum):
    OBSERVER = 0
    USER = 1
    OPERATOR = 2
    EXPERT = 3
    ADMIN = 4

class AccessMode(Enum):
    UNDEFINED = -1
    INITONLY = 1
    READONLY = 2
    RECONFIGURABLE = 4

class Assignment(Enum):
    OPTIONAL = 0
    MANDATORY = 1
    INTERNAL = 2

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


class Unit(Enum):
    """ A fair collections of units from the SI system """
    NUMBER = "#"
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
    BIT = "B"
    METER_PER_SECOND = "m/s"
    VOLT_PER_SECOND = "V/s"
    AMPERE_PER_SECOND = "A/s"
    PERCENT = "%"
    NOT_ASSIGNED = "N_A"


class EncodingType:
    # from karabo.xip.Encoding.EncodingType
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
