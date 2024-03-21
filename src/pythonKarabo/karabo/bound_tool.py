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
""" This module provides all the bound API names which a Device might need.
"""
import os

use_karathon = 'USE_KARATHON' in os.environ

if use_karathon:
    from karathon import (
        ADMIN, ALARM_ELEMENT, AMPERE, AMPERE_PER_SECOND, ATTO, ATTOSEC, BAR,
        BAYER, BECQUEREL, BGR, BGRA, BIT, BMP, BOOL_ELEMENT, BYTE,
        BYTEARRAY_ELEMENT, CANDELA, CENTI, CHOICE_ELEMENT, CHOICE_OF_NODES,
        CMYK, COMMAND, COULOMB, COUNT, DAY, DECA, DECI, DEGREE, DEGREE_CELSIUS,
        DOUBLE_ELEMENT, ELECTRONVOLT, EVERY_1MIN, EVERY_1S, EVERY_5S,
        EVERY_10MIN, EVERY_10S, EVERY_100MS, EVERY_EVENT, EXA, EXPERT, FARAD,
        FEMTO, FEMTOSEC, FLOAT_ELEMENT, GIGA, GRAM, GRAY, HECTO, HENRY, HERTZ,
        HOUR, IMAGEDATA_ELEMENT, INIT, INPUT_CHANNEL, INPUT_ELEMENT,
        INT32_ELEMENT, INT64_ELEMENT, INTERNAL, JOULE, JPEG, KATAL, KELVIN,
        KILO, LEAF, LIST_ELEMENT, LIST_OF_NODES, LUMEN, LUX, MANDATORY, MEGA,
        METER, METER_PER_SECOND, MICRO, MICROSEC, MILLI, MILLISEC, MINUTE,
        MOLE, NANO, NANOSEC, NDARRAY_ELEMENT, NEWTON, NO_ARCHIVING, NODE,
        NODE_ELEMENT, NONE, NOT_ASSIGNED, NUMBER, OBSERVER, OHM, ONESECOND,
        OPERATOR, OPTIONAL, OUTPUT_CHANNEL, OUTPUT_ELEMENT, OVERWRITE_ELEMENT,
        PASCAL, PATH_ELEMENT, PERCENT, PETA, PICO, PICOSEC, PIXEL, PNG,
        PROPERTY, RADIAN, READ, RGB, RGBA, SECOND, SIEMENS, SIEVERT,
        SLOT_ELEMENT, STATE_ELEMENT, STERADIAN, STRING_ELEMENT, TABLE_ELEMENT,
        TERA, TESLA, TIFF, TIME_UNITS, UINT32_ELEMENT, UINT64_ELEMENT,
        UNDEFINED, USER, VECTOR_BOOL_ELEMENT, VECTOR_CHAR_ELEMENT,
        VECTOR_DOUBLE_ELEMENT, VECTOR_FLOAT_ELEMENT, VECTOR_INT32_ELEMENT,
        VECTOR_INT64_ELEMENT, VECTOR_STRING_ELEMENT, VECTOR_UINT32_ELEMENT,
        VECTOR_UINT64_ELEMENT, VOLT, VOLT_PER_SECOND, WATT, WEBER, WRITE, YEAR,
        YOCTO, YOTTA, YUV, ZEPTO, ZETTA, AbstractInput, AccessLevel,
        AccessType, ArchivePolicy, AssemblyRules, AssignmentType,
        BinarySerializerHash, BinarySerializerSchema, Broker, Category,
        Channel, ChannelMetaData, ClassInfo, Connection, ConnectionStatus,
        DaqDataType, DAQPolicy, DateTimeString,
        DeviceClient as BoundDeviceClient, Dims, Encoding, Epochstamp,
        ErrorCode, EventLoop, Hash, HashAttributes, HashAttributesNode,
        HashFilter, HashMergePolicy, HashNode, ImageData, InputChannel,
        InputHash, InputSchema, LeafType, Logger, MetricPrefix, NodeType,
        OutputChannel, OutputHash, OutputSchema, Priority, PriorityLevel,
        Requestor, RollingWindowStatistics, Rotation, Schema, SignalSlotable,
        SignalSlotableIntern, Slot, SlotElementBase, TextSerializerHash,
        TextSerializerSchema, TimeDuration, Timestamp, Trainstamp, Types,
        TypesClass, Unit, Validator, ValidatorValidationRules, VectorHash,
        VectorHashPointer, _DimsIntern, fullyEqual, generateAutoStartHash,
        isStdVectorDefaultConversion, jsonToHash, loadFromFile,
        loadHashFromFile, loadSchemaFromFile, saveHashToFile, saveSchemaToFile,
        saveToFile, setDims, setStdVectorDefaultConversion, similar,
        startDeviceServer, stopDeviceServer)
else:
    from karabind import (
        ADMIN, ALARM_ELEMENT, AMPERE, AMPERE_PER_SECOND, ATTO, ATTOSEC, BAR,
        BAYER, BECQUEREL, BGR, BGRA, BIT, BMP, BOOL_ELEMENT, BYTE,
        BYTEARRAY_ELEMENT, CANDELA, CENTI, CHOICE_ELEMENT, CHOICE_OF_NODES,
        CMYK, COMMAND, COULOMB, COUNT, DAY, DECA, DECI, DEGREE, DEGREE_CELSIUS,
        DOUBLE_ELEMENT, ELECTRONVOLT, EVERY_1MIN, EVERY_1S, EVERY_5S,
        EVERY_10MIN, EVERY_10S, EVERY_100MS, EVERY_EVENT, EXA, EXPERT, FARAD,
        FEMTO, FEMTOSEC, FLOAT_ELEMENT, GIGA, GRAM, GRAY, HECTO, HENRY, HERTZ,
        HOUR, IMAGEDATA_ELEMENT, INIT, INPUT_CHANNEL, INPUT_ELEMENT,
        INT32_ELEMENT, INT64_ELEMENT, INTERNAL, JOULE, JPEG, KATAL, KELVIN,
        KILO, LEAF, LIST_ELEMENT, LIST_OF_NODES, LUMEN, LUX, MANDATORY, MEGA,
        METER, METER_PER_SECOND, MICRO, MICROSEC, MILLI, MILLISEC, MINUTE,
        MOLE, NANO, NANOSEC, NDARRAY_ELEMENT, NEWTON, NO_ARCHIVING, NODE,
        NODE_ELEMENT, NONE, NOT_ASSIGNED, NUMBER, OBSERVER, OHM, ONESECOND,
        OPERATOR, OPTIONAL, OUTPUT_CHANNEL, OUTPUT_ELEMENT, OVERWRITE_ELEMENT,
        PASCAL, PATH_ELEMENT, PERCENT, PETA, PICO, PICOSEC, PIXEL, PNG,
        PROPERTY, RADIAN, READ, RGB, RGBA, SECOND, SIEMENS, SIEVERT,
        SLOT_ELEMENT, STATE_ELEMENT, STERADIAN, STRING_ELEMENT, TABLE_ELEMENT,
        TERA, TESLA, TIFF, TIME_UNITS, UINT32_ELEMENT, UINT64_ELEMENT,
        UNDEFINED, USER, VECTOR_BOOL_ELEMENT, VECTOR_CHAR_ELEMENT,
        VECTOR_DOUBLE_ELEMENT, VECTOR_FLOAT_ELEMENT, VECTOR_INT32_ELEMENT,
        VECTOR_INT64_ELEMENT, VECTOR_STRING, VECTOR_STRING_ELEMENT,
        VECTOR_UINT32_ELEMENT, VECTOR_UINT64_ELEMENT, VOLT, VOLT_PER_SECOND,
        WATT, WEBER, WRITE, YEAR, YOCTO, YOTTA, YUV, ZEPTO, ZETTA, AccessLevel,
        AccessType, ArchivePolicy, AssemblyRules, AssignmentType,
        BinarySerializerHash, BinarySerializerSchema, Broker, Category,
        Channel, ChannelMetaData, ClassInfo, Connection, ConnectionStatus,
        DaqDataType, DAQPolicy, DateTimeString,
        DeviceClient as BoundDeviceClient, Dims, Encoding, Epochstamp,
        ErrorCode, EventLoop, Hash, HashAttributes, HashAttributesNode,
        HashFilter, HashMergePolicy, HashNode, ImageData, InputChannel,
        InputHash, InputSchema, LeafType, Logger, MetricPrefix, NodeType,
        OutputChannel, OutputHash, OutputSchema, Priority, PriorityLevel,
        Requestor, RollingWindowStatistics, Rotation, Schema, SignalSlotable,
        SignalSlotableIntern, Slot, SlotElementBase, TextSerializerHash,
        TextSerializerSchema, TimeDuration, Timestamp, Trainstamp, Types,
        TypesClass, Unit, Validator, ValidatorValidationRules, VectorHash,
        VectorHashPointer, _DimsIntern, cppNDArray, cppNDArrayCopy, fullyEqual,
        generateAutoStartHash, jsonToHash, loadFromFile, loadHashFromFile,
        loadSchemaFromFile, saveHashToFile, saveSchemaToFile, saveToFile,
        setDims, similar, startDeviceServer, stopDeviceServer)

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
