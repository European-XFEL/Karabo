# Broker-based Communication

Karabo’s distributed communication relies on message brokers for housekeeping
(heartbeats, slow‑control parameters) and on point‑to‑point TCP/IP
**data pipelines** for high‑throughput data (images, digitizer or
pulse‑resolved data). Connection details are exchanged via the broker.

## Data Pipelines

- Each device can define multiple **output** and **input** channels, 
  identified by `"<device_id>:<channel_name>"`,
  e.g., `A/DEVICE/WITH_OUTPUT:output_1`.
- **Output → Input** connections are configured by listing desired
  output‑channel IDs in each input channel’s configuration. Karabo 
  auto‑establishes TCP connections when both devices are active.
- Data is sent as a *Karabo Hash* plus metadata (common timestamp, 
  data source = output channel ID).
- **Handlers** on input channels:
  - **data** handler: `fn(data, metadata)` invoked per data item.
  - **input** handler: `fn(channel)` must loop over available data items
    (batch size per configuration).
  - **end-of-stream** handler: `fn(channel)` notified when the sender
    signals end‑of‑stream.
- In C++/Python, device methods `writeChannel`, `signalEndOfStream` 
  (and underlying channel `write`, `update`, `signalEndOfStream`) are 
  *not thread‑safe* and must not be called concurrently for the same channel.
- Scatter/gather supported: many inputs ← one output; many outputs → one input.

---

# Channel Configurations

Define channels in the device schema (e.g. in C++/Python `expectedParameters`).
You can also create channels dynamically, but for unknown future outputs it’s
recommended to use `remote().registerChannelMonitor()`.

## Input Channel Properties

- **connectedOutputChannels** *(VECTOR_STRING)*  
  List of output channel IDs to receive from, e.g.  
  `["A/DEVICE/WITH_OUTPUT:output_1"]`.
- **dataDistribution** *(STRING)*  
  - `copy` (default): each input receives all data.  
  - `shared`: data items are load‑distributed among all inputs in this mode.
- **onSlowness** *(STRING)* (only in `copy` mode)  
  Policy if this input is too slow:
  - `drop` (default since 2.10.0): drop new items.
  - `queueDrop` (since 2.10.0): buffer items; if full, drop oldest.
- **minData** *(UINT32)*  
  Minimum items per call in an **input** handler.  
  Default 1; `0`=all available; `0xFFFFFFFF`=unbounded.
- **respondToEndOfStream** *(BOOL)*  
  Whether to invoke the end‑of‑stream handler. Default `true`.
- **delayOnInput** *(INT32)*  
  Milliseconds to delay before signalling readiness for more data. Default 0.

## Output Channel Properties

- **distributionMode** *(STRING)*  
  (applies when inputs use `shared` distribution)
  - `load-balanced` (default): send to any ready input.
  - `round-robin`: cycle through inputs in order.
- **noInputShared** *(STRING)*  
  (if no `shared` inputs are ready)
  - `drop` (default since 2.10.0): drop new items.
  - `queueDrop` (since 2.10.0): buffer items; if full, drop oldest.
- **hostname** *(STRING)*  
  Address for inputs to connect to.  
  Default `"default"` (device’s host); set to a specific interface if needed.
- **port** *(UINT32)*  
  TCP port for inputs. Default 0 (auto‑assigned); if nonzero, must be free.

---

# Schema Description of Channels

Although Karabo can send arbitrary hash structures, you should:

- **Output channels**: declare the schema of sent data 
  (required for DAQ storage or GUI visualization).
- **Input channels**: declare expected data schemas.

In C++/Python, specify schemas in the device’s `expectedParameters()` 
when defining channels.