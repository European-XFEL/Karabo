# AGENTS.md

This file routes work in this repository to the correct API-specific guide.

The first decision is which API the task is about:

- C++ device
- bound-Python device
- middlelayer device

That API choice can come either from the request itself or from the code being
edited. Use both signals together.

## API Routing

- If the request explicitly says `C++ device`, `cpp device`, `C++ framework`,
  or otherwise clearly asks for the classic C++ API, use
  `AGENTS.cpp.md`.
- If the code contains `karabo::core::Device`,
  `expectedParameters(Schema&)`, `KARABO_REGISTER_FOR_CONFIGURATION`,
  `KARABO_SLOT`, `KARABO_ON_DATA`, `karabo::data::Hash`, or
  `karabo::core::DeviceClient`, use
  `AGENTS.cpp.md`.
- If the request explicitly says `bound device`, `bound Python`,
  `pybind11 binding`, `karabind`, or otherwise clearly asks for the bound API,
  use
  `AGENTS.python-bound.md`.
- If the code contains `from karabo.bound import ...`, `from karabind import
  ...`, `PythonDevice`, `@KARABO_CLASSINFO`,
  `@KARABO_CONFIGURATION_BASE_CLASS`, `registerInitialFunction`,
  `updateSchema(schema)`, or bound `DeviceClient`, use
  `AGENTS.python-bound.md`.
- If the request explicitly says `middlelayer device`, `middle layer`, `MDL
  device`, `Karabo async Python device`, or otherwise clearly asks for
  Karabo's descriptor-based asynchronous Python API, use
  `AGENTS.middlelayer.md`.
- If the code imports `karabo.middlelayer` (including
  `karabo.middlelayer.*`), or subclasses its `Device`, `DeviceClientBase`, or
  `Macro`, use `AGENTS.middlelayer.md`.
- Also use `AGENTS.middlelayer.md` when descriptor-style class attributes are
  combined with characteristic middlelayer symbols such as `@Slot(...)`,
  `@slot`, `@InputChannel(...)`, `OutputChannel(...)`,
  `async def onInitialization(...)`, `getDevice(...)`, `connectDevice(...)`,
  `setWait(...)`, or `background(...)`.

## Repository Rule

- Do not mix the APIs in one implementation style.
- Prefer explicit API intent plus buzzwords and imported symbols over directory
  names when deciding.
- Do not route on generic symbols such as `Hash`, `State`, `String`, `Slot`,
  `Configurable`, or `async def` alone. Require an API-specific import, base
  class, or a combination of characteristic symbols.
- Use directory location only as a fallback when the API surface is still
  unclear.
