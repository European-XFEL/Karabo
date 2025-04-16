# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import numpy as np

from karabo.common.states import State
from karabo.native import (
    AccessLevel, AccessMode, Assignment, Bool, ByteArray, Char, Configurable,
    Double, Float, Int8, Int16, Int32, Int64, Node, Slot, String, TypeHash,
    TypeSchema, UInt8, UInt16, UInt32, UInt64, Unit, VectorBool, VectorChar,
    VectorDouble, VectorFloat, VectorHash, VectorInt8, VectorInt16,
    VectorInt32, VectorInt64, VectorString, VectorUInt8, VectorUInt16,
    VectorUInt32, VectorUInt64)
from karabogui.binding.api import (
    BoolBinding, ByteArrayBinding, CharBinding, FloatBinding, HashBinding,
    Int8Binding, Int16Binding, Int32Binding, Int64Binding, NodeBinding,
    SchemaBinding, SlotBinding, StringBinding, Uint8Binding, Uint16Binding,
    Uint32Binding, Uint64Binding, VectorBoolBinding, VectorCharBinding,
    VectorDoubleBinding, VectorFloatBinding, VectorHashBinding,
    VectorInt8Binding, VectorInt16Binding, VectorInt32Binding,
    VectorInt64Binding, VectorStringBinding, VectorUint8Binding,
    VectorUint16Binding, VectorUint32Binding, VectorUint64Binding)


def get_all_props_schema():
    return AllProperties.getClassSchema()


def get_simple_props_schema():
    return SimpleProperties.getClassSchema()


def get_pipeline_schema():
    return PipelineData.getClassSchema()


def get_pipeline_vector_schema():
    return VectorPipelineData.getClassSchema()


def get_simple_schema():
    return Simple.getClassSchema()


def get_slotted_schema():
    return SlottedDevice.getClassSchema()


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


class OutputVectorNodeInner(Configurable):
    vector = VectorDouble()


class OutputVectorNode(Configurable):
    data = Node(OutputVectorNodeInner)


class OutputNode(Configurable):
    data = Node(OutputNodeInner)


class Simple(Configurable):
    """A configurable with a few properties
    """
    foo = Bool(defaultValue=True)
    bar = String(defaultValue="default")
    charlie = Int32(accessMode=AccessMode.READONLY)


class PipelineData(Configurable):
    output = Node(OutputNode, displayType='OutputChannel')


class VectorPipelineData(Configurable):
    output = Node(OutputVectorNode, displayType='OutputChannel')


class SlottedDevice(Configurable):
    state = String(defaultValue=State.ACTIVE)

    @Slot(allowedStates=[State.INIT, State.ACTIVE])
    def callme(self):
        pass


class RowSchema(Configurable):
    start = Float()
    stop = Float()


ALL_PROPERTIES_MAP = {
    'a': BoolBinding, 'b': CharBinding, 'e': FloatBinding, 'f': FloatBinding,
    'g': HashBinding, 'h': Int16Binding, 'i': Int32Binding,
    'j': Int64Binding, 'k': Int8Binding, 'll': SchemaBinding,
    'm': StringBinding, 'n': Uint16Binding, 'o': Uint32Binding,
    'p': Uint64Binding, 'q': Uint8Binding, 'r': VectorBoolBinding,
    's': VectorCharBinding, 'v': VectorDoubleBinding,
    'w': VectorFloatBinding, 'x': VectorHashBinding,
    'y': VectorInt16Binding, 'z': VectorInt32Binding,
    'a1': VectorInt64Binding, 'b1': VectorInt8Binding,
    'c1': VectorStringBinding, 'd1': VectorUint16Binding,
    'e1': VectorUint32Binding, 'f1': VectorUint64Binding,
    'g1': VectorUint8Binding, 'h1': NodeBinding,
    'k1': SlotBinding, 'mm': ByteArrayBinding,
}


class AllProperties(Configurable):
    """A `Configurable` with every type of property
    """
    a = Bool(defaultValue=True, requiredAccessLevel=AccessLevel.EXPERT)
    b = Char(defaultValue='c')
    e = Double(unitSymbol=Unit.METER)
    f = Float()
    g = TypeHash()
    h = Int16()
    i = Int32(accessMode=AccessMode.READONLY, assignment=Assignment.INTERNAL)
    j = Int64(unitSymbol=Unit.METER)
    k = Int8()
    ll = TypeSchema()
    m = String(options=['foo', 'bar', 'baz', 'qux'])
    n = UInt16()
    o = UInt32()
    p = UInt64()
    q = UInt8()
    r = VectorBool()
    s = VectorChar()
    v = VectorDouble()
    w = VectorFloat()
    x = VectorHash(rows=RowSchema)
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
    mm = ByteArray()

    @Slot(allowedStates=[State.INTERLOCKED, State.ACTIVE])
    def k1(self):
        pass


class VectorAttr(Configurable):
    vec = VectorBool(defaultValue=np.array([True, True]))


class SimpleProperties(Configurable):
    """A `Configurable` with every type of property
    """
    boolProperty = Bool(defaultValue=True)
    falseProperty = Bool(defaultValue=False)
    doubleProperty = Double(unitSymbol=Unit.METER)
    floatProperty = Float(accessMode=AccessMode.READONLY)
    intProperty = Int16(accessMode=AccessMode.READONLY)
    stringProperty = String(options=['foo', 'bar', 'baz', 'qux'])
    vectorProperty = VectorDouble()
    vectorStringProperty = VectorString()
    table = VectorHash(Simple)
    node = Node(Simple)
    internal = Bool(defaultValue=True,
                    assignment=Assignment.INTERNAL)

    @Slot(allowedStates=[State.INTERLOCKED, State.ACTIVE])
    def anySlot(self):
        pass
