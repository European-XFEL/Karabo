# Karathon Middlelayer API Reference


```python
import karabo.middlelayer
```

## The Karabo Hash Dictionary

Convert Karabo hashes to Python dictionaries and back.

::: karabo.middlelayer.Hash

::: karabo.middlelayer.hashToDict

::: karabo.middlelayer.dictToHash

## Accessing Remote Devices with Proxies

Core functionality for interacting with devices via _proxies_.

::: karabo.middlelayer.getDevice

::: karabo.middlelayer.connectDevice

::: karabo.middlelayer.updateDevice

::: karabo.middlelayer.isAlive

## Accessing Remote Devices Directly

A low-overhead API for short-lived interactions (no proxy). Note: state
inspection still requires proxies for efficiency.

::: karabo.middlelayer.setWait

::: karabo.middlelayer.setNoWait

::: karabo.middlelayer.execute

::: karabo.middlelayer.executeNoWait

## Running Devices on Servers

Instantiate or shut down devices remotely, with optional fire‑and‑forget.

::: karabo.middlelayer.instantiate

::: karabo.middlelayer.instantiateNoWait

::: karabo.middlelayer.shutdown

::: karabo.middlelayer.shutdownNoWait

## Inspecting Device Servers

Discover running devices and servers from the command line or within
`DeviceClientBase`.

::: karabo.middlelayer.findDevices

::: karabo.middlelayer.findServers

::: karabo.middlelayer.getClients

::: karabo.middlelayer.getDevices

::: karabo.middlelayer.getServers

::: karabo.middlelayer.getClasses

::: karabo.middlelayer.getTopology

## Working with Devices

Retrieve current configuration, schema, or instance info.

::: karabo.middlelayer.getConfiguration

::: karabo.middlelayer.getSchema

::: karabo.middlelayer.getInstanceInfo

## Configurations and Timestamps

### Historic Configuration

Retrieve past configurations or schemas by timestamp.

::: karabo.middlelayer.getConfigurationFromPast

::: karabo.middlelayer.getSchemaFromPast

### Named Configurations

Manage configurations by name:

::: karabo.middlelayer.getConfigurationFromName

::: karabo.middlelayer.getConfiguration

::: karabo.middlelayer.listConfigurationFromName

::: karabo.middlelayer.listDevicesWithConfiguration

::: karabo.middlelayer.instantiateFromName

::: karabo.middlelayer.saveConfigurationFromName

### Comparing Configurations

Utilities for comparing device configurations:

::: karabo.middlelayer.compareDeviceWithPast

::: karabo.middlelayer.compareDeviceConfiguration

::: karabo.middlelayer.minutesAgo

::: karabo.middlelayer.hoursAgo

::: karabo.middlelayer.daysAgo

## Utility Functions

Convenience helpers:

::: karabo.middlelayer.get_property

::: karabo.middlelayer.set_property

::: karabo.middlelayer.get_array_data

::: karabo.middlelayer.get_image_data

::: karabo.middlelayer.maximum

::: karabo.middlelayer.minimum

::: karabo.middlelayer.removeQuantity

::: karabo.middlelayer.profiler

::: karabo.middlelayer.validate_args

::: karabo.middlelayer.get_descriptor_from_data

::: karabo.middlelayer.get_hash_type_from_data

::: karabo.middlelayer.is_equal

::: karabo.middlelayer.has_changes

::: karabo.middlelayer.simple_deepcopy

## Writing a Device

Base classes and exceptions for device authors:

::: karabo.middlelayer.Configurable

::: karabo.middlelayer.Device

::: karabo.middlelayer.DeviceClientBase

::: karabo.middlelayer.KaraboError

## Synchronization

Asynchronous helpers and primitives:

::: karabo.middlelayer.background

::: karabo.middlelayer.gather

::: karabo.middlelayer.firstCompleted

::: karabo.middlelayer.allCompleted

::: karabo.middlelayer.firstException

::: karabo.middlelayer.sleep

::: karabo.middlelayer.KaraboFuture

::: karabo.middlelayer.lock

## Synchronization Primitives

Blocking until conditions are met:

::: karabo.middlelayer.waitUntil

::: karabo.middlelayer.waitUntilNew

## Karabo Descriptors

### Descriptor Categories

Abstract base classes for Karabo types:

::: karabo.middlelayer.Type

::: karabo.middlelayer.Vector

::: karabo.middlelayer.NumpyVector

::: karabo.middlelayer.Simple

::: karabo.middlelayer.Number

::: karabo.middlelayer.Integer

::: karabo.middlelayer.Enumable

### Atomic Descriptors

Primitive data types:

::: karabo.middlelayer.String

::: karabo.middlelayer.VectorChar

::: karabo.middlelayer.Double

::: karabo.middlelayer.Float

::: karabo.middlelayer.Int16

::: karabo.middlelayer.Int32

::: karabo.middlelayer.Int64

::: karabo.middlelayer.Int8

::: karabo.middlelayer.UInt16

::: karabo.middlelayer.UInt32

::: karabo.middlelayer.UInt64

::: karabo.middlelayer.UInt8

::: karabo.middlelayer.Bool

::: karabo.middlelayer.VectorDouble

::: karabo.middlelayer.VectorFloat

::: karabo.middlelayer.VectorInt16

::: karabo.middlelayer.VectorInt32

::: karabo.middlelayer.VectorInt64

::: karabo.middlelayer.VectorInt8

::: karabo.middlelayer.VectorUInt16

::: karabo.middlelayer.VectorUInt32

::: karabo.middlelayer.VectorUInt64

::: karabo.middlelayer.VectorUInt8

::: karabo.middlelayer.VectorBool

::: karabo.middlelayer.VectorString

::: karabo.middlelayer.Char

### Compound Descriptors

Structured types:

::: karabo.middlelayer.Image

::: karabo.middlelayer.Node

::: karabo.middlelayer.VectorHash

### Special Descriptors

Advanced or regex‑based types:

::: karabo.middlelayer.DeviceNode

::: karabo.middlelayer.VectorRegexString

::: karabo.middlelayer.RegexString

## Karabo Data Types

Values and quantifiable types:

::: karabo.middlelayer.KaraboValue

::: karabo.middlelayer.isSet

::: karabo.middlelayer.QuantityValue

::: karabo.middlelayer.StringValue

::: karabo.middlelayer.VectorStringValue

::: karabo.middlelayer.TableValue

::: karabo.middlelayer.EnumValue

::: karabo.middlelayer.BoolValue

::: karabo.middlelayer.ImageData

::: karabo.middlelayer.NoneValue

::: karabo.middlelayer.Timestamp
