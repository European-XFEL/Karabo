from karabo.middlelayer import (
    AccessLevel, AccessMode, Bool, Configurable, Device, Double, Float, Hash,
    Int32, Int64, Node, Overwrite, UInt32, UInt64, State, String, VectorBool,
    VectorChar, VectorDouble, VectorFloat, VectorHash, VectorInt32,
    VectorInt64, VectorUInt32, VectorUInt64, VectorString)


class VectorNode(Configurable):
    boolProperty = VectorBool(
        displayedName="Boolean properties",
        description="Vector boolean values",
        defaultValue=[True, False, True, False, True, False],
        minSize=1,
        maxSize=10)

    charProperty = VectorChar(
        displayedName="Char Vector",
        description="Vector char values",
        minSize=1,
        maxSize=10,
        defaultValue="ABCDEF")

    int32Property = VectorInt32(
        displayedName="Int32 Vector (No Limits)",
        description="Vector int32 values",
        defaultValue=[-1, 2, 3])

    uint32Property = VectorUInt32(
        displayedName="UInt32 Vector",
        description="Vector uint32 values",
        defaultValue=[1, 2, 3],
        minSize=1,
        maxSize=10)

    int64Property = VectorInt64(
        displayedName="Int64 Vector (No Limits)",
        description="Vector int64 values",
        defaultValue=[1, 2],
        minSize=1,
        maxSize=10)

    uint64Property = VectorUInt64(
        displayedName="UInt64 Vector",
        description="Vector uint64 values",
        defaultValue=[-10, -10],
        minSize=1,
        maxSize=10)

    floatProperty = VectorFloat(
        displayedName="Float Vector (No Limits)",
        description="Vector float values",
        defaultValue=[7.1, 0.1, 5.3])

    doubleProperty = VectorDouble(
        displayedName="Double Vector",
        description="Vector double values",
        defaultValue=[1.23, 0.4, 1.0],
        minSize=1,
        maxSize=10)

    stringProperty = VectorString(
        displayedName="String Vector",
        description="Vector string values",
        minSize=1,
        maxSize=10,
        defaultValue=["A", "B", "C"])


class TableRow(Configurable):
    intProperty = UInt32(
        displayedName="Integer Property",
        description="An optional integer property",
        options=[1, 2, 3, 4],
        defaultValue=2)

    stringProperty = String(
        displayedName="String Property",
        description="An option property for strings",
        options=["spam", "eggs"],
        defaultValue="spam")

    boolInit = Bool(
        displayedName="Bool (INIT)",
        defaultValue=True,
        accessMode=AccessMode.INITONLY)

    bool = Bool(
        displayedName="Bool",
        defaultValue=False,
        accessMode=AccessMode.RECONFIGURABLE)

    boolReadOnly = Bool(
        displayedName="Bool (RO)",
        defaultValue=False,
        accessMode=AccessMode.READONLY)

    floatProperty = Float(
        displayedName="Float Property",
        description="A float property",
        defaultValue=-3.0)

    doubleProperty = Double(
        displayedName="Double Property",
        description="A double property",
        defaultValue=14.0)


class PropertyTestMDL(Device):
    __version__ = "2.3.0"

    allowedStates = [State.INIT, State.ON]

    state = Overwrite(
        defaultValue=State.INIT,
        options=allowedStates)

    visibility = Overwrite(
        defaultValue=AccessLevel.ADMIN,
        options=[AccessLevel.ADMIN])

    boolProperty = Bool(
        displayedName="Bool",
        description="a boolean value",
        defaultValue=False)

    boolPropertyReadonly = Bool(
        displayedName="Bool Readonly",
        description="A readonly boolean property",
        defaultValue=True,
        accessMode=AccessMode.READONLY)

    int32Property = Int32(
        displayedName="Int32",
        description="An integer property (32 Bit)",
        defaultValue=2,
        minInc=-20,
        maxInc=20)

    int32PropertyReadonly = Int32(
        displayedName="Int32 (RO)",
        description="A integer property",
        defaultValue=0,
        accessMode=AccessMode.READONLY)

    uint32Property = UInt32(
        displayedName="UInt32",
        description="An unsigned integer property (32 Bit)",
        defaultValue=2)

    uint32PropertyReadonly = UInt32(
        displayedName="UInt32 (RO)",
        description="A readonly integer property",
        defaultValue=30,
        accessMode=AccessMode.READONLY)

    int64Property = Int64(
        displayedName="Int64",
        description="An integer 64 Bit property",
        defaultValue=10)

    int64PropertyReadonly = Int64(
        displayedName="Int64 (RO)",
        description="A readonly integer property",
        defaultValue=0,
        accessMode=AccessMode.READONLY)

    uint64Property = UInt64(
        displayedName="UInt64",
        description="An unsigned integer property (64 Bit)",
        defaultValue=30)

    uint64PropertyReadonly = UInt64(
        displayedName="UInt64 (RO)",
        description="A readonly integer property (64 Bit)",
        defaultValue=47,
        accessMode=AccessMode.READONLY)

    floatProperty = Float(
        displayedName="Float (Min / Max)",
        description="A float property",
        defaultValue=20.0,
        minExc=-1000.0,
        maxExc=1000.0)

    floatPropertyReadonly = Float(
        displayedName="Float (RO)",
        description="A readonly float property",
        defaultValue=0.0,
        accessMode=AccessMode.READONLY)

    doubleProperty = Double(
        displayedName="Double",
        description="A double property",
        defaultValue=0.1)

    doublePropertyReadonly = Double(
        displayedName="Double (RO)",
        description="A readonly double property",
        defaultValue=3.1415,
        accessMode=AccessMode.READONLY)

    stringProperty = String(
        displayedName="String",
        description="A string property",
        defaultValue="XFEL")

    vectors = Node(
        VectorNode,
        displayedName="Vectors",
        description="A node containing vector properties")

    table = VectorHash(
        TableRow,
        displayedName="Table property",
        description="Table containing one node",
        accessMode=AccessMode.RECONFIGURABLE,
        defaultValue=[
            Hash('intProperty', 1, 'stringProperty', 'spam', 'boolInit', True,
                 'bool', False, 'boolReadOnly', False, 'floatProperty', 5.0,
                 'doubleProperty', 27.1),
            Hash('intProperty', 3, 'stringProperty', 'eggs', 'boolInit', False,
                 'bool', True, 'boolReadOnly', True, 'floatProperty', 1.3,
                 'doubleProperty', 22.5)])

    async def onInitialization(self):
        self.state = State.ON
