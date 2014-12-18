"""Support for legacy features

Historically, all devices just did a `from karabo.device import *`.
This is really weird, why should everything be defined in karabo.device?
So now karabo.device star-imports this module, which does the old stuff. """

import numpy

from karabo.enums import MetricPrefix, Unit, EncodingType, ArchivePolicy
from karabo.decorators import KARABO_CLASSINFO
from karabo.hash import Hash
from karabo.hashtypes import Type, StringList, Vector, String
from karabo.schema import *
from karabo.base_fsm import BaseFsm
import karabo.schema

from enum import Enum

__all__ = ["MetricPrefix", "Unit", "EncodingType", "KARABO_CLASSINFO",
           "Schema", "PATH_ELEMENT", "NODE_ELEMENT", "SLOT_ELEMENT",
           "IMAGE_ELEMENT", "OVERWRITE_ELEMENT", "CHOICE_ELEMENT",
           "LIST_ELEMENT", "RawImageData", "BaseFsm", "Types"]


def publish(enum):
    globals().update((e.name, e) for e in enum)
    __all__.extend(e.name for e in enum)


publish(MetricPrefix)
publish(Unit)
publish(ArchivePolicy)


def RawImageData(image, type):
    return Hash("dims", list(image.shape),
                "type", Type.strs[image.dtype.str].hashname(),
                "data", image.tostring())


def parameter(name, value):
    def f(self):
        self.attributes[name] = value.value
        return self
    return f


def fixed(name, value):
    def f(self):
        self.attributes[name] = value
        return self
    return f


def parameterlist(name):
    def f(self, value, sep=None):
        if sep is None:
            if "," in value:
                sep = ","
        self.attributes[name] = StringList(
            s.strip() for s in value.split(sep))
        return self
    return f

def simple(name, type=str):
    def f(self, value):
        assert isinstance(value, type)
        self.attributes[name] = value
        return self
    return f

def sametype(name):
    def f(self, value):
        self.attributes[name] = self.hashtype().cast(value)
        return self
    return f

def enumparam(name, enum):
    def f(self, value):
        assert isinstance(value, enum)
        self.attributes[name] = value.value
        return self
    return f


class Dotter(object):
    hashtype = None

    def __init__(self, schema):
        self.schema = schema
        self.attributes = {}
        self.value = 0

    def __getattr__(self, name):
        return partial(self.dotter, name)

    def key(self, name):
        self.key = name
        return self

    operatorAccess = parameter('requiredAccessLevel', AccessLevel.OPERATOR)
    adminAccess = parameter('requiredAccessLevel', AccessLevel.ADMIN)
    userAccess = parameter('requiredAccessLevel', AccessLevel.USER)
    expertAccess = parameter('requiredAccessLevel', AccessLevel.EXPERT)
    observerAccess = parameter("requiredAccessLevel", AccessLevel.OBSERVER)
    advanced = parameter('requiredAccessLevel', AccessLevel.EXPERT)
    reconfigurable = parameter('accessMode', AccessMode.RECONFIGURABLE)
    init = parameter('accessMode', AccessMode.INITONLY)
    assignmentOptional = parameter('assignment', Assignment.OPTIONAL)
    assignmentInternal = parameter('assignment', Assignment.INTERNAL)
    oct = fixed("displayType", "oct")
    hex = fixed("displayType", "hex")

    tags = parameterlist("tags")
    options = parameterlist("options")
    allowedStates = parameterlist("allowedStates")

    description = simple("description")
    displayedName = simple("displayedName")
    maxSize = simple("maxSize", int)
    minSize = simple("minSize", int)
    alias = simple("alias", object)

    defaultValue = sametype("defaultValue")
    maxInc = sametype("maxInc")
    minInc = sametype("minInc")
    maxExc = sametype("maxExc")
    minExc = sametype("minExc")
    warnLow = sametype("warnLow")
    warnHigh = sametype("warnHigh")
    alarmLow = sametype("alarmLow")
    alarmHigh = sametype("alarmHigh")

    archivePolicy = enumparam("archivePolicy", ArchivePolicy)

    def noDefaultValue(self):
        if "defaultValue" in self.attributes:
            del self.attributes["defaultValue"]
        return self

    def assignmentMandatory(self):
        self.attributes["assignment"] = Assignment.MANDATORY.value
        return self.noDefaultValue()

    def defaultValueFromString(self, value):
        self.attributes["defaultValue"] = self.hashtype.fromstring(value)
        return self

    def readOnly(self):
        self.attributes.update(accessMode=2, assignment=0,
                               requiredAccessLevel=0)
        return self

    def initialValue(self, value):
        self.attributes["defaultValue"] = value
        return self

    def metricPrefix(self, value):
        self.attributes["metricPrefixSymbol"] = value.value
        self.attributes["metricPrefixName"] = value.name.lower()
        self.attributes["metricPrefixEnum"] = value.number
        return self

    def unit(self, value):
        self.attributes["unitSymbol"] = value.value
        self.attributes["unitName"] = value.name.lower()
        self.attributes["unitEnum"] = value.number
        return self

    def bin(self, format=None):
        if format is None:
            self.attributes["displayType"] = "bin"
        else:
            self.attributes["displayType"] = "bin|" + format
        return self

    hashtype = None

    def commit(self):
        self.schema.hash[self.key] = self.value
        self.schema.hash[self.key, ...] = self.attributes


class Leaf(Dotter):
    def __init__(self, schema):
        super().__init__(schema)
        self.attributes = dict(requiredAccessLevel=1, accessMode=1,
                               assignment=0)
        if issubclass(self.hashtype, Vector):
            self.attributes["displayType"] = "Curve"
            self.attributes["defaultValue"] = self.hashtype().cast([])
        elif issubclass(self.hashtype, String):
            self.attributes["defaultValue"] = "0"

    def commit(self):
        self.attributes['nodeType'] = 0
        self.attributes['leafType'] = 0
        n = str(type(self).__name__)
        self.attributes['valueType'] = self.hashtype.hashname()
        super(Leaf, self).commit()


for t in hashtypes.Type.types:
    if t is not None:
        name = "{}_ELEMENT".format(t.hashname())
        globals()[name] = type(name, (Leaf,), dict(hashtype=t))
        __all__.append(name)


class PATH_ELEMENT(STRING_ELEMENT):
    isDirectory = fixed('displayType', 'directory')
    isOutputFile = fixed('displayType', 'fileOut')
    isInputFile = fixed('displayType', 'fileIn')


class NODE_ELEMENT(Dotter):
    def __init__(self, schema):
        super().__init__(schema)
        self.attributes = dict(accessMode=4)
        self.value = Hash()

    def appendParametersOfConfigurableClass(self, cls, classId):
        self.value = cls.getSchema(classId).hash
        self.attributes['classId'] = classId
        self.attributes['displayType'] = cls.__name__
        return self

    def appendParametersOf(self, cls):
        self.value = cls.getSchema(cls.__classid__).hash
        return self

    def commit(self):
        self.attributes['nodeType'] = 1
        super(NODE_ELEMENT, self).commit()


class IMAGE_ELEMENT(NODE_ELEMENT):
    def __init__(self, schema):
        super().__init__(schema)
        self.value = Hash("data", 0, "dims", 0, "roiOffsets", 0, "encoding", 0,
                          "channelSpace", 0, "type", 0, "isBigEndian", 0)
        dd = dict(assignment=0, leafType=0, accessMode=2, archivePolicy=7,
                  nodeType=0, requiredAccessLevel=0)
        self.value["data", ...] = dict(
            dd, defaultValue=b"", valueType="VECTOR_CHAR",
            description="Pixel array", displayType="Curve")
        self.value["dims", ...] = dict(
            dd, valueType="VECTOR_UINT32",
            description="The length of the array reflects total "
            "dimensionality and each element the extension in this dimension",
            defaultValue=numpy.array([], dtype=numpy.uint32),
            displayedName="Dimensions", displayType="Curve")
        self.value["roiOffsets", ...] = dict(
            dd, valueType="VECTOR_UINT32",
            description="Describes the offset of the Region-of-Interest; it "
            "will contain zeros if the image has no ROI defined",
            defaultValue=numpy.array([], dtype=numpy.uint32),
            displayedName="ROI Offsets", displayType="Curve")
        self.value["encoding", ...] = dict(
            dd, valueType="INT32", displayedName="Encoding",
            description='Describes the color space of pixel encoding of the '
            'data (e.g. GRAY, RGB, JPG, PNG etc.', defaultValue="0")
        self.value["channelSpace", ...] = dict(
            dd, defaultValue="0", valueType="INT32",
            description='Describes the channel encoding, i.e. '
            'signed/unsigned/floating point, bits per channel and bytes per '
            'pixel', displayedName="Channel space")
        self.value["type", ...] = dict(
            dd, defaultValue="0", valueType="STRING",
            description='Describes the underlying data type',
            displayedName="Type")
        self.value["isBigEndian", ...] = dict(
            dd, defaultValue="0", valueType="BOOL",
            description='Flags whether the raw data are in big or little '
            'endian', displayedName="Is big endian")
        self.attributes.update(displayType="Image", archivePolicy=7,
                               accessMode=2)


class SLOT_ELEMENT(NODE_ELEMENT):
    def __init__(self, schema):
        super().__init__(schema)
        self.attributes.update(displayType='Slot', requiredAccessLevel=1)
        self.value = Hash("connectedSignals", 0)
        self.value["connectedSignals", ...] = dict(
            valueType="VECTOR_STRING",
            description="Signals already connected to this slot",
            displayedName="Connected Signals", nodeType=0,
            requiredAccessLevel=3, accessMode=4, assignment=0, leafType=1)


class OVERWRITE_ELEMENT(Dotter):
    def key(self, name):
        self.key = name
        assert name in self.schema.hash
        return self

    def setNowReconfigurable(self):
        self.attributes["accessMode"] = 4
        return self

    def dotter(self, name, *args):
        assert name.startswith('setNew')
        name = name[6].lower() + name[7:]
        return Dotter.dotter(self, name, *args)

    def commit(self):
        self.schema.hash[self.key, ...].update(self.attributes)


class CHOICE_ELEMENT(Dotter):
    def __init__(self, schema):
        super().__init__(schema)
        self.attributes = dict(accessMode=4)
        self.value = Hash()

    def appendNodesOfConfigurationBase(self, base):
        for n in base.getRegisteredClasses():
            self.value[n] = base.getSchema(n).hash
            d = dict(classId=n, displayType=n, nodeType=1, accessMode=4)
            self.value[n, ...] = d
        return self

    defaultValue = simple("defaultValue")

    def commit(self):
        self.attributes['nodeType'] = 2
        super(CHOICE_ELEMENT, self).commit()


class LIST_ELEMENT(Dotter):
    def __init__(self, schema):
        super().__init__(schema)
        self.attributes = dict(accessMode=4)
        self.value = Hash()

    def appendNodesOfConfigurationBase(self, base):
        for n in base.getRegisteredClasses():
            self.value[n] = base.getSchema(n).hash
            d = dict(classId=n, displayType=n, nodeType=1, accessMode=4)
            self.value[n, ...] = d
        return self

    def defaultValueFromString(self, value):
        self.attributes["defaultValue"] = StringList(value.split(","))
        return self

    def commit(self):
        self.attributes['nodeType'] = 3
        super(LIST_ELEMENT, self).commit()


class Types:
    pass

for t in hashtypes.Type.types:
    if t is not None:
        setattr(Types, t.hashname(), t)
