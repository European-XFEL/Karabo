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

## Repository Rule

- Do not mix the APIs in one implementation style.
- Prefer explicit API intent plus buzzwords and imported symbols over directory
  names when deciding.
- Use directory location only as a fallback when the API surface is still
  unclear.
