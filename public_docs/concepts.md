# Key Concepts

## Devices

In Karabo, the core concept is the **device**. Karabo comprises multiple
devices running across a network, where each device is an instance of an
object‑oriented class that implements the device interface and logic.  
Each device provides exactly one logical, encapsulated service
(1 device = 1 service).

### What a Device Can Represent

- **I/O channel**  
  e.g. a digital output for switching something on or off  
- **Equipment**  
  e.g. a motor or a pump  
- **Controller**  
  e.g. a pump controller driving multiple pumps  
- **Composite component**  
  e.g. a slit formed by two motors  
- **Software algorithm**  
  e.g. image processing  
- **Service connection**  
  e.g. data archive reader, calibration database adapter

Devices hide all implementation details of the underlying service and expose
a standardized interface.

---

## Device Identifiers

Each device must have a **unique** instance ID. Karabo disallows starting a
second device with an existing ID anywhere in its managed topology.

- IDs must be non‑empty strings.
- Allowed characters:  
  - Uppercase and lowercase English letters (A–Z, a–z)  
  - Digits (0–9)  
  - Special characters: `_`, `/`, `-`
- **Preferred format**:  
  ```text
  <domain>/<type>/<member>
  ```

# Device Implementation

Devices are _classes_ with _methods_ and _properties_. All devices inherit
from a base class in the respective API, providing common functionality for
inter-device communication, data types, self-description and logging.

## Device Slots

_Device slots_ are like member functions exposed to all other devices. They
can take up to four arguments (of Karabo data types; for more, use a `Hash`)
and return zero to four Karabo-known types. Slots exposed in the
self-description (and GUI) take no arguments. As _commands_, they should
return the device’s _state_ after execution.

## Call & Request/Reply Patterns

Karabo uses a signal/slot mechanism for (inter-)device communication.
The low-level interface is part of the Device API in C++ and Python.
Exceptions thrown during synchronous remote slot calls propagate; for
asynchronous calls you can specify a failure handler.

### Call Pattern (Fire-and-Forget)

Use when no return value is expected:

```python
class RemoteDevice(PythonDevice):
    def __init__(self, configuration):
        super().__init__(configuration)
        self.KARABO_SLOT(self.foo)

    def foo(self, a):
        self.log.INFO(a)

# Caller:
self.call("a/remote/device", "foo", 1)
```

> **Note:** A _global call_ uses `"*"` instead of a device ID to call any
device’s slot. It is fire-and-forget: no reply or failure is reported.

### Request/Reply Pattern

Use when replies are needed.

#### 1. Synchronous Request

Blocks until reply or exception (timeout optional):

```python
class RemoteDevice(PythonDevice):
    def initialization(self):
        self.KARABO_SLOT(self.bar)

    def bar(self, b):
        c = b + 1
        self.reply(c)

# Caller:
result = self.request("/a/remote/device", "bar", 1).waitForReply(1000)
```

#### 2. Asynchronous Request

Returns immediately; callback invoked on completion:

```python
# Caller:
def onBar(self, response):
    self.log.INFO(response)

self.request("a/remote/device", "bar", 2).receiveAsync(self.onBar)
```

In C++:

```cpp
string txt("The answer is: ");
request("some/device/1", "slotFoo", 21)
  .receiveAsync<int>(
    bind_weak(&onReply, this, txt, _1),
    bind_weak(&onError, this)
  );

void onReply(const std::string& arg1, int arg2) {
    std::cout << arg1 << arg2 << std::endl; // "The answer is: 42"
}

void onError() {
    try { throw; }
    catch (const std::exception& e) {
        std::cout << "Error calling 'slotFoo': " << e.what() << std::endl;
    }
}

// On the replying device:
void slotFoo(const int arg1) {
    reply(arg1 + arg1);
}
```

> **Note:** `karabo::util::bind_weak` protects the callback from being invoked
on a destroyed `this`, while not preventing its destruction if uninvoked.

### Signals

Attach methods to signals emitted by other devices:

```python
class RemoteDevice(PythonDevice):
    def initialization(self):
        self.registerSignal("foo", int)
    def bar(self):
        self.emit("foo", 1)

class Receiver1(PythonDevice):
    def initialization(self):
        self.KARABO_SLOT(self.onFoo)
        self.connect("remote/device/1", "foo", "", "onFoo")
    def onFoo(self, a):
        self.log.INFO(a)

class Receiver2(PythonDevice):
    def initialization(self):
        self.KARABO_SLOT(self.onBar)
        self.connect("remote/device/1", "foo", "", "onBar")
    def onBar(self, b):
        self.log.INFO(b + 1)
```

## Technical Implementation

Every device is a client to a central message broker. Devices subscribe using
their IDs; the broker routes request/reply messages by device ID. Each request
carries a unique request ID for blocking/unblocking or callback lookup.

## Device Properties

> This section covers the C++ and bound Python APIs; middle-layer devices
simplify property access.

Properties are like public members: class variables exposed to other devices.
Defined statically in the `expectedParameters` section, they form part of the
device’s _Schema_. Defining a property implies:

- It is readable (`get`) and optionally writable (`set`) in the distributed
system, given proper access rights.
- The `(device ID, property key)` is unique across the system.
- The GUI can display and (if writable) edit it.
- It is available to middle-layer devices and macros via proxies.
- It can be serialized in Karabo’s formats.

Properties use Karabo data types (see **Karabo Data Types**).

Example definition:

```python
@staticmethod
def expectedParameters(expected):
    (
      STRING_ELEMENT(expected).key("stringProperty")
        .displayedName("A string property")
        .assignmentMandatory()
        .commit(),
      UINT32_ELEMENT(expected).key("integerProperty")
        .displayedName("An integer property")
        .assignmentOptional().defaultValue(1)
        .commit(),
    )
```

### Node Elements

Group properties hierarchically using _node_ elements. Requesting a node
returns a `Hash` of its children.

- **Choice of nodes** (select one type):

  ```python
  @staticmethod
  def expectedParameters(expected):
      (
        CHOICE_ELEMENT(expected).key("connection")
          .appendNodesOfConfigurationBase(ConnectionBase)
          .commit(),
      )
  ```

- **List of nodes** (multiple entries):

  ```python
  @staticmethod
  def expectedParameters(expected):
      (
        LIST_ELEMENT(expected).key("categories")
          .appendNodesOfConfigurationBase(CategoryBase)
          .commit(),
      )
  ```

## Device Version

Each device configuration declares the Karabo Framework version and its
package version for automated logging of the software configuration.

## Device Hooks

Karabo devices provide common hooks (Python & C++, not middle‑layer):

- `preReconfigure(incomingReconfiguration)`: Modify or validate incoming
  configuration (a Karabo `Hash`) before applying.
- `postReconfigure()`: After new configuration is applied.
- `preDestruction()`: Before device instance destruction—for cleanup.
- `onTimeUpdate(trainId, sec, frac, period)`: On timing system updates.
- `registerInitialFunction(func)`: Call `func` after initialization (after
  initial property values are set).

## Events vs. Polling on Bound Devices

Bound devices may receive hardware values via events or by polling. In both
cases, new values are made available by `set`‑ting the corresponding property
(an atomic, blocking operation). To poll hardware, use a worker thread:

```python
from karabo.bound.worker import Worker
from karabo.bound.decorators import KARABO_CLASSINFO
from karabo.bound.device import PythonDevice, launchPythonDevice
from ._version import version as deviceVersion

@KARABO_CLASSINFO("HardwarePollingDevice", deviceVersion)
class HardwarePollingDevice(PythonDevice):
    def __init__(self, configuration):
        super().__init__(configuration)
        self.pollWorker = None
        self.registerInitialFunction(self.initialization)

    def preDestruction(self):
        if self.pollWorker and self.pollWorker.is_running():
            self.pollWorker.stop()
            self.pollWorker.join()
            self.pollWorker = None

    @staticmethod
    def expectedParameters(expected):
        (
          INT32_ELEMENT(expected).key("polledValue")
            .readOnly().noInitialValue()
            .commit(),
          # ...
        )

    def initialization(self):
        if not self.pollWorker:
            timeout = 1000  # ms
            self.pollWorker = Worker(self.pollingFunction, timeout, -1).start()

    def pollingFunction(self):
        # do something useful
        value = ... 
        self.set("polledValue", value)
```

## Client Interface: Sync & Async Calls

For simpler use, the `DeviceClient` (via `remote()`) provides convenient methods:

```python
# Synchronous call (blocks until return or exception; optional timeout)
self.remote().execute("/a/remote/device", "foo", 1)

# Asynchronous call (returns immediately)
self.remote().executeNoWait("/a/remote/device", "foo", 1)

# Set/get properties remotely
self.remote().set("/a/remote/device", "A", 1)
self.remote().setNoWait("/a/remote/device", "B", 2)
value = self.remote().get("/a/remote/device", "A")
```

### Property & Device Monitors

Register callbacks on property or device changes:

```python
def myCallBack(self, value, timestamp):
    self.log.INFO(f"Value changed: {value} at {timestamp}")

self.remote().registerPropertyMonitor("/a/remote/device", "A", self.myCallBack)
self.remote().registerDeviceMonitor("/a/remote/device", "", self.myCallBack)
```

> **Note:** For bound devices not needing low‑level event control, prefer the
middle-layer API.

## Simple State Machine

All device APIs provide state awareness via a _simple state machine_. Slots
can specify allowed states; state transitions are driven programmatically:

```python
@staticmethod
def expectedParameters(expected):
    (
      SLOT_ELEMENT(expected).key("start")
        .displayedName("Start")
        .allowedStates([States.STOPPED, States.IDLE])
        .commit(),
      SLOT_ELEMENT(expected).key("stop")
        .displayedName("Stop")
        .allowedStates([States.MOVING])
        .commit(),
    )

def start(self):
    # ...
    self.updateState(States.MOVING)
```

Available states are in the `states` enumerator.

## Karabo Data Types

Karabo properties use various data types—scalars, vectors, complex types,
arrays, tables—and a key‑value container, the **Hash**. Data is converted
between Karabo types and native types on `get`/`set`. Casting is supported
via `getAs`.

```python
h = Hash("foo", 1)
f = h.getAs("foo", float)   # f == 1.0
s = h.getAs("foo", str)     # s == "1"
h2 = Hash("bar", "Hello")
# h2.getAs("bar", int)      # raises exception
```

In C++:

```cpp
Hash h("foo", 1);
float f = h.getAs<float>("foo");
std::string s = h.getAs<std::string>("foo");
Hash h2("bar", "Hello World!");
// int i = h2.getAs<int>("bar"); // throws
```

### The Karabo Hash

An ordered key‑value store; insertion order preserved; supports nested `Hash`es.

```python
h = Hash()
h.set("foo", 1)
h.set("bar", 2)
for key in h.getKeys():
    print(key, h.get(key))
# foo 1
# bar 2

# Nested:
h1 = Hash()
h2 = Hash("a", 1)
h1.set("b", h2)
h3 = Hash("c", h1)
print(h3.get("c.b.a"))  # 1
h3.setAttribute("c.b.a", "myAttribute", "Test")
print(h3.getAttribute("c.b.a", "myAttribute"))  # "Test"
```

> **Note:** Retrieving a non‑existent key returns `None` (Python).

### Scalar Types

| Type | Karabo |
|------|--------|
| Boolean | BOOL |
| Character (raw byte) | CHAR |
| Signed integers | INT8, INT16, INT32, INT64 |
| Unsigned integers | UINT8, UINT16, UINT32, UINT64 |
| Floating point | FLOAT, DOUBLE |

> **Note:** Use explicit sizes (e.g., `INT32`, `INT64`) to avoid ambiguity.

### Complex Types

| Type | Karabo |
|------|--------|
| Complex float | COMPLEX_FLOAT |
| Complex double | COMPLEX_DOUBLE |

_C++ uses `std::complex<>`; Python uses `complex`._

### Vector Types

| Category | Karabo |
|----------|--------|
| Boolean vector | VECTOR_BOOL |
| Signed integer vectors | VECTOR_INT8, VECTOR_INT16, VECTOR_INT32, VECTOR_INT64 |
| Unsigned integer vectors | VECTOR_UINT8, VECTOR_UINT16, VECTOR_UINT32, VECTOR_UINT64 |
| Floating point vectors | VECTOR_FLOAT, VECTOR_DOUBLE |
| Complex vectors | VECTOR_COMPLEX_FLOAT, VECTOR_COMPLEX_DOUBLE |
| Hash vector | VECTOR_HASH |

### NDArray Types

`NDArray` and `ImageData` (with `IMAGEDATA_ELEMENT`) derive from `Hash`, store
arbitrary‑rank arrays or images plus metadata. Use standard `get`/`set`.

## Attributes

Attributes further specify a property or `Hash` key (e.g., bounds, units).
They support scalar, vector, and complex types.

### Numerical Representation

Set binary, hex or octal display in the GUI:

```python
UINT8_ELEMENT(expected).key("binaryRep")
  .displayedName("As binary")
  .bin()
  .assignmentOptional().defaultValue(128)
  .commit()  # displays as 0b10000000
```

Available representations:

| Representation | Method |
|----------------|--------|
| Binary mask    | `.bin()`  |
| Hexadecimal    | `.hex()`  |
| Octal          | `.oct()`  |


### Units

Assign SI or other units as attributes. Best practice for physical observables.

| Unit                | Symbol | Karabo              | Used for                            |
|---------------------|--------|---------------------|-------------------------------------|
| unitless            | --     | NUMBER              | Values without a defined unit       |
| count               | --     | COUNT               | Counters, iteration variable        |
| meter               | m      | METER               | Length, wavelength                  |
| gram                | g      | GRAM                | Weight                              |
| second              | s      | SECOND              | Time                                |
| ampère              | A      | AMPERE              | Electrical currents                 |
| kelvin              | K      | KELVIN              | Temperature                         |
| mole                | mol    | MOLE                | Molecular amount                    |
| candela             | cd     | CANDELA             | Luminous intensity                  |
| litre               | l      | LITRE               | Volume                              |
| hertz               | Hz     | HERTZ               | Frequency                           |
| radian              | rad    | RADIAN              | Angular distances                   |
| degree              | °      | DEGREE              | Angular distances                   |
| steradian           | sr     | STERADIAN           | Solid angles                        |
| newton              | N      | NEWTON              | Force                               |
| pascal              | Pa     | PASCAL              | Pressure                            |
| joule               | J      | JOULE               | Energy                              |
| electron volt       | eV     | ELECTRONVOLT        | Energy (1 eV = 1.602176×10⁻¹⁹ J)   |
| watt                | W      | WATT                | Power                               |
| coulomb             | C      | COULOMB             | Charge                              |
| volt                | V      | VOLT                | Voltage                             |
| farad               | F      | FARAD               | Capacity                            |
| ohm                 | Ω      | OHM                 | Resistance                          |
| siemens             | S      | SIEMENS             | Conductance, admittance, susceptance|
| weber               | Wb     | WEBER               | Magnetic flux                       |
| tesla               | T      | TESLA               | Magnetic flux density               |
| henry               | H      | HENRY               | Inductance                          |
| °C                  | °C     | DEGREE_CELSIUS      | Temperature                         |
| lumen               | lm     | LUMEN               | Luminous flux                       |
| lux                 | lx     | LUX                 | Illuminance                         |
| becquerel           | Bq     | BECQUEREL           | Radioactivity                       |
| gray                | Gy     | GRAY                | Ionizing dose                       |
| sievert             | Sv     | SIEVERT             | Effective dose                      |
| katal               | kat    | KATAL               | Catalytic activity                  |
| minute              | min    | MINUTE              | Time                                |
| hour                | h      | HOUR                | Time                                |
| day                 | d      | DAY                 | Time                                |
| year                | yr     | YEAR                | Time                                |
| bar                 | bar    | BAR                 | Pressure (use Pascal if possible)   |
| pixel               | px     | PIXEL               | Image display                       |
| byte                | B      | BYTE                | Memory, storage                     |
| bit                 | b      | BIT                 | Memory, storage                     |
| meter per second    | m/s    | METER_PER_SECOND    | Velocity                            |
| volt per second     | V/s    | VOLT_PER_SECOND     | Voltage ramping                     |
| ampère per second   | A/s    | AMPERE_PER_SECOND   | Current ramping                     |
| percent             | %      | PERCENT             | Relative quantification             |

> **Warning:** Karabo does _not_ perform unit-based calculations; ensure unit
compatibility manually.

### Metric Prefixes

Shift values by powers of ten without losing precision. Example:

```python
UINT8_ELEMENT(expected).key("prefixedValue")
  .displayedName("Prefixed value")
  .metricPrefix(MetricPrefix.MEGA)
  .assignmentOptional().defaultValue(128)
  .commit()
# Interpreted as 128×10^6
```

| Prefix | Factor       | Karabo      |
|--------|--------------|-------------|
| yotta  | 10^24        | YOTTA       |
| zetta  | 10^21        | ZETTA       |
| exa    | 10^18        | EXA         |
| peta   | 10^15        | PETA        |
| tera   | 10^12        | TERA        |
| giga   | 10^9         | GIGA        |
| mega   | 10^6         | MEGA        |
| kilo   | 10^3         | KILO        |
| hecto  | 10^2         | HECTO       |
| deca   | 10^1         | DECA        |
| (none) | 10^0         | NONE        |
| deci   | 10⁻1         | DECI        |
| centi  | 10⁻2         | CENTI       |
| milli  | 10⁻3         | MILLI       |
| micro  | 10⁻6         | MICRO       |
| nano   | 10⁻9         | NANO        |
| pico   | 10⁻12        | PICO        |
| femto  | 10⁻15        | FEMTO       |
| atto   | 10⁻18        | ATTO        |
| zepto  | 10⁻21        | ZEPTO       |
| yocto  | 10⁻24        | YOCTO       |

> **Note:** Prefixes are _not_ applied in native calculations.
Use `getPrefixFactor(key)` to retrieve the factor.

### Advantages of Units & Prefixes

- Reduces ambiguity; improves understanding for new users.
- Enables richer plotting (shared axes by unit, automatic scaling).

## Timestamps

Properties carry timestamps (UNIX epoch seconds + fractional seconds +
train ID). Use:

```python
now = self.getActualTimestamp()
train = 12
ts = Timestamp(now, train)
self.set("a", 1, ts)
```

Conversion utilities:

- `toIso8601(precision=MICROSEC, extended=False)`
- `toIso8601Ext(precision=MICROSEC, extended=False)`
- `toFormattedString(format="%Y-%b-%d %H:%M:%S", localTimeZone="Z")`
- `getSeconds()` → UNIX seconds
- `getFractionalSeconds()`
- `getTrainId()`

## The Karabo Schema

A device’s schema is a static, hierarchical description (serialized to XML)
of its expected parameters. Schema evolution is not supported.

### TABLE_ELEMENT

Internally a `VECTOR_HASH` with a `rowSchema` defining columns:

```python
# Define row schema
tableSchema = Schema()
(
  UINT32_ELEMENT(tableSchema).key("col1")
    .displayedName("Column One")
    .assignmentOptional().noDefaultValue()
    .commit(),
  STRING_ELEMENT(tableSchema).key("a")
    .displayedName("A")
    .assignmentOptional().defaultValue("Hello World!")
    .commit(),
  FLOAT_ELEMENT(tableSchema).key("b")
    .displayedName("Float Val")
    .assignmentMandatory()
    .commit(),
)

# Default rows
tableDefault = [ Hash("col1", 1, "b", 2.0) ]

# Define table element
TABLE_ELEMENT(expected).key("table")
  .displayedName("A Table Element")
  .setRowSchema(tableSchema)
  .assignmentOptional().defaultValue(tableDefault)
  .commit()
```

In the GUI this renders:

| Column One | A            | Float Val |
|------------|--------------|-----------|
| 1          | Hello World! | 2.0       |

Default values and validations follow the row schema.