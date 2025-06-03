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
from karabind import (
    ALARM_ELEMENT, ATTOSEC, BOOL_ELEMENT, BYTEARRAY_ELEMENT, DOUBLE_ELEMENT,
    FEMTOSEC, FLOAT_ELEMENT, IMAGEDATA_ELEMENT, INPUT_CHANNEL, INT32_ELEMENT,
    INT64_ELEMENT, MICROSEC, MILLISEC, NANOSEC, NDARRAY_ELEMENT, NODE_ELEMENT,
    OUTPUT_CHANNEL, OVERWRITE_ELEMENT, PICOSEC, SLOT_ELEMENT, STATE_ELEMENT,
    STRING_ELEMENT, TABLE_ELEMENT, TIME_UNITS, UINT32_ELEMENT, UINT64_ELEMENT,
    VECTOR_BOOL_ELEMENT, VECTOR_CHAR_ELEMENT, VECTOR_DOUBLE_ELEMENT,
    VECTOR_FLOAT_ELEMENT, VECTOR_INT32_ELEMENT, VECTOR_INT64_ELEMENT,
    VECTOR_STRING, VECTOR_STRING_ELEMENT, VECTOR_UINT32_ELEMENT,
    VECTOR_UINT64_ELEMENT, AccessLevel, AccessType, ArchivePolicy,
    AssemblyRules, AssignmentType as Assignment, BinarySerializerHash,
    BinarySerializerSchema, Broker, Category, Channel, ChannelMetaData,
    ClassInfo, Connection, ConnectionStatus, DaqDataType, DateTimeString,
    DeviceClient as BoundDeviceClient, Dims, Encoding, Epochstamp, ErrorCode,
    EventLoop, Hash, HashAttributes, HashAttributesNode, HashFilter,
    HashMergePolicy, HashNode, ImageData, InputChannel, InputHash, InputSchema,
    Logger, MetricPrefix, NodeType, OutputChannel, OutputHash, OutputSchema,
    PriorityLevel, Requestor, Rotation, Schema, SignalSlotable,
    SignalSlotableIntern, Slot, SlotElementBase, TextSerializerHash,
    TextSerializerSchema, TimeDuration, TimeId, Timestamp, Types, TypesClass,
    Unit, Validator, ValidatorValidationRules, VectorHash, VectorHashPointer,
    _DimsIntern, cppNDArray, cppNDArrayCopy, fullyEqual, generateAutoStartHash,
    jsonToHash, loadFromFile, loadHashFromFile, loadSchemaFromFile,
    saveHashToFile, saveSchemaToFile, saveToFile, setDims, similar,
    startDeviceServer, stopDeviceServer)

from ..common.alarm_conditions import AlarmCondition
from ..common.states import State, StateSignifier
from .configurator import Configurator
from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from .device import PythonDevice
from .device_client import DeviceClient
from .device_server import DeviceServer, Launcher
from .plugin_loader import PluginLoader
from .runner import Runner
from .worker import Worker

# For comptibility with old karathon bindings, take care that e.g.
# str(Types.INT32) is 'INT32' and not 'Types.INT32'


def __typesToString(self):
    repStr = repr(self)  # i.e. '<Types.{this we want}: {enum value}>'
    return repStr[7:repStr.find(':')]


setattr(Types, "__str__", __typesToString)


class AbstractInput:
    pass


def isStdVectorDefaultConversion():
    return True


def setStdVectorDefaultConversion():
    pass
