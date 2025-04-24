# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
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
    import karabo.native as native
    native_symbols = [f"karabo.middlelayer.{symbol}"
                      for symbol in dir(native) if not symbol.startswith("__")]

    mdl_symbols = [f"karabo.middlelayer.{symbol}" for symbol in _MIDDLELAYER]

    asyncio_symbols = [f"asyncio.{symbol}" for symbol in _ASYNCIO]
    symbols = native_symbols + mdl_symbols + asyncio_symbols

    return symbols
