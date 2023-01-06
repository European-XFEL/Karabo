"""Custom API definition for Qsci autocompletion"""

global _KARABO_NAMESPACE
_KARABO_NAMESPACE = None


def get_symbols():
    global _KARABO_NAMESPACE
    if _KARABO_NAMESPACE is None:
        _KARABO_NAMESPACE = create_symbols()
    return _KARABO_NAMESPACE


_MIDDLELAYER = (
    "allCompleted",
    "background",
    "call",
    "callNoWait",
    "connectDevice",
    "execute",
    "executeNoWait",
    "DeviceClientBase",
    "firstCompleted",
    "getDevice",
    "getDevices",
    "getConfiguration",
    "getInstanceInfo",
    "getSchema",
    "getServers",
    "getTimeInfo",
    "getTopology",
    "get_property",
    "instantiate",
    "isAlive",
    "Macro",
    "MacroSlot",
    "PipelineContext",
    "set_property",
    "setWait",
    "setNoWait",
    "shutdown",
    "sleep",
    "updateDevice",
    "waitUntil",
    "waitUntilNew",
    "waitWhile",
)

_ASYNCIO = (
    "CancelledError",
    "gather",
    "sleep",
    "TimeoutError",
    "wait_for",
)


def create_symbols():
    """Lazily create all symbols for the global namespace of a Scintilla
    editor
    """
    from karabo.native import data, schema

    native_symbols = [f"karabo.middlelayer.{symbol}"
                      for symbol in data.__all__ + schema.__all__]

    mdl_symbols = [f"karabo.middlelayer.{symbol}" for symbol in _MIDDLELAYER]

    asyncio_symbols = [f"asyncio.{symbol}" for symbol in _ASYNCIO]
    symbols = native_symbols + mdl_symbols + asyncio_symbols

    return symbols
