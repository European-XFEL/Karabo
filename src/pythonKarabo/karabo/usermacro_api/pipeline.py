"""OutputChannel implemented to be retrofitted to the middlelayer
"""
from asyncio import (
    coroutine, get_event_loop, Lock, open_connection)
import os
from struct import pack, unpack, calcsize

from karabo.middlelayer import (
    Assignment, AccessMode, Bool, background,
    decodeBinary, encodeBinary, Hash, Node, VectorUInt8,
    Schema, String, VectorString)
from karabo.middlelayer_api.proxy import ProxyBase, ProxyFactory
from karabo.middlelayer_api.pipeline import PipelineMetaData


class OutputChannel(VectorUInt8):
    """Create an output channel"""
