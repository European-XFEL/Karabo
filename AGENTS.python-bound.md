# AGENTS.python-bound.md

This guide distills the patterns in the bound Python API into a reusable
playbook for writing and operating devices under `src/pythonKarabo/karabo/bound`.

It is not a full API reference. It captures the conventions, design choices,
and implementation techniques that recur across the bound API and that device
authors should treat as default practice.

## 1. When To Use This Guide

Use this file when the code contains bound-Python buzzwords such as:

- `from karabo.bound import ...`
- `from karabind import ...`
- `PythonDevice`
- `@KARABO_CLASSINFO(...)`
- `@KARABO_CONFIGURATION_BASE_CLASS`
- `registerInitialFunction(...)`
- `registerSlot(...)` or `KARABO_SLOT(...)`
- `KARABO_ON_DATA(...)`, `KARABO_ON_INPUT(...)`, `KARABO_ON_EOS(...)`
- `Hash`, `Schema`, `ImageData`, `OutputChannel`, `InputChannel`
- `karabo.bound.DeviceClient`

Do not use this guide for:

- C++ framework code under `src/karabo`
- middlelayer devices using `karabo.middlelayer.Device`, async slots, or
  proxies

## 2. Core Mental Model

- A bound-Python device is a `PythonDevice` subclass.
- The device schema is declared statically in
  `expectedParameters(expected: Schema)`.
- Schema assembly is builder-based, not descriptor-based as in middlelayer.
- Runtime values live in a validated `Hash` held by `PythonDevice`.
- Device behavior is event-driven and runs on the shared C++ event loop plus
  worker threads managed by the framework.
- There are no middlelayer proxies here. Remote access is done with
  `karabo.bound.DeviceClient`, either created directly or via `self.remote()`.

Default rule: treat bound Python as the Python spelling of the classic
Karabo device model, not as middlelayer.

## 3. Primary Bound API Surface

The core surface is:

- `karabo.bound.PythonDevice`
- `karabo.bound.DeviceClient`
- `karabo.bound.Configurator`
- `karabo.bound.KARABO_CLASSINFO`
- `karabo.bound.KARABO_CONFIGURATION_BASE_CLASS`
- schema builders from `karabind`, for example `STRING_ELEMENT`,
  `INT32_ELEMENT`, `NODE_ELEMENT`, `SLOT_ELEMENT`, `INPUT_CHANNEL`,
  `OUTPUT_CHANNEL`, `NDARRAY_ELEMENT`, `IMAGEDATA_ELEMENT`,
  `OVERWRITE_ELEMENT`
- core data types such as `Hash`, `Schema`, `ImageData`, `Timestamp`,
  `State`, `AlarmCondition`

## 4. Device Skeleton

Use this as the baseline structure:

```python
from karabo.bound import (
    INPUT_CHANNEL, INT32_ELEMENT, KARABO_CLASSINFO, NODE_ELEMENT,
    OUTPUT_CHANNEL, SLOT_ELEMENT, STRING_ELEMENT, Hash, PythonDevice, Schema,
    State)


@KARABO_CLASSINFO("MyBoundDevice", "1.0")
class MyBoundDevice(PythonDevice):
    @staticmethod
    def expectedParameters(expected):
        pipe = Schema()
        (
            INT32_ELEMENT(pipe)
            .key("value")
            .assignmentOptional().defaultValue(0)
            .commit(),

            STRING_ELEMENT(expected)
            .key("statusText")
            .readOnly().initialValue("")
            .commit(),

            SLOT_ELEMENT(expected).key("send").commit(),
            INPUT_CHANNEL(expected).key("input").commit(),
            OUTPUT_CHANNEL(expected).key("output").dataSchema(pipe).commit(),
        )

    def __init__(self, configuration):
        super().__init__(configuration)
        self.registerInitialFunction(self.initialize)
        self.registerSlot(self.send)
        self.KARABO_ON_DATA("input", self.onData)
        self.KARABO_ON_EOS("input", self.onEndOfStream)

    def initialize(self):
        self.updateState(State.NORMAL)

    def send(self):
        self.writeChannel("output", Hash("value", 1))

    def onData(self, data, meta):
        self.set("statusText", f"got {meta['source']}")

    def onEndOfStream(self, channel):
        self.set("statusText", "input EOS")
```

Expect to define:

- `@KARABO_CLASSINFO(...)` on every device class
- a static `expectedParameters(...)`
- constructor-time slot registration and handler registration
- `registerInitialFunction(...)` for startup work that must happen after
  framework construction
- explicit state changes through `updateState(...)`

## 5. Class Registration And Schema Assembly

Bound Python uses the configurator and decorators rather than C++ macros.

- Base configurable classes use `@KARABO_CONFIGURATION_BASE_CLASS`.
- Derived device classes use `@KARABO_CLASSINFO(...)`.
- `Configurator` assembles the final schema by walking base classes from base
  to derived and calling `expectedParameters(...)` in that order.
- `PythonDevice.getSchema(classId)` is the standard assembled schema entry
  point.

Default rule: keep `expectedParameters(...)` as the single source of truth for
the static device contract.

## 6. Schema Rules

- Use builder macros from `karabind`, not manual `Hash` edits of schema.
- Define every public property, slot, node, table, and channel in
  `expectedParameters(...)`.
- Set access explicitly:
  - `.reconfigurable()` for runtime writes
  - `.readOnly().initialValue(...)` for device-owned feedback
  - `.init()` for init-only configuration
  - `.assignmentOptional()`, `.assignmentMandatory()`, or
    `.assignmentInternal()` intentionally
- Use `allowedStates(...)` on slots and state-sensitive properties.
- Use `OVERWRITE_ELEMENT(...)` to refine inherited schema instead of
  re-declaring the same leaf.
- Use `appendParametersOf(...)` when a node should embed another configurable
  schema.

Always set meaningful operator metadata where relevant:

- `displayedName`
- `description`
- `defaultValue` or `initialValue`
- bounds
- units and metric prefix
- tags
- access level
- DAQ metadata

## 7. Lifecycle, Event Loop, And State

Bound devices are not asyncio devices. They use Karabo's shared event loop and
framework threads.

- Keep slots short.
- Do not block the event loop with long synchronous work.
- Put post-construction setup into `registerInitialFunction(...)`.
- Use `updateState(...)`, not `set("state", ...)`.
- Use `preReconfigure(...)`, `postReconfigure()`, and `preDestruction()` as
  the lifecycle hooks for adaptation, follow-up work, and cleanup.

Typical pattern:

```python
def initialize(self):
    self.updateState(State.NORMAL)

def preReconfigure(self, incoming):
    if incoming.has("targetValue"):
        incoming.set("actualValue", incoming["targetValue"])
```

## 8. Slots, Replies, And Naming

- Register public slots with `self.registerSlot(...)` or `self.KARABO_SLOT(...)`.
- Expose them in schema with matching `SLOT_ELEMENT(...)`.
- For node-local slot keys such as `node.increment`, the Python method name can
  use `_` and the framework maps `.` and `_` compatibly.
- Slots can take arguments, but keep signatures strict and intentional.
- Use `self.reply(...)` for explicit replies when the slot returns data.
- For delayed slot completion, use `self.signalSlotable.createAsyncReply()`.
- `updateState(...)` also publishes the state as the default reply.

Practical rule: public control slots should be schema-declared and state-gated;
internal helper slots may be registered without adding them to schema.

### `AsyncReply` best practices

The bound API supports asynchronous slot replies through
`SignalSlotable.createAsyncReply()`.

- Create the async reply object inside the slot.
- Complete it later from posted work, a timer callback, or an async request
  handler.
- Complete it exactly once.
- Use `.error(message, details)` for error replies.
- Prefer `EventLoop.post(...)` when the completion should happen after the slot
  returns.

Preferred pattern:

```python
from karabo.bound import EventLoop


def startSlowAction(self):
    areply = self.signalSlotable.createAsyncReply()

    def finish():
        areply("ok")

    EventLoop.post(finish)
```

Error reply pattern:

```python
def startSlowAction(self):
    areply = self.signalSlotable.createAsyncReply()

    def fail():
        areply.error("Failed", "details")

    EventLoop.post(fail)
```

Important rule: do not rely on a direct same-stack async-reply call inside the
slot body. When in doubt, post the completion to the event loop.

## 9. Property Updates And Validation

- Use `self.set(...)` for normal validated updates.
- Use `self[key] = value` only as a shorthand for `set`.
- `set(...)` accepts:
  - `Hash`
  - `key, value`
  - `key, value, Timestamp`
  - keyword arguments for flat properties
- `State` and `AlarmCondition` are converted correctly when set directly.
- Use `setVectorUpdate(...)` for concurrent vector mutation instead of
  hand-editing a fetched vector and writing it back blindly.
- `slotReconfigure(...)` validates external updates and re-validates any
  `preReconfigure(...)` modifications before applying them.

## 10. `Hash` And `Schema` Handling

`Hash` and `Schema` are the main data structures in the bound API as well.

- `Hash` is the runtime container for configuration, payloads, topology data,
  timestamps, and attributes.
- `Schema` defines what `Hash` structures are valid and how they should be
  exposed.

Preferred `Hash` construction patterns:

```python
update = Hash("status", "ready", "counter", 3)
```

```python
update = Hash()
update.set("node.counter", 3)
update.set("node.status", "ready")
```

```python
payload = Hash({"data": {"value": 42}})
```

Important handling rules:

- `self.get(key)` returns copies for `Hash` and `VectorHash` to avoid
  back-door mutation of device state.
- Use `getFullSchema()` before making schema-driven decisions.
- Use `getCurrentConfiguration(tags="...")` or `filterByTags(...)` when tag
  filtering is the intent.
- Use `HashFilter.byTag(schema, configuration, tags, " ,;")` for explicit tag
  filtering logic.
- Use `getAliasFromKey(...)`, `getKeyFromAlias(...)`, `aliasHasKey(...)`, and
  `keyHasAlias(...)` when alias-aware code is required.

## 11. Nodes, Tables, And Nested Structures

- Use dotted keys for exposed hierarchy, for example `node.counter`.
- Declare parent nodes explicitly with `NODE_ELEMENT(...)` when you want a
  structured subtree.
- Use `TABLE_ELEMENT(...).setNodeSchema(rowSchema)` for tables.
- Use `appendParametersOf(...)` to embed reusable structured schema sections.
- For nested channel payloads, build nested `Hash` values with dotted paths or
  nested Python dictionaries.

Practical rule: model hierarchy in schema first, then construct runtime hashes
to match it exactly.

## 12. Runtime Schema Injection

Bound Python supports the same major schema injection mechanisms as the C++
framework:

- `updateSchema(schema)` replaces the currently injected schema and merges it
  over the static schema
- `appendSchema(schema)` adds to the current injected schema
- `appendSchemaMaxSize(path, value, emitFlag=True)` updates vector or table
  max size efficiently
- `appendSchemaMultiMaxSize(paths, values)` updates several max sizes at once
- `updateSchema(Schema())` resets back to the static schema

Important behavior:

- injected defaults are validated and applied
- newly injected channels are created automatically
- re-injected input channels keep registered handlers
- output channels are recreated when their schema changes so downstream users
  see the new `.schema`

Use this pattern for additive injection:

```python
def injectSchema(self):
    schema = Schema()
    (
        STRING_ELEMENT(schema).key("word1")
        .assignmentOptional().defaultValue("Hello")
        .reconfigurable()
        .commit(),

        NODE_ELEMENT(schema).key("injectedNode").commit(),

        OUTPUT_CHANNEL(schema).key("output").commit(),

        INT32_ELEMENT(schema).key("injectedNode.counter")
        .assignmentOptional().defaultValue(0)
        .commit(),
    )
    self.appendSchema(schema)
```

Use `OUTPUT_CHANNEL(schema).key("output").commit()` when the intent is to force
recreation of an existing output channel after schema changes beneath it.

## 13. Input Channels, Output Channels, And Pipelines

Declare channels in schema:

```python
INPUT_CHANNEL(expected).key("input").commit()
OUTPUT_CHANNEL(expected).key("output").dataSchema(pipeSchema).commit()
```

Register handlers in the constructor or initial setup:

```python
self.KARABO_ON_DATA("input", self.onData)
self.KARABO_ON_INPUT("input", self.onInput)
self.KARABO_ON_EOS("input", self.onEndOfStream)
```

Use the handlers intentionally:

- `KARABO_ON_DATA(...)` for item-by-item processing
- `KARABO_ON_INPUT(...)` when the device should explicitly drain the input
  channel itself
- `KARABO_ON_EOS(...)` for end-of-stream logic

Wiring rules:

- `connectedOutputChannels` uses `<deviceId>:<channelName>`
- it is init-only configuration, not a normal runtime property
- channel connection policy is expressed on the input side

Writing output:

```python
payload = Hash("e", 5.0, "s", "hello")
self.writeChannel("output", payload)
```

Pipeline rules:

- channel payload must match the declared schema exactly when a schema exists
- call `signalEndOfStream("output")` explicitly when a pipeline stage ends
- do not call `writeChannel(...)` and `signalEndOfStream(...)` concurrently on
  the same channel
- if you use the low-level `OutputChannel` API directly, call `write(...)`
  and then `update()` or `asyncUpdate(...)`
- if you use the low-level `InputChannel` API directly, register data, input,
  EOS, and connection tracker handlers explicitly

## 14. Images, NDArrays, And `safeNDArray`

Define array and image payloads in channel schema, not only at runtime.

```python
NDARRAY_ELEMENT(pipe).key("ndarray").dtype(Types.FLOAT).shape("10").commit()

IMAGEDATA_ELEMENT(pipe).key("image") \
    .setDimensions("50,50") \
    .setType(Types.UINT16) \
    .setEncoding(Encoding.GRAY) \
    .commit()
```

Write runtime values with numpy arrays and `ImageData`:

```python
arr = numpy.full((10,), 42.0, dtype=numpy.float32)
img = numpy.full((50, 50), 42, dtype=numpy.uint16)
payload = Hash("ndarray", arr, "image", ImageData(img, encoding=Encoding.GRAY))
self.writeChannel("output", payload)
```

Important data-handling rules:

- `ImageData` does not imply a deep copy by default
- C-contiguous numpy arrays can be shared without copying more easily than
  non-contiguous arrays
- non-C-contiguous arrays may be copied into C-order internally
- `safeNDArray=True` should only be used when the underlying array memory
  stays valid and unchanged after `writeChannel(...)`
- if you will mutate or reuse the same numpy buffer immediately, do not use
  `safeNDArray=True`

## 15. Remote Devices And `DeviceClient`

There are no bound-Python proxies. Use `DeviceClient`.

Inside a device:

```python
remote = self.remote()
value = remote.get(deviceId, "node.counter")
remote.set(deviceId, "node.counter", 10)
remote.execute(deviceId, "node.increment", 5)
```

Outside a device:

```python
client = DeviceClient()
client.instantiate(serverId, "PropertyTest", Hash("deviceId", deviceId), 5)
```

Use the sync and async APIs intentionally:

- sync: `instantiate`, `get`, `set`, `execute`, `killDevice`
- fire-and-forget or polling style: `instantiateNoWait`, `setNoWait`,
  `executeNoWait`, `killDeviceNoWait`
- schema/config introspection: `getDeviceSchema`, `getActiveSchema`,
  `getClassSchema`, `getOutputChannelSchema`, `getDeviceSchemaNoWait`
- discovery: `getServers`, `getDevices`, `getClasses`, `exists`

Practical rule: inside a slot, avoid synchronous call chains back to the
current caller if that would deadlock ordering; prefer posted work or no-wait
operations where appropriate.

## 16. Monitoring, Topology Tracking, And Introspection

Use `DeviceClient` monitors instead of middlelayer proxy monitoring.

Topology tracking:

```python
client = DeviceClient()

def on_instance_new(entry):
    ...

def on_instance_updated(entry):
    ...

def on_instance_gone(instance_id, info):
    ...

client.registerInstanceNewMonitor(on_instance_new)
client.registerInstanceUpdatedMonitor(on_instance_updated)
client.registerInstanceGoneMonitor(on_instance_gone)
client.enableInstanceTracking()
```

Register the callbacks before calling `enableInstanceTracking()`.

Configuration and schema monitoring:

- `registerSchemaUpdatedMonitor(handler)`
- `registerPropertyMonitor(deviceId, key, handler)`
- `registerDeviceMonitor(deviceId, handler)`
- `registerDeviceForMonitoring(deviceId)` when you need schema updates to keep
  arriving over longer periods
- `unregisterPropertyMonitor(...)`, `unregisterDeviceMonitor(...)`,
  `unregisterDeviceFromMonitoring(...)` when monitoring is no longer needed

System snapshots:

- `getSystemTopology()`
- `getSystemInformation()`
- `getProperties(deviceId)`
- `getCurrentlyExecutableCommands(deviceId)`
- `getCurrentlySettableProperties(deviceId)`
- `getOutputChannelNames(deviceId)`

Default rule: combine callbacks for change events with explicit snapshot calls
for initial state, and do not rely on `instanceUpdated` alone as the complete
source of topology truth.

## 17. History, Persistence, And Operational Queries

`DeviceClient` also exposes operational queries beyond live control:

- `getPropertyHistory(...)`
- `getConfigurationFromPast(...)`
- `listInitConfigurations(...)`
- `saveInitConfiguration(...)`

These depend on the relevant Karabo services being available. Handle timeouts
and failed replies as normal outcomes, not exceptional surprises.

## 18. Common Mistakes

- Do not confuse bound Python with middlelayer. `PythonDevice` is not an
  asyncio device.
- Do not mutate a `Hash` returned by `self.get(...)` and assume it updates
  device state.
- Do not call `set("state", ...)`; use `updateState(...)`.
- Do not forget to register slots in the constructor.
- Do not register channel handlers only in schema; they must be registered in
  code.
- Do not inject only a leaf change when you actually need an output channel
  recreation; touch the channel itself in the injected schema.
- Do not assume `ImageData` deep-copies numpy data.
- Do not use synchronous remote call chains in slots when sender ordering can
  block the reply path.
- Do not block a slot when a delayed result should be sent with
  `createAsyncReply()`.
- Do not call an async reply from the same slot stack when a posted completion
  is the safe event-loop pattern.

## 19. Recommended Default Architecture

For most bound devices, default to this shape:

- static schema in `expectedParameters(...)`
- post-construction setup via `registerInitialFunction(...)`
- explicit state machine through `updateState(...)`
- validated property updates through `set(...)`
- remote interaction through `self.remote()` and `DeviceClient`
- channel-based data flow through `INPUT_CHANNEL`, `OUTPUT_CHANNEL`,
  `KARABO_ON_DATA`, and `writeChannel(...)`
- runtime extension through `appendSchema(...)` or `updateSchema(...)` only
  when the static schema truly cannot cover the use case
