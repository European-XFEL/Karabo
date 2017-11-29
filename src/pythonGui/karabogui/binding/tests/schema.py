import numpy as np

from karabo.middlelayer import (
    Bool, Char, ComplexDouble, ComplexFloat, Configurable,
    Double, Float, HashType, Int16, Int32, Int64, Int8,
    SchemaHashType, Slot, String, UInt16, UInt32, UInt64, UInt8,
    VectorBool, VectorChar, VectorComplexDouble, VectorComplexFloat,
    VectorDouble, VectorFloat, VectorHash, VectorInt16, VectorInt32,
    VectorInt64, VectorInt8, VectorString, VectorUInt16, VectorUInt32,
    VectorUInt64, VectorUInt8, Node, ChoiceOfNodes, ListOfNodes,
    AccessLevel, AccessMode, Assignment, State, Unit
)


def get_all_props_schema():
    return AllProperties.getClassSchema()


def get_pipeline_schema():
    return PipelineData.getClassSchema()


def get_simple_schema():
    return Simple.getClassSchema()


def get_slotted_schema():
    return SlottedDevice.getClassSchema()


def get_recursive_schema():
    return Recursive.getClassSchema()


def get_vectorattr_schema():
    return VectorAttr.getClassSchema()


class Multi(Configurable):
    """`ChoiceOfNodes` and `ListOfNodes` are implemented such that they take a
    single class as their argument, then get their collection of nodes from
    all the classes which inherit from that class. Yeah. I'll take _two_ of
    whatever that guy is having, please.
    """


class _NodeOne(Multi):
    zero = String(defaultValue='First')


class _NodeTwo(Multi):
    one = String(defaultValue='Second')


class OutputNodeInner(Configurable):
    image = Node(Configurable, displayType='Image')


class OutputNode(Configurable):
    data = Node(OutputNodeInner)


class Simple(Configurable):
    """A configurable with a few properties
    """
    foo = Bool(defaultValue=True)
    bar = String()


class PipelineData(Configurable):
    output = Node(OutputNode, displayType='OutputChannel')


class SlottedDevice(Configurable):
    state = String(defaultValue=State.ACTIVE)

    @Slot(allowedStates=[State.INIT, State.ACTIVE])
    def callme(self):
        pass


class AllProperties(Configurable):
    """A `Configurable` with every type of property
    """
    a = Bool(defaultValue=True)
    b = Char(defaultValue='c')
    c = ComplexDouble(requiredAccessLevel=AccessLevel.EXPERT)
    d = ComplexFloat(accessMode=AccessMode.READONLY,
                     assignment=Assignment.INTERNAL)
    e = Double(unitSymbol=Unit.METER)
    f = Float()
    g = HashType()
    h = Int16()
    i = Int32()
    j = Int64()
    k = Int8()
    ll = SchemaHashType()
    m = String(options=['foo', 'bar', 'baz', 'qux'])
    n = UInt16()
    o = UInt32()
    p = UInt64()
    q = UInt8()
    r = VectorBool()
    s = VectorChar()
    t = VectorComplexDouble()
    u = VectorComplexFloat()
    v = VectorDouble()
    w = VectorFloat()
    x = VectorHash()
    y = VectorInt16()
    z = VectorInt32()
    a1 = VectorInt64()
    b1 = VectorInt8()
    c1 = VectorString()
    d1 = VectorUInt16()
    e1 = VectorUInt32()
    f1 = VectorUInt64()
    g1 = VectorUInt8()
    h1 = Node(_NodeOne)
    i1 = ChoiceOfNodes(Multi)
    j1 = ListOfNodes(Multi)

    @Slot(allowedStates=[State.INTERLOCKED, State.ACTIVE])
    def k1(self):
        pass


class Recursive(Configurable):
    """A `Configurable` for testing recursive node types
    """
    con = ChoiceOfNodes(Multi, defaultValue='_NodeTwo')
    lon = ListOfNodes(Multi, defaultValue=['_NodeOne'])


class VectorAttr(Configurable):
    vec = VectorBool(defaultValue=np.array([True, True]))
