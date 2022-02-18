# flake8: noqa: F401
"""This is the complete public API for middle layer devices. You are
free to use everything defined here, try to avoid doing deep imports
into the Karabo package, as the internals may change.
"""
from karabo.native import (
    AccessLevel, AccessMode, ArchivePolicy, Assignment, Attribute, Bool,
    BoolValue, ByteArray, Char, ChoiceOfNodes, ComplexDouble, ComplexFloat,
    Configurable, DaqDataType, DaqPolicy, Descriptor, Double, EncodingType,
    Enumable, EnumValue, Float, Hash, HashByte, HashElement, HashList,
    HashMergePolicy, HashType, Image, ImageData, Int8, Int16, Int32, Int64,
    Integer, KaraboError, KaraboValue, LeafType, ListOfNodes, MetaRegistry,
    MetricPrefix, NDArray, Node, NodeType, NoneValue, Number, NumpyVector,
    OldProject, Overwrite, QuantityValue, RegexString, Registry, Schema,
    Simple, Slot, String, StringValue, TableValue, TimeMixin, Timestamp, Type,
    TypeHash, TypeSchema, UInt8, UInt16, UInt32, UInt64, Unit, Vector,
    VectorBool, VectorChar, VectorCharValue, VectorComplexDouble,
    VectorComplexFloat, VectorDouble, VectorFloat, VectorHash, VectorInt8,
    VectorInt16, VectorInt32, VectorInt64, VectorRegexString, VectorString,
    VectorStringValue, VectorUInt8, VectorUInt16, VectorUInt32, VectorUInt64,
    Weak, convert_old_project, daysAgo, decodeBinary, decodeXML, dictToHash,
    dtype_from_number, encodeBinary, encodeXML, get_array_data,
    get_default_value, get_descriptor_from_data, get_hash_type_from_data,
    get_image_data, get_instance_parent, get_timestamp, has_changes,
    hashToDict, hashtype_from_string, hoursAgo, is_equal, isSet, loadFromFile,
    minutesAgo, newest_timestamp, read_project_model, sanitize_table_schema,
    saveToFile, simple_deepcopy, string_from_hashtype, unit_registry as unit,
    write_project_model, writeBinary, writeXML)

# Common api
from .common.alarm_conditions import AlarmCondition
from .common.decorators import validate_args
from .common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    MacroModel, ProjectModel)
from .common.scenemodel.api import SceneModel, read_scene, write_scene
from .common.states import State
from .middlelayer_api import numeric
from .middlelayer_api.configuration import (
    config_changes, extract_modified_schema_attributes,
    sanitize_init_configuration, sanitize_write_configuration)
from .middlelayer_api.device import Device
# Middlelayer api
from .middlelayer_api.device_client import (
    DeviceClientBase, Queue, call, callNoWait, compareConfigurationsFromPast,
    compareDeviceConfiguration, compareDeviceWithPast, connectDevice,
    disconnectDevice, execute, executeNoWait, filterByTags, findDevices,
    findServers, getClasses, getClients, getConfiguration,
    getConfigurationFromName, getConfigurationFromPast, getDescriptors,
    getDevice, getDevices, getHistory, getInstanceInfo, getLastConfiguration,
    getSchema, getSchemaFromPast, getServers, getTopology, instantiate,
    instantiateFromName, instantiateNoWait, isAlive, listConfigurationFromName,
    listDevicesWithConfiguration, lock, saveConfigurationFromName, setNoWait,
    setWait, shutdown, shutdownNoWait, updateDevice, waitUntil, waitUntilNew,
    waitWhile)
from .middlelayer_api.devicenode import DeviceNode
from .middlelayer_api.eventloop import KaraboFuture
from .middlelayer_api.injectable import Injectable
from .middlelayer_api.json import KaraboJSONEncoder
from .middlelayer_api.macro import Macro, Monitor, RemoteDevice
from .middlelayer_api.pipeline import (
    InputChannel, NetworkInput, NetworkOutput, OutputChannel)
from .middlelayer_api.proxy import (
    ProxyBase as Proxy, ProxyNodeBase as ProxyNode, ProxySlotBase as ProxySlot,
    SubProxyBase as SubProxy)
from .middlelayer_api.signalslot import Signal, SignalSlotable, coslot, slot
from .middlelayer_api.synchronization import (
    allCompleted, background, firstCompleted, firstException, gather,
    processEvents, sleep, synchronous)
from .middlelayer_api.unitutil import (
    StateSignifier, maximum, minimum, removeQuantity)
from .middlelayer_api.utils import (
    build_karabo_value, get_property, profiler, set_property)


def _create_cli_submodule():
    """Create the namespace used by ikarabo."""
    import karabo
    from karabo.common.api import create_module

    from .middlelayer_api.ikarabo import connectDevice

    # NOTE: This is the middlelayer part of the ikarabo namespace
    symbols = (
        call, callNoWait, connectDevice, compareConfigurationsFromPast,
        compareDeviceConfiguration, compareDeviceWithPast, daysAgo,
        disconnectDevice, execute, has_changes, executeNoWait, findDevices,
        findServers, getClasses, getClients, getConfiguration,
        getConfigurationFromPast, getSchemaFromPast, getConfigurationFromName,
        getLastConfiguration, getDevice, listConfigurationFromName,
        listDevicesWithConfiguration, getDevices, getHistory, getInstanceInfo,
        getSchema, getServers, getTopology, get_timestamp, Hash, hoursAgo,
        instantiate, instantiateFromName, instantiateNoWait, isSet, karabo,
        minutesAgo, saveConfigurationFromName, setWait, setNoWait, shutdown,
        shutdownNoWait, sleep, State, Timestamp, waitUntil, waitUntilNew
    )
    module = create_module('karabo.middlelayer.cli', *symbols)
    module.__file__ = __file__  # looks nicer when repr(cli) is used
    return module


cli = _create_cli_submodule()
del _create_cli_submodule
