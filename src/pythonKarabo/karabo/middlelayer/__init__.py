# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
# flake8: noqa: F401
"""This is the complete public API for middle layer devices. You are
free to use everything defined here, try to avoid doing deep imports
into the Karabo package, as the internals may change.
"""
# Common api
from karabo.common.alarm_conditions import AlarmCondition
from karabo.common.decorators import validate_args
from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    MacroModel, ProjectModel)
from karabo.common.scenemodel.api import SceneModel, read_scene, write_scene
from karabo.common.states import State
from karabo.native import (
    AccessLevel, AccessMode, ArchivePolicy, Assignment, Attribute, Bool,
    BoolValue, ByteArray, Char, Configurable, DaqDataType, Descriptor, Double,
    EncodingType, Enumable, EnumValue, Float, Hash, HashByte, HashElement,
    HashList, HashMergePolicy, HashType, Image, ImageData, Int8, Int16, Int32,
    Int64, Integer, KaraboError, KaraboJSONEncoder, KaraboValue, LeafType,
    MetricPrefix, NDArray, Node, NodeType, NoneValue, Number, NumpyVector,
    OldProject, Overwrite, QuantityValue, RegexString, Schema, Simple, Slot,
    String, StringValue, TableValue, TimeMixin, Timestamp, Type, TypeHash,
    TypeSchema, UInt8, UInt16, UInt32, UInt64, Unit, Vector, VectorBool,
    VectorChar, VectorCharValue, VectorDouble, VectorFloat, VectorHash,
    VectorInt8, VectorInt16, VectorInt32, VectorInt64, VectorRegexString,
    VectorString, VectorStringValue, VectorUInt8, VectorUInt16, VectorUInt32,
    VectorUInt64, Weak, convert_old_project, daysAgo, decodeBinary,
    decodeBinaryPos, decodeXML, dictToHash, dtype_from_number, encodeBinary,
    encodeXML, get_array_data, get_default_value, get_descriptor_from_data,
    get_hash_type_from_data, get_image_data, get_instance_parent,
    get_timestamp, has_changes, hashToDict, hashtype_from_string, hoursAgo,
    is_equal, isSet, isStringSet, loadFromFile, minutesAgo, newest_timestamp,
    read_project_model, sanitize_table_schema, saveToFile, simple_deepcopy,
    string_from_hashtype, unit_registry as unit, write_project_model,
    writeBinary, writeXML)

# Middlelayer api
from .broker import get_connector
from .configuration import (
    config_changes, sanitize_init_configuration, sanitize_write_configuration)
from .device import Device, DeviceClientBase
from .device_client import (
    Queue, call, callNoWait, compareConfigurationsFromPast,
    compareDeviceConfiguration, compareDeviceWithPast, connectDevice,
    disconnectDevice, execute, executeNoWait, filterByTags, findDevices,
    findServers, get_utc_string, getClasses, getClassSchema, getClients,
    getConfiguration, getConfigurationFromName, getConfigurationFromPast,
    getDescriptors, getDevice, getDevices, getHistory, getInstanceInfo,
    getProperties, getSchema, getSchemaFromPast, getServers, getSystemInfo,
    getTimeInfo, getTopology, instantiate, instantiateFromName,
    instantiateNoWait, isAlive, listConfigurationFromName,
    listDevicesWithConfiguration, lock, saveConfigurationFromName, setNoWait,
    setWait, shutdown, shutdownNoWait, updateDevice, waitUntil, waitUntilNew,
    waitWhile)
from .device_interface import (
    listCameras, listDeviceInstantiators, listMotors, listMultiAxisMotors,
    listProcessors, listTriggers)
from .device_server import MiddleLayerDeviceServer
from .devicenode import DeviceNode
from .eventloop import (
    EventLoop, KaraboFuture, NoEventLoop, global_sync, synchronize,
    synchronize_notimeout)
from .logger import CacheLog
from .macro import (
    EventThread, Macro, MacroSlot, Monitor, RemoteDevice, TopologyMacro,
    run_macro)
from .pipeline import (
    Channel, InputChannel, NetworkInput, NetworkOutput, OutputChannel,
    OutputProxy, PipelineContext, PipelineMetaData, RingQueue)
from .proxy import (
    ProxyBase as Proxy, ProxyFactory, ProxyNodeBase as ProxyNode,
    ProxySlotBase as ProxySlot, SubProxyBase as SubProxy)
from .signalslot import Signal, SignalSlotable, coslot, slot
from .synchronization import (
    allCompleted, background, firstCompleted, firstException, gather,
    processEvents, sleep, synchronous)
from .unitutil import StateSignifier, maximum, minimum, removeQuantity
from .utils import AsyncTimer, get_property, profiler, set_property
