# AGENTS.cpp.md

This guide distills the patterns in the Karabo C++ framework into a reusable
playbook for writing and operating devices under `src/karabo`.

It is not a full API reference. It captures the conventions, design choices,
and implementation techniques that recur across the framework and that device
authors should treat as default practice.

## 1. Core Mental Model

- A Karabo C++ device is a `karabo::core::Device` subclass.
- The device schema is declared statically in `expectedParameters(Schema&)`.
- The schema is the contract: type, access, defaults, limits, units,
  descriptions, allowed states, channel structure, DAQ metadata, and visibility.
- Runtime state lives in the device configuration hash managed by `Device`.
- Device behavior is event-driven. Slots, timers, channel handlers, and remote
  callbacks run through Karabo communication and the shared event loop.
- For remote control there are no middlelayer proxies. The C++ equivalent is
  `karabo::core::DeviceClient`, either standalone or via `Device::remote()`.

Default rule: model a device as a static schema plus a runtime state machine,
and use `DeviceClient` as the standard high-level interface to other devices.

## 2. Primary Reference Files

- `src/karabo/core/Device.hh` and `src/karabo/core/Device.cc`
- `src/karabo/core/DeviceClient.hh` and `src/karabo/core/DeviceClient.cc`
- `src/karabo/devices/PropertyTest.hh` and `src/karabo/devices/PropertyTest.cc`
- `src/karabo/xms/InputChannel.hh` and `src/karabo/xms/InputChannel.cc`
- `src/karabo/xms/OutputChannel.hh` and `src/karabo/xms/OutputChannel.cc`
- `src/karabo/xms/ImageData.hh`

## 3. Device Skeleton

Use this as the baseline structure:

```cpp
class MyDevice : public karabo::core::Device {
   public:
    KARABO_CLASSINFO(MyDevice, "MyDevice", "my-package-" + karabo::util::Version::getVersion())

    static void expectedParameters(karabo::data::Schema& expected);

    explicit MyDevice(const karabo::data::Hash& config) : Device(config) {
        KARABO_INITIAL_FUNCTION(initialize);
        KARABO_SLOT(start);
        KARABO_SLOT(stop);
    }

   private:
    void initialize() {
        KARABO_ON_DATA("input", onData);
        KARABO_ON_EOS("input", onEndOfStream);
        updateState(karabo::data::State::NORMAL);
    }

    void start();
    void stop();
    void onData(const karabo::data::Hash& data, const karabo::xms::InputChannel::MetaData& meta);
    void onEndOfStream(const karabo::xms::InputChannel::Pointer& input);
};

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::Device, MyDevice)
```

Expect to define:

- schema properties, slots, nodes, tables, and channels in `expectedParameters`
- explicit `state` behavior, often via `OVERWRITE_ELEMENT(expected).key("state")`
- constructor-time registration of slots and signals
- `initialize()` for work that needs a fully constructed instance
- private helpers for actual device logic

## 4. Schema Rules

### Naming

- Public device properties and slots use `camelCase`.
- Internal helpers, local variables, and members use normal C++ naming rules.
- Device classes use `CamelCase`.
- Dotted names are for exposed hierarchy, e.g. `axis1.targetPosition`,
  `node.output`, `vectors.int32Property`.

### Preferred schema style

- Treat `expectedParameters()` as the source of truth for the static schema.
- Use schema builder macros instead of manual schema mutation:
  `BOOL_ELEMENT`, `INT32_ELEMENT`, `DOUBLE_ELEMENT`, `STRING_ELEMENT`,
  `VECTOR_*_ELEMENT`, `TABLE_ELEMENT`, `NODE_ELEMENT`, `SLOT_ELEMENT`,
  `OUTPUT_CHANNEL`, `INPUT_CHANNEL`, `NDARRAY_ELEMENT`, `IMAGEDATA_ELEMENT`,
  `OVERWRITE_ELEMENT`.
- Be explicit about access:
  - `.reconfigurable()` for runtime writes from clients
  - `.readOnly()` plus `.initialValue(...)` for device-owned feedback
  - `.init()` for init-only configuration
  - `.assignmentOptional()`, `.assignmentInternal()`, or other assignment
    semantics intentionally
- Use `allowedStates(...)` on callable slots.

### Always set the important metadata

For user-facing properties, define as many of these as are meaningful:

- `displayedName`
- `description`
- `defaultValue` or `initialValue`
- access mode
- units and metric prefix
- numeric limits
- vector or table size limits
- allowed states
- DAQ metadata where relevant

The schema is part of the operator contract. Be explicit rather than relying on
defaults where behavior matters.

## 5. State Model And Slot Flow Control

State is the main protection mechanism for slots and reconfiguration.

- Override or constrain `state` deliberately.
- Move the device into the correct initial state in `initialize()`.
- Gate slots with `allowedStates(...)`.
- Set a non-callable state before starting a long operation.
- Restore a stable state when the operation finishes or fails.

Typical pattern:

```cpp
void MyDevice::start() {
    updateState(State::STARTING);
    boost::asio::post(karabo::net::EventLoop::getIOService(),
                      karabo::util::bind_weak(&MyDevice::doStart, this));
}
```

And later:

```cpp
void MyDevice::doStart() {
    try {
        ...
        updateState(State::NORMAL);
    } catch (...) {
        updateState(State::ERROR, karabo::data::Hash("status", "start failed"));
        throw;
    }
}
```

Use separate target and actual properties when actions take time.

## 6. Execution Model And Non-Blocking Rules

Karabo C++ devices are event-driven and share the framework event loop.

Therefore:

- do not block slot handlers longer than necessary
- do not sleep in normal control paths unless you really mean to stall progress
- prefer `boost::asio::post`, timers, or dedicated worker logic for longer work
- keep slot implementations short and state-driven
- use `KARABO_INITIAL_FUNCTION(...)` rather than doing everything in the constructor

`PropertyTest` explicitly shows the right direction:

- channel handlers are registered in `initialize()`
- repeated output is timer-driven
- delayed slot replies use `AsyncReply`
- a comment in `slowSlot()` calls out that blocking is not the preferred pattern

## 7. Slots, Signals, And Replies

Slots are the externally callable actions of a device.

- Register runtime slots with `KARABO_SLOT(...)`.
- Expose public slots in schema with `SLOT_ELEMENT(...)`.
- If a slot should stay internal, register it but do not expose it in schema.
- Register custom signals with `KARABO_SIGNAL(...)`.
- `updateState(...)` implicitly replies with the new state if the slot has not
  replied yet.
- If the slot finishes asynchronously, use `SignalSlotable::AsyncReply` and
  reply explicitly when done.

Typical delayed reply pattern:

```cpp
void MyDevice::node_increment() {
    AsyncReply areply(this);
    set("node.counter", get<unsigned int>("node.counter") + 1);
    boost::asio::post(karabo::net::EventLoop::getIOService(),
                      karabo::util::bind_weak(&MyDevice::replier, this, areply));
}
```

### `AsyncReply` best practices

Use `AsyncReply` when a slot cannot finish in the current call stack.

- Create the `AsyncReply` in the slot handler before returning control.
- Pass it into the deferred callback by value.
- Complete it exactly once with either a success reply or an error reply.
- Prefer deferred completion through the event loop, a timer, or an async
  network/database callback.
- Do not block the slot thread while waiting for work that can complete
  asynchronously.

Preferred pattern:

```cpp
void MyDevice::startSlowAction() {
    AsyncReply areply(this);
    boost::asio::post(karabo::net::EventLoop::getIOService(),
                      karabo::util::bind_weak(&MyDevice::finishSlowAction, this, areply));
}

void MyDevice::finishSlowAction(const AsyncReply& areply) {
    areply("ok");
}
```

Practical rule: if the slot work needs another thread hop, timer callback, or
remote callback, use `AsyncReply` instead of sleeping or spinning inside the
slot.

### `bind_weak` best practices

`bind_weak` is the default lifetime guard for deferred callbacks in C++.

Use it for:

- `boost::asio::post(...)`
- timer callbacks such as `async_wait(...)`
- async request success and failure handlers
- any callback that captures `this` and may run after the current stack frame

Why:

- it suppresses the callback if the owning object is already being destroyed
- it avoids use-after-free when async work outlives the device

Preferred pattern:

```cpp
m_timer.async_wait(karabo::util::bind_weak(&MyDevice::onTimer, this,
                                           boost::asio::placeholders::error));
```

```cpp
boost::asio::post(karabo::net::EventLoop::getIOService(),
                  karabo::util::bind_weak(&MyDevice::finishSlowAction, this, areply));
```

Important rules:

- do not use `bind_weak` in the constructor; the object is not yet safely
  available through the weak/shared ownership path
- this is why `KARABO_INITIAL_FUNCTION(...)` and `initialize()` are the normal
  place for `KARABO_ON_DATA(...)`, timers, and other async registrations
- if a callback is guaranteed not to outlive the current call path, `bind_weak`
  may be unnecessary, but treat that as the exception
- if you need custom lifetime control, use a lambda with an explicit weak guard
  instead of a naked `this` capture

## 8. Properties, Timestamps, And Bulk Updates

- Use `set(key, value)` or `set(Hash(...))` for normal validated updates.
- Use `setNoValidate(...)` only when bypassing validation is intentional.
- Use `updateState(...)` instead of writing `state` manually.
- Use `setAlarmCondition(...)` instead of hand-writing the alarm field.
- `getActualTimestamp()` is the default source of timestamps for internal updates.
- Bulk updates through `set(Hash(...))` are the standard way to publish several
  changes atomically in one signal.

Use `preReconfigure(Hash&)` when incoming user changes must be adapted before
merge. `PropertyTest::preReconfigure(...)` mirrors writable properties into
corresponding readonly feedback fields and is the reference pattern.

## 9. `Hash` And `Schema` Handling

`Hash` and `Schema` are the two core data structures of the C++ Karabo API.

- `Hash` is the runtime container for configuration, state, messages, payloads,
  attributes, and topology data.
- `Schema` is the declarative description of valid `Hash` structure, access
  rules, limits, display metadata, channel layout, and tags.

Practical rule:

- build and exchange values as `Hash`
- describe and validate them as `Schema`

### `Hash` construction patterns

Use these three patterns most of the time.

#### Flat construction

```cpp
Hash update("status", "ready", "counter", 3, "enabled", true);
set(update);
```

This is the most compact way to publish several unrelated values at once.

#### Dotted-path construction

```cpp
Hash update;
update.set("node.counter", 3);
update.set("node.status", "ready");
update.set("timing.period", 0.1);
```

Use this when you already know the final path strings.

#### Explicit node construction

```cpp
Hash data;
Hash& node = data.bindReference<Hash>("node");
node.set("int32", 1);
node.set("string", "1");
node.set("vecInt64", std::vector<long long>(100, 1));
```

This is the preferred pattern for stream payloads and nested message trees.
`PropertyTest::writeOutput()` uses exactly this form.

### `Schema` handling rules

- Use `getFullSchema()` if decisions must include injected schema.
- Use static `expectedParameters(...)` for the stable contract and runtime
  injection only for genuinely dynamic cases.
- Use `OVERWRITE_ELEMENT(...)` to refine an existing path instead of trying to
  rebuild it manually.
- Use tag filtering through `HashFilter::byTag(...)` where filtered views of
  configuration are needed.
- Keep aliases, required access levels, tags, and DAQ metadata in schema, not
  in ad-hoc side tables.

### Tag filtering

Use tags as part of the schema contract:

```cpp
STRING_ELEMENT(expected)
      .key("hardwareName")
      .tags("hardware,poll")
      .assignmentOptional()
      .defaultValue("")
      .commit();
```

Filter configuration against tags through schema-aware filtering:

```cpp
Hash filtered = HashFilter::byTag(getFullSchema(), getConfiguration(),
                                  "hardware,poll", " ,;");
```

Practical rule: tags belong in schema and filtering should be schema-driven,
not implemented by string matching over paths.

### Construction of nodes

For exposed hierarchy, declare nodes explicitly in schema and then populate
matching `Hash` structures at runtime.

Schema side:

```cpp
NODE_ELEMENT(expected).key("timing").commit();
DOUBLE_ELEMENT(expected)
      .key("timing.period")
      .assignmentOptional()
      .defaultValue(0.1)
      .reconfigurable()
      .commit();
```

Runtime side:

```cpp
Hash update;
Hash& timing = update.bindReference<Hash>("timing");
timing.set("period", 0.1);
set(update);
```

For channel payloads, prefer nested node construction over repeated manual
dotted writes once the structure becomes non-trivial.

## 10. Nodes, Tables, And Structured Configuration

Use nodes and tables to model structure explicitly.

- Declare parent nodes with `NODE_ELEMENT(...)`.
- Put leaf properties under dotted paths such as `node.counter`.
- Use `TABLE_ELEMENT(...)` with a dedicated row schema.
- Use nested nodes for grouped device settings, grouped readback, and channel
  payload structure.

Typical table pattern:

```cpp
Schema row;
STRING_ELEMENT(row).key("name").assignmentOptional().defaultValue("").commit();
DOUBLE_ELEMENT(row).key("value").assignmentOptional().defaultValue(0.0).commit();

TABLE_ELEMENT(expected)
      .key("table")
      .setColumns(row)
      .assignmentOptional()
      .defaultValue(std::vector<Hash>())
      .reconfigurable()
      .commit();
```

## 11. Runtime Schema Injection

Karabo C++ supports runtime schema extension and modification, and `PropertyTest`
is the main reference pattern.

Use:

- `appendSchema(...)` to add or extend injected schema
- `updateSchema(...)` to replace the injected schema entirely
- `updateSchema(Schema())` to reset back to the static schema
- `appendSchemaMaxSize(...)` or `appendSchemaMultiMaxSize(...)` for max-size
  changes without rebuilding bigger schema fragments

Important behavior:

- injected schema is validated before merge
- defaults can be injected automatically
- `signalSchemaUpdated` is emitted
- input/output channels are created or recreated as required

### Schema injection recipe

```cpp
void MyDevice::slotUpdateSchema() {
    const Schema schema(getFullSchema());

    appendSchemaMaxSize("vectors.samples", schema.getMaxSize("vectors.samples") * 2, false);

    Schema injected;
    OUTPUT_CHANNEL(injected).key("output").commit();
    INT32_ELEMENT(injected)
          .key("injectedCounter")
          .assignmentOptional()
          .defaultValue(-1)
          .reconfigurable()
          .commit();

    appendSchema(injected);
}

void MyDevice::slotResetSchema() {
    updateSchema(Schema());
}
```

### Output channel recreation rule

If an output channel schema changes, explicitly include the output channel
itself in the injected schema:

```cpp
OUTPUT_CHANNEL(injected).key("output").commit();
```

Without that, consumers may not see the recreated channel schema when the real
intent was to change the channel contract.

## 12. Input Channels, Output Channels, And Pipeline Authoring

Declare channels in schema:

```cpp
INPUT_CHANNEL(expected).key("input").commit();
OUTPUT_CHANNEL(expected).key("output").dataSchema(pipeSchema).commit();
```

Rules:

- channels can live under nodes, e.g. `node.output`
- `Device::initChannels()` discovers them recursively through schema
- the runtime payload must match the declared output schema exactly
- `connectedOutputChannels` uses `<instanceId>:<channelName>`

Register handlers after construction:

```cpp
void MyDevice::initialize() {
    KARABO_ON_DATA("input", onData);
    KARABO_ON_EOS("input", onEndOfStream);
}
```

Do not register these in the constructor.

### Pipeline behavior rules

- `InputChannel.dataDistribution = copy` means every input receives every item
- `shared` means connected inputs share the stream
- `InputChannel.onSlowness` matters in `copy` mode: `drop`, `wait`,
  `queueDrop`
- `InputChannel.minData = 0` means collect until EOS
- `OutputChannel.noInputShared` controls behavior if no shared consumer can
  currently receive

### Producer pattern

```cpp
Hash data;
Hash& node = data.bindReference<Hash>("node");
node.set("counter", m_counter++);
writeChannel("output", data);
```

### EOS rule

End-of-stream is explicit:

```cpp
signalEndOfStream("output");
```

If a device is a pipeline stage, it must forward EOS deliberately instead of
assuming it propagates automatically.

## 13. Images, NDArrays, And DAQ-Facing Schema

Define image and array payloads in the channel schema itself.

Use:

- `NDARRAY_ELEMENT(...)` for raw arrays
- `IMAGEDATA_ELEMENT(...)` for image payloads

Minimum image contract:

```cpp
IMAGEDATA_ELEMENT(pipeData)
      .key("node.image")
      .setDimensions(std::vector<unsigned long long>({400, 500}))
      .setEncoding(Encoding::GRAY)
      .setType(Types::UINT16)
      .commit();
```

`ImageDataElement` also fills DAQ-relevant metadata such as pixel shape and
pixel storage schema. This is why it should be preferred over hand-rolled hash
structures for images.

### Output image recipe

```cpp
Schema pipeData;
NODE_ELEMENT(pipeData).key("node").setDaqDataType(karabo::data::DaqDataType::TRAIN).commit();

NDARRAY_ELEMENT(pipeData)
      .key("node.ndarray")
      .dtype(Types::FLOAT)
      .shape(std::vector<unsigned long long>({100, 200}))
      .commit();

IMAGEDATA_ELEMENT(pipeData)
      .key("node.image")
      .setDimensions(std::vector<unsigned long long>({400, 500}))
      .setEncoding(Encoding::GRAY)
      .setType(Types::UINT16)
      .commit();
```

```cpp
Hash data;
Hash& node = data.bindReference<Hash>("node");
node.set("ndarray", NDArray(Dims(100ull, 200ull), 1.0f));
node.set("image", ImageData(NDArray(Dims(400ull, 500ull),
                                    static_cast<unsigned short>(42)),
                            Dims(), Encoding::GRAY, 16));

writeChannel("output", data);
```

## 14. `safeNDArray` Rule

`writeChannel(...)` can avoid extra safety copies if the data buffer is known
to stay valid.

- default to `safeNDArray = false`
- set `safeNDArray = true` only when NDArray-backed memory will stay valid and
  unchanged until the framework is done with it
- if the same buffer will be reused or mutated after the call, do not set it

Practical rule: performance optimization here is secondary to data lifetime
correctness.

## 15. Remote Devices And `DeviceClient`

There are no middlelayer proxies in C++. Use `karabo::core::DeviceClient`.

Typical capabilities:

- instantiate and kill devices
- get and set properties
- execute slots
- query class, active, and device schema
- inspect output channel schemas
- register monitoring callbacks
- inspect topology

Inside a device, use `remote()` when available. Outside a device, create a
standalone `DeviceClient`.

## 16. Monitoring With `DeviceClient`

`DeviceClient` is the standard monitoring interface for remote devices.

Use:

- `registerDeviceMonitor(...)`
- `registerPropertyMonitor(...)`
- `registerSchemaUpdatedMonitor(...)`
- `registerChannelMonitor(...)` where channel monitoring is needed
- `registerDeviceForMonitoring(...)` and corresponding unregister functions

Practical rules:

- use property monitors for a few targeted leaves
- use device monitors for broader configuration change tracking
- use schema monitors when injected schema can change at runtime
- keep monitoring registration and unregistration explicit

## 17. Topology Tracking And Instance Handlers

For topology changes, use the `DeviceClient` instance tracking callbacks.

Pattern:

```cpp
client->registerInstanceNewMonitor(
    [this](const Hash& topologyEntry) { onInstanceNew(topologyEntry); });
client->registerInstanceUpdatedMonitor(
    [this](const Hash& topologyEntry) { onInstanceUpdated(topologyEntry); });
client->registerInstanceGoneMonitor(
    [this](const std::string& instanceId, const Hash& info) {
        onInstanceGone(instanceId, info);
    });
client->enableInstanceTracking();
```

Rules:

- register handlers before calling `enableInstanceTracking()`
- `instanceNew` is also used to report already-known instances once tracking
  becomes active
- use `instanceGone` to clean up cached device-specific state
- do not rely on `instanceUpdated` as the only topology source; combine it with
  snapshot queries when needed

`GuiServerDevice` is the reference pattern for this style of topology tracking.

### Throttled topology aggregation

If the application needs coalesced notifications rather than one callback per
change, use the aggregate change monitor pattern exposed by the client and keep
the immediate per-instance callbacks for stateful bookkeeping only when needed.

## 18. Configuration, History, And Operational Queries

`DeviceClient` also exposes:

- current and historical configuration access
- property history queries
- class discovery and topology inspection
- currently executable commands
- currently settable properties
- output channel name/schema inspection

Use these as the standard high-level control-plane interface rather than
re-implementing direct messaging.

## 19. Logging And Alarm Handling

- Use device logging consistently for startup, schema injection, channel
  creation, remote connection issues, and recoverable failures.
- Use `setAlarmCondition(...)` and `updateState(...)` to surface operational
  issues in the normal Karabo way.
- Avoid encoding alarm semantics only into free-text status messages.

## 20. Common Mistakes

- do not register `KARABO_ON_DATA(...)` or `KARABO_ON_EOS(...)` in the constructor
- do not write output payloads that only approximately match the declared schema
- do not inject only a leaf change when an output channel recreation is the goal
- do not assume `ImageData` or `NDArray` deep-copy payload memory
- do not call `writeChannel(...)` and `signalEndOfStream(...)` concurrently on
  the same channel
- do not post or register async callbacks with bare `this` when `bind_weak`
  should guard lifetime
- do not use blocking slot code where `AsyncReply` plus deferred completion is
  the correct pattern
- do not treat runtime schema injection as a replacement for a well-designed
  static schema

## 21. Recommended Default Architecture

For most new C++ devices, default to:

- static schema in `expectedParameters(...)`
- event-driven startup in `initialize()`
- explicit state machine updates through `updateState(...)`
- validated property changes through `set(...)`
- channel wiring through schema plus `KARABO_ON_DATA(...)`
- remote integration through `DeviceClient`
- schema injection only when genuinely dynamic behavior is required
