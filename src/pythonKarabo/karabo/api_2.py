""" This module provides all the API 2 names which a Device might need.
"""

from .api2.device_client import (
    connectDevice, DeviceClientBase, disconnectDevice, Proxy, ProxySlot,
    ProxyNode, AutoDisconnectProxy, SubProxy, getHistory, waitUntilNew,
    waitUntil, setWait, instantiate, instantiateNoWait, shutdown,
    shutdownNoWait, setNoWait, getClasses, getDevice, getDevices, getServers,
    execute, executeNoWait, updateDevice, sleep
)
from .api2.enums import (
    AccessLevel, AccessMode, Assignment, ChannelSpaceType, EncodingType,
    MetricPrefix, NodeType, Unit
)
from .api2.basetypes import (
    BoolValue, EnumValue, KaraboValue, QuantityValue, StringValue,
    unit_registry as unit, VectorStringValue, VectorCharValue
)
from .api2.exceptions import KaraboError
from .api2.hash import (
    Attribute, Bool, Char, ComplexDouble, ComplexFloat, Descriptor,
    Double, Element, Enumable, Float, Hash, HashElement, HashMergePolicy,
    HashType, Int16, Int32, Int64, Int8, Integer, ListElement, Number,
    NumpyVector, Schema, SchemaHashType, Simple, SimpleElement, Slot, Special,
    String, StringList, Type, UInt16, UInt32, UInt64, UInt8, Vector,
    VectorBool, VectorChar, VectorComplexDouble, VectorComplexFloat,
    VectorDouble, VectorFloat, VectorHash, VectorInt16, VectorInt32,
    VectorInt64, VectorInt8, VectorString, VectorUInt16, VectorUInt32,
    VectorUInt64, VectorUInt8, XMLWriter, XMLParser, BinaryWriter, BinaryParser
)
from .api2.macro import Macro, Monitor
from .api2.device import Device
from .api2.project import (
    BaseDevice, BaseDeviceGroup, BaseMacro, Project, ProjectAccess,
    ProjectConfiguration
)
from .api2.registry import MetaRegistry, Registry
from .api2.schema import (
    Configurable, Node, ChoiceOfNodes, ListOfNodes, Validator
)
from .api2.signalslot import Signal, SignalSlotable, slot, coslot
from .api2.timestamp import Timestamp
