"""This is the complete public API for middle layer devices. You are
free to use everything defined here, try to avoid doing deep imports
into the Karabo package, as the internals may change.
"""
from .common.alarm_conditions import AlarmCondition
from .common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    MacroModel, ProjectModel
)
from .common.scenemodel.api import SceneModel, read_scene, write_scene
from .common.states import State

from .middlelayer_api.device_client import (
    call, connectDevice, DeviceClientBase, disconnectDevice, execute,
    executeNoWait, filterByTags, getClasses, getConfiguration,
    getConfigurationFromPast, getDevice, getDevices, getDescriptors, getSchema,
    getServers, getHistory, isAlive, instantiate, instantiateNoWait, lock,
    waitUntilNew, waitUntil, waitWhile, setWait, shutdown, shutdownNoWait,
    setNoWait, updateDevice, Queue
)
from .middlelayer_api.proxy import (
    ProxyBase as Proxy, ProxySlotBase as ProxySlot,
    ProxyNodeBase as ProxyNode, SubProxyBase as SubProxy
)
from .middlelayer_api.enums import (
    AccessLevel, AccessMode, Assignment, DaqDataType, DaqPolicy, EncodingType,
    LeafType, MetricPrefix, NodeType, Unit
)
from .middlelayer_api.eventloop import KaraboFuture
from .middlelayer_api.basetypes import (
    BoolValue, EnumValue, isSet, KaraboValue, NoneValue, QuantityValue,
    StringValue, TableValue, unit_registry as unit, VectorStringValue,
    VectorCharValue
)
from .middlelayer_api.devicenode import DeviceNode
from .middlelayer_api.exceptions import KaraboError
from .middlelayer_api.hash import (
    Attribute, Bool, ByteArray, Char, ComplexDouble, ComplexFloat, Descriptor,
    Double, Enumable, Float, Hash, HashList,
    HashMergePolicy, HashType, Int16, Int32, Int64, Int8, Integer,
    Number, NumpyVector, Schema, SchemaHashType, Simple, Slot,
    Special, String, Type, UInt16, UInt32, UInt64, UInt8, Vector,
    VectorBool, VectorChar, VectorComplexDouble, VectorComplexFloat,
    VectorDouble, VectorFloat, VectorHash, VectorInt16, VectorInt32,
    VectorInt64, VectorInt8, VectorString, VectorUInt16, VectorUInt32,
    VectorUInt64, VectorUInt8
)
from .middlelayer_api.injectable import Injectable
from .middlelayer_api.json import KaraboJSONEncoder
from .middlelayer_api.ndarray import NDArray
from .middlelayer_api.pipeline import (
    InputChannel, NetworkInput, OutputChannel, NetworkOutput
)
from .middlelayer_api.project.api import (
    convert_old_project, OldProject, read_project_model, write_project_model
)
from .middlelayer_api.macro import Macro, Monitor, RemoteDevice
from .middlelayer_api.device import Device
from .middlelayer_api.registry import MetaRegistry, Registry
from .middlelayer_api.schema import (
    Configurable, Node, ChoiceOfNodes, ListOfNodes, Overwrite
)
from .middlelayer_api.serializers import (
    decodeBinary, decodeXML, encodeBinary, encodeXML, loadFromFile, saveToFile,
    writeBinary, writeXML, XMLParser, XMLWriter
)
from .middlelayer_api.signalslot import Signal, SignalSlotable, slot, coslot
from .middlelayer_api.synchronization import (
    allCompleted, background, firstCompleted, firstException, gather,
    processEvents, sleep, synchronous
)
from .middlelayer_api.time_mixin import TimeMixin, get_timestamp
from .middlelayer_api.timestamp import Timestamp
from .middlelayer_api.unitutil import (
    maximum, minimum, StateSignifier)
from .middlelayer_api import numeric


def _create_cli_submodule():
    """Create the namespace used by ikarabo."""
    import karabo
    from karabo.common.api import create_module
    from .middlelayer_api.ikarabo import connectDevice

    # NOTE: This is the middlelayer part of the ikarabo namespace
    symbols = (
        call, connectDevice, disconnectDevice, execute, executeNoWait,
        getClasses, getConfiguration, getDevice, getDevices, getHistory,
        getSchema, getServers, instantiate, instantiateNoWait, karabo,
        setWait, setNoWait, shutdown, shutdownNoWait, sleep, State, waitUntil,
        waitUntilNew
    )
    module = create_module('karabo.middlelayer.cli', *symbols)
    module.__file__ = __file__  # looks nicer when repr(cli) is used
    return module

cli = _create_cli_submodule()
del _create_cli_submodule
