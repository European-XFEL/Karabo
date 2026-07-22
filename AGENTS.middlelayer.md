# AGENTS.middlelayer.md

This guide distills the patterns in Karabo's middlelayer API into a reusable
playbook for writing and operating asynchronous Python devices under
`src/pythonKarabo/karabo/middlelayer`.

It is not a full API reference. It captures the conventions, design choices,
and implementation techniques that recur across the middlelayer API and that
device authors should treat as default practice.

## 1. When To Use This Guide

Use this file when the request explicitly concerns a middlelayer, middle
layer, MDL, or asynchronous descriptor-based Python device.

Strong code signals include:

- `from karabo.middlelayer import ...`
- imports from `karabo.middlelayer.*`
- `karabo.middlelayer.Device`, `DeviceClientBase`, or `Macro`
- class-level descriptors such as `String(...)`, `Int32(...)`, `Node(...)`,
  `VectorHash(...)`, and `Overwrite(...)`
- public `@Slot(...)` methods or internal `@slot` methods
- `@InputChannel(...)` handlers and `OutputChannel(...)` descriptors
- lifecycle hooks such as `preInitialization()`, `onInitialization()`, and
  `onDestruction()`
- remote helpers such as `getDevice(...)`, `connectDevice(...)`, `setWait(...)`,
  and `waitUntil(...)`
- asynchronous helpers such as `background(...)` and middlelayer `sleep(...)`

Do not use this guide for:

- C++ devices based on `karabo::core::Device`
- bound-Python devices based on `PythonDevice` and builder-style
  `expectedParameters(...)`
- code that merely uses generic names such as `Hash`, `State`, `String`,
  `Configurable`, or `async def` without a middlelayer-specific signal

## 2. Core Mental Model

- A middlelayer device is a `karabo.middlelayer.Device` subclass.
- Its static schema is declared with descriptors assigned to class attributes.
- Reusable schema sections are `Configurable` subclasses composed with
  `Node(...)`.
- The class hierarchy is the schema hierarchy. `Configurable` assembles
  descriptors across the method-resolution order, and `Overwrite(...)`
  refines inherited descriptors.
- Runtime property reads return `KaraboValue` objects carrying value,
  descriptor, timestamp, and, for numeric properties, unit information.
- Runtime property assignment goes through the descriptor, performs type and
  shape conversion, timestamps the value, and schedules a configuration
  update.
- Device behavior is asynchronous. Slots, lifecycle hooks, proxy operations,
  and pipeline handlers cooperate on Karabo's asyncio-based event loop.
- Remote devices are normally represented by schema-derived proxies obtained
  with `getDevice(...)` or `connectDevice(...)`.

Default rule: model a middlelayer device as a descriptor-defined schema plus
an asynchronous state machine, and use proxies for typed remote interaction.

## 3. Public API And Primary References

Import device-authoring APIs from `karabo.middlelayer`:

```python
from karabo.middlelayer import (
    AccessMode, Configurable, Device, Double, Node, Slot, State)
```

`src/pythonKarabo/karabo/middlelayer/__init__.py` defines the complete public
middlelayer API and explicitly advises callers to avoid deep imports. Deep
imports bind device code to implementation details and are rejected by the
minimal device template's import test. The main exception is dedicated test
support from `karabo.middlelayer.testing`.

Primary reference files are:

- `src/pythonKarabo/karabo/middlelayer/__init__.py`
- `src/pythonKarabo/karabo/middlelayer/device.py`
- `src/pythonKarabo/karabo/middlelayer/signalslot.py`
- `src/pythonKarabo/karabo/middlelayer/synchronization.py`
- `src/pythonKarabo/karabo/middlelayer/device_client.py`
- `src/pythonKarabo/karabo/middlelayer/proxy.py`
- `src/pythonKarabo/karabo/middlelayer/pipeline.py`
- `src/pythonKarabo/karabo/middlelayer/injectable.py`
- `src/pythonKarabo/karabo/native/schema/configurable.py`
- `src/pythonKarabo/karabo/native/schema/descriptors.py`
- `src/pythonKarabo/karabo/native/schema/ndarray.py`
- `src/pythonKarabo/karabo/native/schema/image_data.py`

Representative devices and tests are:

- `src/templates/middlelayer/minimal/`
- `src/pythonKarabo/karabo/middlelayer_devices/property_test.py`
- `src/pythonKarabo/karabo/middlelayer/tests/device_test.py`
- `src/pythonKarabo/karabo/middlelayer/tests/remote_test.py`
- `src/pythonKarabo/karabo/middlelayer/tests/remote_pipeline_test.py`
- `src/pythonKarabo/karabo/integration_tests/device_cross_test/test_cross.py`

## 4. Device Skeleton

Use this as the baseline shape for a stateful device with typed pipeline data:

```python
from asyncio import CancelledError, current_task
from contextlib import suppress

from karabo.middlelayer import (
    AccessMode, Configurable, Device, Double, InputChannel, Node,
    OutputChannel, Overwrite, Slot, State, String, Unit, background, sleep)


class Settings(Configurable):
    interval = Double(
        displayedName="Interval",
        description="Delay between samples",
        defaultValue=1.0,
        minExc=0.0,
        unitSymbol=Unit.SECOND,
        accessMode=AccessMode.RECONFIGURABLE)


class Sample(Configurable):
    value = Double(accessMode=AccessMode.READONLY)
    source = String(accessMode=AccessMode.READONLY)


class MyDevice(Device):
    __version__ = "1.0.0"

    state = Overwrite(
        defaultValue=State.INIT,
        options=[State.INIT, State.NORMAL, State.STARTING, State.STARTED,
                 State.STOPPING, State.ERROR])

    settings = Node(Settings, displayedName="Settings")

    actualValue = Double(
        displayedName="Actual value",
        defaultValue=0.0,
        accessMode=AccessMode.READONLY)

    output = OutputChannel(Sample, displayedName="Output")

    @InputChannel(displayedName="Input")
    async def input(self, data, meta):
        self.actualValue = data.value

    @Slot(displayedName="Start", allowedStates=[State.NORMAL])
    async def start(self):
        self.state = State.STARTING
        self._producer = background(self._produce())

    @Slot(displayedName="Stop", allowedStates=[State.STARTED])
    async def stop(self):
        self.state = State.STOPPING
        await self._stop_producer()

    async def onInitialization(self):
        self._producer = None
        self.state = State.NORMAL

    async def onDestruction(self):
        await self._stop_producer()

    async def _stop_producer(self):
        task = self._producer
        if task is None:
            return
        task.cancel()
        try:
            with suppress(CancelledError):
                await task
        finally:
            if self._producer is task:
                self._producer = None

    async def _produce(self):
        self.state = State.STARTED
        try:
            while True:
                self.output.schema.value = self.actualValue
                self.output.schema.source = self.deviceId
                await self.output.writeData()
                await sleep(self.settings.interval)
        except CancelledError:
            raise
        finally:
            if self._producer is current_task():
                self._producer = None
            self.state = State.NORMAL
```

Expect to define:

- class-level property, node, slot, and channel descriptors
- `__version__` from the device package version
- explicit state options and transitions
- lightweight construction through `super().__init__(configuration)`
- asynchronous startup in `onInitialization()`
- cancellation and cleanup in `onDestruction()`
- stored handles for long-lived background work

## 5. Class And Schema Assembly

Middlelayer schema assembly is descriptor-based, not builder-based.

- Each descriptor is assigned to a class attribute.
- `Configurable.__init_subclass__()` records descriptors and combines them
  across base classes.
- `getClassSchema()` builds the static schema from the class hierarchy.
- `getDeviceSchema()` builds the schema for an instance and can filter it for
  a state.
- Descriptor order follows class assembly order and becomes visible schema
  order.
- `Node(SomeConfigurable)` embeds the configurable's schema below that key.
- `Overwrite(...)` changes inherited descriptor attributes without replacing
  the inherited implementation accidentally.

Preferred inheritance pattern:

```python
class BaseMotor(Device):
    state = Overwrite(options=[State.INIT, State.NORMAL, State.ERROR])


class Motor(BaseMotor):
    state = Overwrite(
        defaultValue=State.INIT,
        options=[State.INIT, State.NORMAL, State.MOVING, State.ERROR])
```

Do not write `expectedParameters(...)` or use C++/bound builder elements in a
middlelayer device.

## 6. Descriptor And Schema Rules

Use the native descriptors re-exported by `karabo.middlelayer`:

- scalar types such as `Bool`, `Int32`, `UInt32`, `Float`, `Double`, and
  `String`
- vector types such as `VectorInt32`, `VectorDouble`, `VectorString`, and
  `VectorHash`
- structured types such as `Node`, `NDArray`, and `Image`
- executable and pipeline types such as `Slot`, `InputChannel`, and
  `OutputChannel`

Set access and assignment intentionally:

- `AccessMode.RECONFIGURABLE` for values clients may change at runtime
- `AccessMode.READONLY` for device-owned feedback
- `AccessMode.INITONLY` for configuration fixed after initialization
- `Assignment.MANDATORY` when instantiation must provide a value
- `Assignment.OPTIONAL` when a default or unset value is valid
- `Assignment.INTERNAL` for framework/device-owned initialization

Set meaningful operator metadata where relevant:

- `displayedName`
- `description`
- `defaultValue`
- `minInc`, `maxInc`, `minExc`, and `maxExc`
- `minSize` and `maxSize`
- `options` or `enum`
- `unitSymbol` and `metricPrefixSymbol`
- `allowedStates`
- `requiredAccessLevel`

The schema is the operator contract. Avoid relying on descriptor defaults when
access, assignment, range, state, or units are important to behavior.

## 7. Naming And Public Surface

- Device and `Configurable` classes use `CamelCase`.
- Public property and slot keys use `camelCase`.
- Private helpers and non-schema Python attributes use normal Python
  `snake_case` and a leading underscore where appropriate.
- Local node access uses Python attributes, for example
  `self.axis.targetPosition`.
- Network/configuration paths use dotted keys, for example
  `axis.targetPosition`.
- Device IDs follow the framework's allowed instance-name character set.

The minimal template's `property_naming_test.py` checks public key naming via
`check_device_package_properties(...)`. Keep that test in generated device
packages.

## 8. Properties With Custom Setters

A type descriptor can decorate a method to define external reconfiguration
behavior:

```python
class MyDevice(Device):
    actualValue = Double(
        defaultValue=0.0,
        accessMode=AccessMode.READONLY)

    @Double(
        displayedName="Target value",
        defaultValue=0.0,
        accessMode=AccessMode.RECONFIGURABLE,
        allowedStates=[State.NORMAL])
    async def targetValue(self, value):
        self.targetValue = value
        self.actualValue = value
```

Rules:

- The decorated method receives a converted `KaraboValue`.
- It may be synchronous or asynchronous.
- It should return `None`.
- Assign the accepted value to the decorated property explicitly; merely
  returning from the setter does not update it.
- Let validation errors propagate as `KaraboError` or another meaningful
  exception so the reconfiguration fails visibly.
- `allowedStates` and `AccessMode.RECONFIGURABLE` are checked before the
  setter runs.

The setter also runs while defaults and initial configuration are applied,
before `preInitialization()`. Guard unset values with `isSet(value)` and do not
assume proxies, pipelines, or hardware are ready. Direct device assignment,
such as `self.targetValue = value`, deliberately bypasses the custom setter,
access-mode checks, and allowed-state checks, so it does not recurse.

Setters from one multi-property reconfiguration may run concurrently. Do not
depend on their order or assume rollback after one setter has performed a side
effect. Use a setter for validation, normalization, mirroring, or hardware
writes only when it is safe during both initialization and runtime; otherwise
defer hardware work to `onInitialization()` or a slot.

## 9. Nodes, Tables, And Nested Structures

Define reusable nodes as `Configurable` classes:

```python
class Axis(Configurable):
    targetPosition = Double(defaultValue=0.0)
    actualPosition = Double(
        defaultValue=0.0,
        accessMode=AccessMode.READONLY)


class Stage(Device):
    axis = Node(Axis, displayedName="Axis")
```

Define tables with a row `Configurable` and `VectorHash`:

```python
class Row(Configurable):
    name = String(defaultValue="")
    value = Double(defaultValue=0.0)


class TableDevice(Device):
    readings = VectorHash(
        rows=Row,
        defaultValue=[],
        accessMode=AccessMode.READONLY)
```

Practical rules:

- Pass the `Configurable` class, not an instance, to `Node(...)` and
  `VectorHash(rows=...)`.
- Navigate nodes as attributes in device code.
- Use dotted keys in `Hash` objects and remote helper calls.
- A `VectorHash` row schema must be non-empty and use supported table leaf
  types.
- Use `TableValue` helpers such as `append`, `extend`, `clear`, `pop`,
  `default_row`, and `iter_hashes` instead of editing its backing structured
  numpy array directly.
- `default_row()` returns a safe deep copy; `iter_hashes()` does not copy.
- Build a new value or use a documented mutator when an in-place mutation
  would bypass descriptor publication.

## 10. Lifecycle Hooks

### Construction

Keep `__init__()` synchronous and lightweight:

```python
def __init__(self, configuration):
    super().__init__(configuration)
    self._worker = None
```

Do not connect devices, open pipelines, sleep, or perform slow hardware I/O in
the constructor. `Configurable` initialization validates and initializes
descriptor values, and the broker/event-loop context is not ready yet.

### `preInitialization()`

Use `preInitialization()` only for validation or preparation that does not
depend on the network.

- The framework wraps the hook in a five-second `wait_for` deadline.
- Device proxies and pipeline connections are not ready.
- `DeviceNode` connections have not been established.
- Raising aborts startup and shuts down the instance.

### `onInitialization()`

Use `onInitialization()` for asynchronous post-start setup:

- connect to remote devices
- wait for local or remote dependencies
- initialize hardware asynchronously
- set the stable initial state
- start stored background tasks

An uncaught exception is logged and causes the instance to shut down.
`onInitialization()` is scheduled after the startup gate and is not awaited by
the instantiation/start reply. `is_initialized` becomes true only after the
hook completes successfully, and `DeviceNode` initializers run before it. If
the hook fails, the device is killed, but the failure is not returned through
the already-completed startup call.

### `onDestruction()`

Use `onDestruction()` to cancel long-lived work and release resources.

- Keep it prompt; the framework wraps it in a five-second `wait_for` deadline.
- Cancel stored tasks.
- Disconnect resources that are not already owned by an async context.
- Close hardware connections and flush only bounded essential work.
- The framework subsequently stops tasks attached to the instance.

`onDestruction()` runs only after successful initialization. It is skipped if
`preInitialization()` fails, `onInitialization()` fails, or shutdown occurs
before initialization completes. Clean partially acquired initialization
resources with local `try/finally` blocks too. A `wait_for` deadline cannot
forcibly stop arbitrary blocking synchronous or native work, so keep lifecycle
hooks asynchronous and non-blocking.

`onException(slot, exception, traceback)` and `onCancelled(slot)` customize
capital-`@Slot` failure and cancellation handling. Unhandled instance-task
exceptions may call `onException(None, ...)`; these hooks are not the cleanup
mechanism for every background task or lowercase `@slot`, so use task
`finally` blocks and `onDestruction()`.

## 11. Async Execution And Task Ownership

Middlelayer runs on a shared asyncio-based Karabo event loop.

Therefore:

- prefer `async def` for slots, lifecycle hooks, and channel handlers
- `await` I/O instead of blocking the event-loop thread
- use `karabo.middlelayer.sleep(...)`, never `time.sleep(...)`, in device
  control paths
- keep public slots responsive and move continuing work into a tracked task
- store task handles when work needs explicit stop/restart behavior
- propagate `CancelledError` after local cleanup
- use `try/finally` to restore state and clear task references

Start continuing work with `background(...)`:

```python
self._poll_task = background(self._poll())
```

`background(...)` returns a cancellable future/task. It accepts a coroutine,
or a normal callable plus arguments. Normal callables run through the
framework's worker-thread machinery, which is appropriate for unavoidable
blocking APIs.

Cancellation of a normal callable running in the worker executor is
cooperative; blocking system or native calls cannot be forcibly stopped. The
current main-event-loop path also does not apply `background(..., timeout=...)`.
Use explicit `asyncio.wait_for(...)` for a real device-task deadline.

Prefer `self.create_instance_task(self._poll())` when ownership must be
explicit. It requires an actual coroutine object, not a coroutine function,
and returns a tracked asyncio task. Tasks created through the Karabo event loop
can inherit the current instance, but do not rely on that inference when code
may run outside a device-associated task.

Use the matching coordination primitive:

- `asyncio.gather(...)` for asyncio tasks/coroutines
- middlelayer `gather(...)` for `KaraboFuture` objects
- `firstCompleted(...)`, `allCompleted(...)`, or `firstException(...)` when
  named results, pending work, and errors must be separated

By default, the `first*`/`allCompleted` helpers cancel pending futures before
returning unless `cancel_pending=False` is passed.

## 12. State, Status, And Alarm Handling

Middlelayer updates state by assignment:

```python
self.state = State.STARTING
self.status = "Connecting to controller"
```

There is no middlelayer `updateState(...)` method.

Rules:

- refine the inherited `state` descriptor with `Overwrite(...)`
- include every state used by the implementation in `options`
- remember that `State.UNKNOWN` remains valid for remote-death semantics even
  when omitted from explicit options
- gate slots and reconfigurable properties with `allowedStates`
- set the transitional or busy state before the first `await` in a public
  slot; otherwise concurrent calls can both pass the `allowedStates` check
- restore a stable state in success, cancellation, and failure paths
- use separate target and actual properties for asynchronous hardware motion
- assign `alarmCondition` for machine-readable alarms; do not encode alarm
  semantics only in `status`
- use `self.logger` for diagnostic detail and keep `status` operator-focused

Typical failure shape:

```python
async def _connect_hardware(self):
    self.state = State.STARTING
    try:
        await self.controller.connect()
    except CancelledError:
        self.status = ""
        self.state = State.NORMAL
        raise
    except Exception:
        self.state = State.ERROR
        self.status = "Controller connection failed"
        raise
    else:
        self.status = ""
        self.state = State.NORMAL
```

## 13. Public Slots, Internal Slots, And Replies

Use `@Slot(...)` for operator-visible commands:

```python
@Slot(displayedName="Reset", allowedStates=[State.ERROR])
async def reset(self):
    await self._reset_hardware()
    self.state = State.NORMAL
    return True
```

Public `Slot` rules:

- a public slot is part of the schema
- it takes no remote arguments
- it may be synchronous or asynchronous
- a synchronous public slot runs through the framework's worker executor
- its return value becomes the reply
- an exception becomes an error reply and triggers normal exception handling
- `allowedStates` and device locks are enforced before execution
- nested `Configurable` nodes may contain `@Slot(...)` methods

Use lowercase `@slot` only for internal protocol/helper slots that must not
appear in the schema. Internal slots may accept arguments and are addressed by
name through the signal-slot layer.

Lowercase `@slot` does not automatically enforce schema access level,
`allowedStates`, or device locking. Its synchronous wrapper executes directly
in broker handling and must not block. Its exception wrapper logs and replies
directly rather than calling `onException()` or `onCancelled()` as capital
`@Slot` does. Reserve it for protocol and framework endpoints.

Middlelayer does not need the classic API's `AsyncReply` pattern. An
`async def` public slot remains pending until its coroutine returns, and its
eventual return value or exception is the reply. If the command should
acknowledge immediately and continue independently, start a tracked background
task and return deliberately.

## 14. Property Values, Timestamps, And Publication

Property reads return `KaraboValue` objects rather than plain Python values.

- use `.value` when a plain scalar, list, or numpy value is explicitly needed
- comparisons and arithmetic normally work directly on `KaraboValue`
- units and timestamps propagate through supported calculations
- `isSet(value)` distinguishes a real value from `NoneValue`
- enum-backed values compare with `==`, not identity-based assumptions

Assignment goes through descriptor validation:

```python
self.actualValue = measured_value
```

If a `KaraboValue` has no timestamp, assignment adds the current framework
timestamp. If it already carries a timestamp, the descriptor preserves it and
updates missing train information where appropriate.

Normal assignments are accumulated for publication on the event loop. Use
`self.update()` when several related assignments must be flushed before the
method continues:

```python
self.actualValue = value
self.status = "updated"
self.update()
```

`configurationAsHash()` creates a schema-shaped `Hash` containing current
values and timestamp attributes. `set(Hash(...))` bypasses custom setters,
while `await set_setter(Hash(...))` invokes them. Neither is the public
`slotReconfigure` path, and these helpers do not enforce the same access-mode
or allowed-state checks. Treat them as trusted internal mechanisms, not
substitutes for normal assignment or `setWait()`.

## 15. `Hash`, `Schema`, And Configuration Boundaries

Middlelayer device code normally works with typed attributes and proxies.
`Hash` and `Schema` remain important at protocol and introspection boundaries.

- use descriptors to declare the stable schema
- use `Hash` for raw channel payloads, generic configuration, topology, and
  APIs whose shape is not known at class definition time
- use `configurationAsHash()` to serialize a `Configurable`
- use `getSchema(...)`, `getClassSchema(...)`, or `getDeviceSchema()` for
  schema introspection
- use dotted paths in generic configuration hashes
- preserve timestamp attributes when manually transforming protocol hashes
- avoid maintaining a parallel hand-written schema for descriptor-defined
  properties

Example generic update:

```python
update = Hash()
update["axis.targetPosition"] = 2.5
await setWait("motor", "axis.targetPosition", 2.5)
```

## 16. Runtime Schema Injection

All devices inherit middlelayer's class-based injection support through
`InjectMixin`.

Add a descriptor to the instance's private runtime class, then publish it:

```python
async def injectDiagnostic(self):
    self.__class__.diagnostic = String(
        displayedName="Diagnostic",
        defaultValue="",
        accessMode=AccessMode.READONLY)
    await self.publishInjectedParameters()
    self.diagnostic = "ready"
```

Important differences from C++ and bound schema injection:

- injection modifies the per-instance class, not a separate builder schema
- inject only at the top level of that per-instance class
- injected descriptors may be simple properties, slots, nodes, or channels
- injection order determines schema order
- changing a class nested below an already injected node is not detected
- replace the top-level assignment and publish again to change such a
  structure
- later calls do not reinitialize previously injected descriptors
- positional key/value pairs override keyword values
- values supplied to `publishInjectedParameters()` are ignored for read-only
  descriptors; use their default or assign them after publication
- delete an injected class attribute and publish to remove it from the schema
- schema notification is scheduled; wait for remote visibility in tests

Use `Overwrite(...)` on the per-instance class when runtime refinement of an
inherited top-level descriptor is required.

To replace the schema of existing top-level output channels, use
`await self.setOutputSchema("output", NewPayloadClass)`. It closes and rebuilds
the channel so consumers reconnect with the new schema. Pass a `Configurable`
payload class, and note that nested output keys are not supported.

## 17. Input Channels

Declare a typed input by decorating its handler:

```python
@InputChannel(displayedName="Input", raw=False)
async def input(self, data, meta):
    self.actualValue = data.value


@input.connect
async def input(self, channel):
    self.logger.info("Connected to %s", channel)


@input.close
async def input(self, channel):
    self.logger.info("Disconnected from %s", channel)


@input.endOfStream
async def input(self, channel):
    await self.output.writeEndOfStream()
```

Rules:

- `raw=False` creates a schema-derived object for each payload
- `raw=True` delivers the uninterpreted `Hash` and is required when the sender
  has no declared data schema
- the data handler receives `(data, meta)`
- `meta.source` identifies `<deviceId>:<channelName>`
- `meta.timestamp` is a true `BoolValue` carrying the packet `Timestamp` in
  `meta.timestamp.timestamp`
- connect, close, data, and EOS handlers may be synchronous or asynchronous
- handlers on one input are serialized and protected by the channel lock
- each handler has a five-second timeout; start background work if processing
  cannot finish promptly
- configured connections use `<deviceId>:<channelName>` in
  `connectedOutputChannels`
- `missingConnections` reports configured outputs that are currently absent
- `connectChannel(...)` is the runtime programmatic connection API
- input channels reconnect automatically while the output remains configured

Connection policy is configured on the input:

- `dataDistribution="copy"` offers each item independently to every copy
  consumer, subject to that consumer's `onSlowness` policy
- `dataDistribution="shared"` divides items among shared consumers
- copy-mode `onSlowness="drop"` discards when the consumer is not ready
- `onSlowness="queueDrop"` uses a bounded ring and drops the oldest item
- `onSlowness="wait"` applies backpressure
- `maxQueueLength` bounds the `queueDrop` queue

Choose the policy as part of the device's data-loss and backpressure contract.

## 18. Output Channels And EOS

Declare a typed output with a `Configurable` payload class:

```python
class Payload(Configurable):
    value = Double(accessMode=AccessMode.READONLY)


class Producer(Device):
    output = OutputChannel(Payload)

    async def _send(self, value):
        self.output.schema.value = value
        await self.output.writeData()
```

For an output without a schema, write a raw `Hash`:

```python
output = OutputChannel()

await self.output.writeRawData(Hash("value", 42))
```

Rules:

- `writeData()` requires a declared output schema
- `writeRawData(hash)` requires a `Hash` and is the raw-channel API
- pass `timestamp=Timestamp(...)` when the sample needs an explicit pipeline
  timestamp
- awaited writes participate in `wait`-policy backpressure by waiting for
  queue admission, but they do not acknowledge TCP delivery or prove that
  every connection has serialized the payload
- `writeDataNoWait()` and `writeRawDataNoWait()` enqueue without backpressure
  and require deliberate buffer-lifetime and ordering decisions
- send EOS explicitly with `await output.writeEndOfStream()`
- a pipeline stage must forward EOS deliberately
- EOS does not call the normal data handler, clear the last value, or close the
  stream; later data is permitted
- serialize writes, schema replacement, and EOS for the same channel

Typed runtime payloads must conform to the declared descriptor schema; use a
raw output for undeclared or dynamically shaped `Hash` payloads.

## 19. `NDArray`, `Image`, And Buffer Lifetime

Declare arrays and images in the payload schema:

```python
import numpy as np

from karabo.middlelayer import (
    Configurable, DaqDataType, Encoding, Image, NDArray, UInt16)


class ImagePayload(Configurable):
    daqDataType = DaqDataType.TRAIN

    array = NDArray(dtype=UInt16, shape=(1024, 1024))
    image = Image(
        dtype=UInt16,
        shape=(1024, 1024),
        encoding=Encoding.GRAY)
```

Rules:

- provide `dtype` and `shape` when declaring an `NDArray`
- a zero in a declared shape dimension permits any extent in that dimension
- assignment validates dtype and shape and converts compatible inputs
- `Image` carries pixel data plus encoding, dimensions, bit depth, ROI,
  binning, rotation, and flip metadata
- assign either a compatible numpy array or an `ImageData` value to `Image`
- set DAQ data type on the containing `Configurable` where appropriate

NDArray conversion and `ImageData` construction do not guarantee a deep copy
of an already compatible numpy array. Pipeline chunks may also be serialized
after they are queued. If the producer will mutate or reuse the source buffer,
hand the property an explicit copy or allocate a fresh buffer for the next
sample. This matters especially for no-wait writes and queued/wait consumers.
Do not treat return from `writeData()` as proof that every connection has
serialized the underlying array. If immediate mutation or reuse is possible,
assign a dedicated copy before writing. Middlelayer `writeData()` has no
`safeNDArray` flag.

## 20. Remote Devices And Proxy Lifetime

Use `getDevice(...)` for a scoped proxy:

```python
async with getDevice("motor") as motor:
    await setWait(motor, targetPosition=10.0)
    await motor.start()
    await waitUntil(lambda: motor.state == State.MOVING)
```

Equivalent established code also uses:

```python
with (await getDevice("motor")) as motor:
    ...
```

A proxy is generated from the remote schema. Outside a connection context,
its configuration is only a one-shot snapshot after the short creation-time
subscription grace; lasting live configuration updates require a context or
an explicit connection.

Use `connectDevice(...)` for a longer-lived subscription:

```python
motor = await connectDevice("motor")
try:
    ...
finally:
    await disconnectDevice(motor)
```

Rules:

- prefer a context when the proxy is needed only in one operation
- explicitly disconnect a long-lived proxy when its owner no longer needs it
- do not mix `getDevice` and incompatible `connectDevice` proxy factories for
  the same cached device ID
- use `updateDevice(proxy)` for a one-shot refresh when the proxy is not
  connected
- use `isAlive(proxy)` to inspect remote liveness
- expect in-flight proxy operations to fail with `KaraboError` if the remote
  device disappears
- a returning device causes its cached proxy schema and configuration to be
  refreshed
- do not cache descriptor objects across remote schema injection or restart

## 21. Remote Reads, Writes, Calls, And Locks

Proxy properties are typed `KaraboValue` objects:

```python
position = motor.actualPosition
```

Proxy assignment is convenient and batches changes on the event loop:

```python
motor.targetPosition = 10.0
```

Use `setWait(...)` when the caller must wait for reconfiguration
acknowledgement:

```python
await setWait(motor, targetPosition=10.0, velocity=2.0)
await setWait("motor", "targetPosition", 10.0)
```

Use the operation whose completion semantics match the workflow:

- `await proxy.slotName()` or `await execute(...)` waits for slot completion
- `executeNoWait(...)` sends a public slot request without waiting
- `call(...)` invokes a named internal/protocol slot and may pass arguments
- `callNoWait(...)` is its fire-and-forget form
- `setWait(...)` waits for reconfiguration acknowledgement
- `setNoWait(...)` does not

For exclusive control, use the regular context manager returned by the async
`lock(...)` helper:

```python
async with getDevice("motor") as motor:
    with (await lock(motor)):
        await setWait(motor, targetPosition=10.0)
```

Keep the proxy connected and the protected block bounded. Lock acquisition can
wait indefinitely, so add an external timeout when needed. Device public slots
and reconfiguration enforce `lockedBy`; do not implement locking by manually
writing that property.

Apply a deadline as `await asyncio.wait_for(lock(proxy), timeout=...)`. In
async code, context exit schedules the unlock but cannot wait for its
acknowledgement; when the next action depends on release, await
`waitUntil(lambda: proxy.lockedBy == "")` after leaving the block.
`wait_for_release=False` is explicitly fire-and-forget.

## 22. Waiting And Monitoring Through Proxies

Use event-driven waiting rather than polling sleeps:

```python
async with getDevice("motor") as motor:
    await waitUntil(lambda: motor.state == State.NORMAL)
    await waitWhile(lambda: motor.state == State.MOVING)
```

Rules:

- every proxy referenced by `waitUntil(...)` or `waitWhile(...)` must be
  connected while waiting
- wrap waits with `asyncio.wait_for(...)` when the operation has a deadline
- use `waitUntilNew(property)` to wait for the next update rather than merely
  for a predicate to become true
- use `Queue(proxy.property)` when every successive property update matters
- keep the `Queue` object alive because its registration uses weak references
- `updateDevice(proxy)` fetches current configuration but is not continuous
  monitoring
- `proxy.setConfigHandler(...)` is experimental and test-oriented; do not
  build stable device architecture around it

Pipeline monitoring is separate. Configure handlers on an output proxy before
calling `proxy.output.connect()`, and call `disconnect()` when finished. Output
proxy connections always use copy/drop; use a real `InputChannel` when full
distribution and slowness control is required.

## 23. Topology, Discovery, And `DeviceClientBase`

A plain `Device` can use normal proxy helpers, but it does not maintain the
full system topology required by discovery helpers. Inherit
`DeviceClientBase` when the device must continuously track devices, servers,
macros, or clients:

```python
class Supervisor(DeviceClientBase):
    async def onInitialization(self):
        self.state = State.NORMAL
```

`DeviceClientBase` maintains `systemTopology` from instance-new, updated, and
gone notifications.

Use:

- `getTopology()` for a defensive deep copy of the current topology
- `getDevices()`, `findDevices(...)`, `getServers()`, `findServers(...)`, and
  `getClients()` for discovery
- `getClasses(serverId)` for server plugin availability
- `getInstanceInfo(...)`, `getSystemInfo(...)`, and `getTimeInfo(...)` for
  operational introspection
- `instantiate(...)` and `shutdown(...)` for lifecycle control

Discovery is eventually consistent; wait for membership rather than asserting
immediately after an instance starts. Do not mutate a returned topology and
expect framework state to change.

Do not inherit `DeviceClientBase` merely to control one known remote device;
the full-topology subscription has additional startup and network cost.

## 24. History And Persistent Configurations

Middlelayer exposes high-level helpers for services outside the target device:

- `getHistory(...)`
- `getConfigurationFromPast(...)`
- `getSchemaFromPast(...)`
- `getInitConfiguration(...)`
- `listInitConfigurations(...)`
- `saveInitConfiguration(...)`
- `instantiateDevice(...)`

`getHistory(...)` accepts a proxy property or a dotted
`"<deviceId>.<propertyPath>"` string. Use the string form when the device is
offline. Sort returned rows by timestamp before order-sensitive assertions.
A bare device ID or bare proxy is not a valid history target. The current
implementation returns three-tuples of
`(timestamp_seconds, isLast, value)`—despite an older function docstring, no
train-ID field is returned. Date strings are parsed with `dateutil`; naive
times are interpreted in the local timezone and converted to UTC.

These calls depend on data logger and configuration manager services. Treat
timeouts, missing history, absent logger mappings, and service-side errors as
normal operational failures. Prefer the current `*InitConfiguration` and
`instantiateDevice` names over deprecated `*FromName` aliases.

## 25. Macros

`Macro` is a specialized middlelayer `Device` and uses the same descriptors,
proxies, async helpers, and lifecycle concepts.

- normal `@Slot(...)` macro actions automatically maintain passive/active
  state, `currentSlot`, cancellation, and exception hooks
- customize macro-wide states with `abstractPassiveState` and
  `abstractActiveState`
- use `@MacroSlot(...)` for a long action that should start in the background
  and return immediately
- declare static remote dependencies with `RemoteDevice(...)` when its managed
  startup and monitoring semantics are required
- use `@Monitor()` to recompute a read-only property from declared remotes;
  keep `@Monitor()` outside the property descriptor decorator
- use `TopologyMacro` when a macro needs full topology discovery

A positive `RemoteDevice` timeout logs a missing dependency and lets startup
continue; `timeout <= 0` removes the timeout and may leave initialization
waiting indefinitely. Any update from any declared remote recomputes all
`@Monitor` properties. Monitor exceptions are logged and watching continues.

Do not copy macro-specific slot state machinery into ordinary devices.

## 26. Testing Middlelayer Devices

Use `AsyncDeviceContext` for device tests:

```python
import pytest

from karabo.middlelayer import State
from karabo.middlelayer.testing import AsyncDeviceContext

from my_package.my_device import MyDevice


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_device():
    device = MyDevice({"deviceId": "TestMyDevice"})
    async with AsyncDeviceContext(device=device) as ctx:
        assert ctx.instances["device"] is device
        assert device.is_initialized
        assert device.state == State.NORMAL
```

The minimal template installs `KaraboTestLoopPolicy` as the pytest event-loop
policy. Follow the repository's current pytest-asyncio loop-scope
configuration rather than creating unmanaged event loops in individual tests.

Test at the right layer:

- descriptor/schema tests for metadata, defaults, and state filtering
- direct device tests for lifecycle and internal state changes
- proxy tests for network-visible behavior and acknowledgements
- pipeline tests for typed/raw payloads, EOS, reconnection, and timestamps
- cross-API integration tests when C++ or bound compatibility is in scope

Prefer `waitUntil(...)`, `waitUntilNew(...)`, or test-only
`sleepUntil(...)` over fixed sleeps. Use unique device IDs so parallel tests do
not collide on the broker. Assert the expected initialized state explicitly.
`AsyncDeviceContext` waits up to its timeout but does not raise merely because
initialization did not finish; assert `is_initialized` or state unless a
stalled startup is the behavior under test.

Useful focused commands include:

```text
py.test -q --pyargs -c src/pythonKarabo/pyproject.toml karabo.middlelayer.tests.device_test
py.test -q --pyargs -c src/pythonKarabo/pyproject.toml karabo.middlelayer.tests.remote_test
py.test -q --pyargs -c src/pythonKarabo/pyproject.toml karabo.middlelayer.tests.remote_pipeline_test
```

Both focused networked tests and the repository runner require an activated
Karabo environment and a configured, running broker:

```text
./run_python_tests.sh --runUnitTests --rootDir /path/to/Framework
```

Use `auto_build_all.sh` when the change requires the full build/test wrapper.
Keep the template tests that enforce public property/slot naming and reject
unsupported deep middlelayer imports.

## 27. Packaging And Running

A middlelayer device package is a normal Python package with a
`karabo.middlelayer_device` entry point:

```toml
[project.entry-points."karabo.middlelayer_device"]
MyDevice = "my_package.my_device:MyDevice"
```

During development, install the package in Karabo's Python environment with
an editable install or `karabo develop <package>`. Run it through a
middlelayer server, for example:

```text
karabo-middlelayerserver serverId=middleLayerServer/1 deviceClasses=MyDevice
```

Use the package version as the device class `__version__` so `classVersion`
identifies the deployed implementation.

## 28. Logging And Error Handling

- log through `self.logger`, which is configured with the device ID
- raise `KaraboError` for expected Karabo-facing validation or operational
  failures
- allow unexpected slot exceptions to reach framework handling so callers get
  an error reply and the traceback is logged
- catch exceptions only when adding recovery, state cleanup, or meaningful
  context
- do not accidentally swallow `CancelledError` in running work; clean up and
  re-raise it. A shutdown owner may suppress it only after deliberately
  cancelling and awaiting the owned task
- put concise operator guidance in `status` and detailed diagnostics in logs
- use state and `alarmCondition` for machine-readable fault semantics

Background-task exceptions are logged by `background(...)`, but a device that
depends on the task should also observe its completion and move to an
appropriate state.

## 29. Common Mistakes

- do not mix middlelayer descriptors with bound `expectedParameters(...)`
  builders
- do not import device-authoring APIs from deep `karabo.middlelayer.*`
  modules when the public package exports them
- do not perform network or pipeline work in `__init__()` or
  `preInitialization()`
- do not call `time.sleep(...)` on the event-loop thread
- do not leave long-lived tasks untracked or uncancelled
- do not accidentally suppress `CancelledError` in running work
- do not use lowercase `@slot` for a command that must appear in schema
- do not add arguments to a public `@Slot(...)`
- do not assume assigning to a proxy has already been acknowledged; use
  `setWait(...)` when acknowledgement matters
- do not use `waitUntil(...)` with an unconnected proxy
- do not keep `getDevice(...)` proxies subscribed without a context or an
  explicit lifecycle plan
- do not use `writeData()` on a schema-less output or `writeRawData()` as a
  shortcut around a declared schema
- do not forget to forward EOS in a pipeline stage
- do not mutate or reuse numpy buffers while queued pipeline serialization may
  still reference them
- do not expect nested mutations below an injected node to change the runtime
  schema automatically
- do not use discovery helpers from a plain `Device` when full topology is
  required; use `DeviceClientBase`
- do not encode alarms only in free-form status text

## 30. Recommended Default Architecture

For most new middlelayer devices, default to:

- class-level descriptors for the static schema
- small `Configurable` classes composed with `Node(...)`
- `Overwrite(...)` for inherited schema refinement
- a lightweight constructor
- validation-only `preInitialization()` when needed
- asynchronous dependency setup in `onInitialization()`
- explicit state transitions and state-gated public slots
- attribute assignment for validated local property publication
- tracked and cancellable background tasks for continuing work
- scoped proxies from `getDevice(...)` for remote control
- `DeviceClientBase` only when full topology tracking is required
- typed input/output channels and explicit EOS propagation
- class-based injection only for genuinely dynamic top-level schema
- `AsyncDeviceContext` tests that verify behavior through the public API
