"""This is the complete public API for middle layer devices. You are
free to use everything defined here, try to avoid doing deep imports
into the Karabo package, as the internals may change.
"""
# Common api
from .common.alarm_conditions import AlarmCondition
from .common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    MacroModel, ProjectModel
)
from .common.scenemodel.api import SceneModel, read_scene, write_scene
from .common.states import State

# Middlelayer api
from .middlelayer_api.device_client import (
    call, callNoWait, compareDeviceConfiguration, compareDeviceWithPast,
    connectDevice, DeviceClientBase, disconnectDevice, execute,
    executeNoWait, findDevices, filterByTags, findServers, getClasses,
    getClients, getConfiguration, getConfigurationFromPast,
    getConfigurationFromName, getDevice, getLastConfiguration,
    getDevices, getDescriptors, listConfigurationFromName,
    listDevicesWithConfiguration, getTopology, getSchema, getSchemaFromPast,
    getServers, getHistory, isAlive, instantiate, instantiateFromName,
    instantiateNoWait, lock, waitUntilNew, waitUntil, waitWhile,
    saveConfigurationFromName, setWait, shutdown, shutdownNoWait, setNoWait,
    updateDevice, Queue,
)
from .middlelayer_api.proxy import (
    ProxyBase as Proxy, ProxySlotBase as ProxySlot,
    ProxyNodeBase as ProxyNode, SubProxyBase as SubProxy
)
from .middlelayer_api.configuration import (
    sanitize_init_configuration, sanitize_write_configuration,
    extract_modified_schema_attributes, config_changes)
from .middlelayer_api.compat import HAVE_UVLOOP
from .middlelayer_api.eventloop import KaraboFuture
from .middlelayer_api.devicenode import DeviceNode
from .middlelayer_api.device import Device
from .middlelayer_api.injectable import Injectable
from .middlelayer_api.json import KaraboJSONEncoder
from .middlelayer_api.macro import Macro, Monitor, RemoteDevice
from .middlelayer_api import numeric
from .middlelayer_api.pipeline import (
    InputChannel, NetworkInput, OutputChannel, NetworkOutput
)
from .middlelayer_api.signalslot import Signal, SignalSlotable, slot, coslot
from .middlelayer_api.synchronization import (
    allCompleted, background, firstCompleted, firstException, gather,
    processEvents, sleep, synchronous
)
from .middlelayer_api.unitutil import (
    maximum, minimum, removeQuantity, StateSignifier)
from .middlelayer_api.utils import build_karabo_value, get_property
# Native api
from karabo.native.data.basetypes import (
    BoolValue, EnumValue, ImageData, isSet, KaraboValue, NoneValue,
    QuantityValue, StringValue, TableValue, unit_registry as unit,
    VectorStringValue, VectorCharValue
)
from karabo.native.data.enums import (
    AccessLevel, AccessMode, ArchivePolicy, Assignment, DaqDataType,
    DaqPolicy, EncodingType, LeafType, MetricPrefix, NodeType, Unit
)
from karabo.native.data.hash import (
    Attribute, Bool, ByteArray, Char, ComplexDouble, ComplexFloat, Descriptor,
    Double, Enumable, Float, Hash, HashList, is_equal,
    HashMergePolicy, HashType, Int16, Int32, Int64, Int8, Integer,
    Number, NumpyVector, RegexString, Schema, SchemaHashType, Simple, Slot,
    Special, String, Type, UInt16, UInt32, UInt64, UInt8, Vector,
    VectorBool, VectorChar, VectorComplexDouble, VectorComplexFloat,
    VectorDouble, VectorFloat, VectorHash, VectorInt16, VectorInt32,
    VectorInt64, VectorInt8, VectorRegexString, VectorString, VectorUInt16,
    VectorUInt32, VectorUInt64, VectorUInt8
)
from karabo.native.data.ndarray import NDArray
from karabo.native.data.image_data import Image

from karabo.native.data.schema import (
    Configurable, Node, ChoiceOfNodes, ListOfNodes, Overwrite
)
from karabo.native.data.serializers import (
    decodeBinary, decodeXML, encodeBinary, encodeXML, loadFromFile, saveToFile,
    writeBinary, writeXML, XMLParser, XMLWriter
)
from karabo.native.data.utils import (
    dictToHash, dtype_from_number, get_image_data)
from karabo.native.exceptions import KaraboError
from karabo.native.project import (
    convert_old_project, OldProject, read_project_model, write_project_model
)
from karabo.native.registry import MetaRegistry, Registry
from karabo.native.time_mixin import TimeMixin, get_timestamp
from karabo.native.timestamp import daysAgo, hoursAgo, minutesAgo, Timestamp


def _create_cli_submodule():
    """Create the namespace used by ikarabo."""
    import karabo
    from karabo.common.api import create_module
    from .middlelayer_api.ikarabo import connectDevice

    # NOTE: This is the middlelayer part of the ikarabo namespace
    symbols = (
        call, callNoWait, connectDevice, compareDeviceConfiguration,
        compareDeviceWithPast, daysAgo, disconnectDevice, execute,
        executeNoWait, findDevices, findServers, getClasses, getClients,
        getConfiguration, getConfigurationFromPast, getSchemaFromPast,
        getConfigurationFromName, getLastConfiguration, getDevice,
        listConfigurationFromName, listDevicesWithConfiguration, getDevices,
        getHistory, getSchema, getServers, getTopology,
        get_timestamp, Hash, hoursAgo, instantiate, instantiateFromName,
        instantiateNoWait, karabo, minutesAgo, saveConfigurationFromName,
        setWait, setNoWait, shutdown, shutdownNoWait, sleep, State,
        Timestamp, waitUntil, waitUntilNew
    )
    module = create_module('karabo.middlelayer.cli', *symbols)
    module.__file__ = __file__  # looks nicer when repr(cli) is used
    return module


cli = _create_cli_submodule()
del _create_cli_submodule
