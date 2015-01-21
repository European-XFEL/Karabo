"""Support for legacy features

Historically, all devices just did a `from karabo.device import *`.
This is really weird, why should everything be defined in karabo.device?
So now karabo.device star-imports this module, which does the old stuff. """

from karabo.enums import MetricPrefix, Unit, EncodingType
from karabo.hash import Hash
from karabo.hashtypes import Type
from karabo.schema import *
import karabo.schema

from enum import Enum

__all__ = ["MetricPrefix", "Unit", "EncodingType", "Hash", "Validator",
           "Schema", "PATH_ELEMENT", "NODE_ELEMENT", "SLOT_ELEMENT",
           "IMAGE_ELEMENT", "OVERWRITE_ELEMENT", "CHOICE_ELEMENT",
           "LIST_ELEMENT", "RawImageData", "AccessType", "AssemblyRules",
           "READ", "WRITE", "INIT"]


AccessType = int
AssemblyRules = int
READ = 1
WRITE = 2
INIT = 4


def publish(enum):
    globals().update((e.name, e) for e in enum)
    __all__.extend(e.name for e in enum)


publish(MetricPrefix)
publish(Unit)


def RawImageData(image, type):
    return Hash("dims", list(image.shape),
                "type", Type.strs[image.dtype.str].hashname(),
                "data", image.tostring())


def parameter(name, value):
    def f(self):
        self.attributes[name] = value.value
        return self
    return f


class Dotter(object):
    def __init__(self, schema):
        self.schema = schema
        self.attributes = {}
        self.value = karabo.hash.Hash()

    def __getattr__(self, name):
        return partial(self.dotter, name)

    def key(self, name):
        self.key = name
        return self

    expertAccess = parameter('requiredAccessLevel', AccessLevel.EXPERT)
    advanced = parameter('requiredAccessLevel', AccessLevel.EXPERT)
    reconfigurable = parameter('accessMode', AccessMode.RECONFIGURABLE)
    init = parameter('accessMode', AccessMode.INITONLY)
    readOnly = parameter('accessMode', AccessMode.READONLY)
    assignmentOptional = parameter('assignment', Assignment.OPTIONAL)
    assignmentInternal = parameter('assignment', Assignment.INTERNAL)

    def noDefaultValue(self):
        return self

    def initialValue(self, value):
        self.attributes["defaultValue"] = value
        return self

    def dotter(self, name, *args):
        if len(args) == 1:
            args = args[0]
            if isinstance(args, Enum):
                args = args.value
            self.attributes[name] = args
        else:
            print('bad:', name, args)
        return self

    def commit(self):
        self.schema.hash[self.key] = self.value
        self.schema.hash[self.key, ...] = self.attributes


class Leaf(Dotter):
    def commit(self):
        self.attributes['nodeType'] = 0
        n = str(type(self).__name__)
        self.attributes['valueType'] = self.hashname
        super(Leaf, self).commit()


for t in hashtypes.Type.types:
    if t is not None:
        name = "{}_ELEMENT".format(t.hashname())
        globals()[name] = type(name, (Leaf,), dict(hashname=t.hashname()))
        __all__.append(name)


class PATH_ELEMENT(STRING_ELEMENT):
    isDirectory = parameter('displayType', 'directory')


class NODE_ELEMENT(Dotter):
    def appendParametersOfConfigurableClass(self, cls, classId):
        self.value = cls.getSchema(classId).hash
        self.attributes['classId'] = classId
        self.attributes['displayType'] = cls.__name__
        return self

    def commit(self):
        self.attributes['nodeType'] = 1
        super(NODE_ELEMENT, self).commit()


class IMAGE_ELEMENT(NODE_ELEMENT):
    def commit(self):
        self.value = Image.getClassSchema().hash
        self.attributes['displayType'] = 'Image'
        super().commit()


class SLOT_ELEMENT(NODE_ELEMENT):
    def commit(self):
        self.attributes['displayType'] = 'Slot'
        super(SLOT_ELEMENT, self).commit()


class OVERWRITE_ELEMENT(Dotter):
    def key(self, name):
        self.key = name
        assert name in self.schema.hash
        return self

    def dotter(self, name, *args):
        assert name.startswith('setNew')
        name = name[6].lower() + name[7:]
        return Dotter.dotter(self, name, *args)

    def commit(self):
        self.schema.hash[self.key, ...].update(self.attributes)


class CHOICE_ELEMENT(Dotter):
    def appendNodesOfConfigurationBase(self, base):
        return self

    def commit(self):
        self.attributes['nodeType'] = 2
        super(CHOICE_ELEMENT, self).commit()


class LIST_ELEMENT(Dotter):
    def appendNodesOfConfigurationBase(self, base):
        if self.value is None:
            self.value = karabo.hash.Hash()
        for n in base.getRegisteredClasses():
            self.value[n] = base.getSchema(n).hash
            d = dict(classId=n, displayType=n, nodeType=1, accessMode=7)
            self.value[n, ...] = d
        return self

    def commit(self):
        self.attributes['nodeType'] = 3
        super(LIST_ELEMENT, self).commit()
