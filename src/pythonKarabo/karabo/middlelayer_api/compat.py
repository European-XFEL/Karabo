# flake8: noqa: F401
import os

HAVE_UVLOOP = False
HAVE_ASYNCIO = False

if os.environ.get("KARABO_UVLOOP"):
    from uvloop import Loop as AbstractEventLoop, Loop as SelectorEventLoop
    HAVE_UVLOOP = True
else:
    from asyncio import AbstractEventLoop, SelectorEventLoop
    HAVE_ASYNCIO = True
