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
# flake8: noqa
from karabo.native import Hash

from ..util import decrypt, encrypt, get_error_message, realign_topo_hash

CPP_SLOT_ERROR_MESSAGE = """
Request to execute 'execute2' on device 'Macro-test1-ec6d56fe-a5ca-445f-ad45-a295c7e16450-Test' failed, details:
1. Exception =====>  {
    Exception Type....:  Remote Exception from Macro-test1-ec6d56fe-a5ca-445f-ad45-a295c7e16450-Test
    Message...........:  Traceback (most recent call last):
  File "/scratch/xctrl/karabo/extern/lib/python3.6/site-packages/karabo/native/data/hash.py", line 591, in wrapper
    ret = yield from coro
  File "/scratch/xctrl/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/eventloop.py", line 644, in run_coroutine_or_thread
    return (yield from future)
  File "/scratch/xctrl/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/eventloop.py", line 630, in thread
    ret = f(*args, **kwargs)
  File "/scratch/xctrl/karabo/extern/lib/python3.6/site-packages/karabo/native/data/hash.py", line 559, in wrapper
    return self.method(device)
  File "/scratch/xctrl/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/macro.py", line 82, in wrapper
    return themethod(device)
  File "test1", line 20, in execute2
  File "test1", line 15, in do2
  File "/scratch/xctrl/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/proxy.py", line 164, in method
    return myself._parent._callSlot(self, *args, **kwargs)
  File "/scratch/xctrl/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/proxy.py", line 363, in _callSlot
    self._raise_on_death(inner()), timeout, wait)
  File "/scratch/xctrl/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/eventloop.py", line 505, in sync
    return self.task.result()
  File "/scratch/xctrl/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/proxy.py", line 443, in _raise_on_death
    return (yield from task)
  File "/scratch/xctrl/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/proxy.py", line 361, in inner
    descriptor.longkey))
  File "/scratch/xctrl/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/signalslot.py", line 338, in call
    return (yield from self._ss.request(device, target, *args))
  File "/scratch/xctrl/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/eventloop.py", line 145, in request
    return (yield from future)
karabo.native.exceptions.KaraboError: 1. Exception =====>  {
    Exception Type....:  Logic Exception
    Message...........:  Command "move" is not allowed in current state "ERROR" of device "MOV_TEST/MOTOR/SERVO_1".
    File..............:  /[...]/src/karabo/core/Device.hh
    Function..........:  void karabo::core::Device<T>::ensureSlotIsValidUnderCurrentState(const string&) [with FSM = karabo::core::NoFsm; std::string = std::basic_string<char>]
    Line Number.......:  1795
    Timestamp.........:  2020-Aug-11 17:25:47.621487
}

    Timestamp.........:  2020-Aug-11 17:25:47.649369
}
"""

PYTHON_SLOT_ERROR_MESSAGE = """
Request to execute 'hello' on device 'Macro-PROBLEMATIC_SLOT-98984c38-9428-4054-9f6d-cc428c6e6e13-ProblematicSlot' failed, details:
1. Exception =====>  {
    Exception Type....:  Remote Exception from Macro-PROBLEMATIC_SLOT-98984c38-9428-4054-9f6d-cc428c6e6e13-ProblematicSlot
    Message...........:  Traceback (most recent call last):
  File "/home/carinanc/karabo/extern/lib/python3.6/site-packages/karabo/native/data/hash.py", line 591, in wrapper
    ret = yield from coro
  File "/home/carinanc/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/eventloop.py", line 644, in run_coroutine_or_thread
    return (yield from future)
  File "/home/carinanc/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/eventloop.py", line 630, in thread
    ret = f(*args, **kwargs)
  File "/home/carinanc/karabo/extern/lib/python3.6/site-packages/karabo/native/data/hash.py", line 559, in wrapper
    return self.method(device)
  File "/home/carinanc/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/macro.py", line 82, in wrapper
    return themethod(device)
  File "PROBLEMATIC_SLOT", line 12, in hello
  File "/home/carinanc/karabo/extern/lib/python3.6/site-packages/karabo/native/data/hash.py", line 559, in wrapper
    return self.method(device)
  File "/home/carinanc/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/macro.py", line 82, in wrapper
    return themethod(device)
  File "PROBLEMATIC_SLOT", line 8, in execute
RuntimeError: Problematic execute: ProblematicSlot

    Timestamp.........:  2020-Sep-07 13:28:20.319395
}
"""

PYTHON_MULTIPLE_ERROR_MESSAGE = """
Request to execute 'hello' on device 'Macro-PROBLEMATIC_SLOT-98984c38-9428-4054-9f6d-cc428c6e6e13-ProblematicSlot' failed, details:
1. Exception =====>  {
    Exception Type....:  Remote Exception from Macro-PROBLEMATIC_SLOT-98984c38-9428-4054-9f6d-cc428c6e6e13-ProblematicSlot
    Message...........:  Traceback (most recent call last):
  File "PROBLEMATIC_SLOT", line 13, in hello
  File "/home/carinanc/karabo/extern/lib/python3.6/site-packages/karabo/native/data/hash.py", line 559, in wrapper
    return self.method(device)
  File "/home/carinanc/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/macro.py", line 82, in wrapper
    return themethod(device)
  File "PROBLEMATIC_SLOT", line 8, in execute
NameError: name 'RunTimeError' is not defined

During handling of the above exception, another exception occurred:

Traceback (most recent call last):
  File "/home/carinanc/karabo/extern/lib/python3.6/site-packages/karabo/native/data/hash.py", line 591, in wrapper
    ret = yield from coro
  File "/home/carinanc/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/eventloop.py", line 644, in run_coroutine_or_thread
    return (yield from future)
  File "/home/carinanc/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/eventloop.py", line 630, in thread
    ret = f(*args, **kwargs)
  File "/home/carinanc/karabo/extern/lib/python3.6/site-packages/karabo/native/data/hash.py", line 559, in wrapper
    return self.method(device)
  File "/home/carinanc/karabo/extern/lib/python3.6/site-packages/karabo/middlelayer_api/macro.py", line 82, in wrapper
    return themethod(device)
  File "PROBLEMATIC_SLOT", line 15, in hello
RuntimeError: Something went wrong.

    Timestamp.........:  2020-Sep-08 00:12:24.605280
}
"""

SIMPLE_ERROR_MESSAGE = """\
Failure on request to execute 'reset' on device 'plcMonitor. Request not answered within 5 seconds."""

UNKNOWN_ERROR_MESSAGE = 42 * "karabo"

KARABO_ERROR_MESSAGE = """
Failure on request to execute 'faultySlot' on device 'XHQ_EG_DG/DATA/PROPERTY_TEST_MDL', details:
1. Exception =====>  {
    Exception Type....:  Remote Exception from XHQ_EG_DG/DATA/PROPERTY_TEST_MDL
    Message...........:  Traceback (most recent call last):
  File "/home/degon/Framework/src/pythonKarabo/karabo/native/schema/descriptors.py", line 657, in wrapper
    ret = await coro
  File "/home/degon/Framework/src/pythonKarabo/karabo/middlelayer_api/eventloop.py", line 1234, in run_coroutine_or_thread
    return (await future)
  File "/home/degon/Framework/src/pythonKarabo/karabo/middlelayer_api/eventloop.py", line 1207, in thread
    ret = f(*args, **kwargs)
  File "/home/degon/Framework/src/pythonKarabo/karabo/native/schema/descriptors.py", line 628, in wrapper
    return self.method(device)
  File "/home/degon/Framework/src/pythonKarabo/karabo/middlelayer_devices/property_test.py", line 614, in faultySlot
    raise exception("Fauly Slot cannot be executed")
karabo.native.exceptions.KaraboError: Fauly Slot cannot be executed

    Timestamp.........:  2021-Mar-31 13:35:28.973438
}
"""

KARABO_ERROR_MESSAGE_WITH_TAGGY_TEXT = """
Failure on request to execute 'loadFromTime' on device 'Princess', details:
1. Exception =====>  {
    Exception Type....:  Remote Exception from Princess
    Message...........:  Traceback (most recent call last):
  File "/path/to/work/Framework/src/pythonKarabo/karabo/native/schema/descriptors.py", line 669, in wrapper
    ret = await coro
  File "/path/to/work/Framework/src/pythonKarabo/karabo/middlelayer_api/eventloop.py", line 400, in run_coroutine_or_thread
    return (await f(*args, **kwargs))
  File "/path/to/work/Framework/src/pythonKarabo/karabo/native/schema/descriptors.py", line 636, in wrapper
    return await self.method(device)
  File "/path/to/work/Framework/src/pythonKarabo/karabo/native/schema/basetypes.py", line 44, in wrap
    raise TypeError('cannot wrap "{}" into Karabo type'.format(type(data)))
TypeError: cannot wrap "<class 'numpy.bool_'>" into Karabo type
    Timestamp.........:  2021-Dec-02 14:29:59.046236
}
"""

EXPECTED_MESSAGE = {
    CPP_SLOT_ERROR_MESSAGE: 'Command &quot;move&quot; is not allowed in current state &quot;ERROR&quot; of device &quot;MOV_TEST/MOTOR/SERVO_1&quot;.',
    PYTHON_SLOT_ERROR_MESSAGE: 'RuntimeError: Problematic execute: ProblematicSlot',
    PYTHON_MULTIPLE_ERROR_MESSAGE: 'RuntimeError: Something went wrong.',
    SIMPLE_ERROR_MESSAGE: SIMPLE_ERROR_MESSAGE,
    KARABO_ERROR_MESSAGE: "Fauly Slot cannot be executed",
    KARABO_ERROR_MESSAGE_WITH_TAGGY_TEXT: 'TypeError: cannot wrap &quot;&lt;class &#x27;numpy.bool_&#x27;&gt;&quot; into Karabo type',
    UNKNOWN_ERROR_MESSAGE: "Unknown exception",
}


def test_get_error_message():
    _assert_error_message(CPP_SLOT_ERROR_MESSAGE)
    _assert_error_message(PYTHON_SLOT_ERROR_MESSAGE)
    _assert_error_message(PYTHON_MULTIPLE_ERROR_MESSAGE)
    _assert_error_message(SIMPLE_ERROR_MESSAGE)
    _assert_error_message(KARABO_ERROR_MESSAGE)
    _assert_error_message(KARABO_ERROR_MESSAGE_WITH_TAGGY_TEXT)
    _assert_error_message(UNKNOWN_ERROR_MESSAGE)


def _assert_error_message(message):
    assert get_error_message(message) == EXPECTED_MESSAGE[message]


def test_realign_topo_hash():
    h = Hash()

    h['server.server1'] = None
    h['server.server1', ...] = {
        'host': 'BIG_IRON',
        'deviceClasses': ['FooClass', 'BarClass'],
    }
    h['server.server2'] = None
    h['server.server2', ...] = {  # None host
        'deviceClasses': ['FooClass', 'BarClass'],
    }
    h['server.server3'] = None
    h['server.server3', ...] = {
        'host': 'AN_IRON',
        'deviceClasses': ['FooClass', 'BarClass'],
    }

    servers = realign_topo_hash(h["server"], "host")

    n = iter(servers)
    item = next(n)
    assert item == "server2"
    item = next(n)
    assert item == "server3"
    item = next(n)
    assert item == "server1"


def test_encryption():
    pw = "karabo"
    cipher = "secret"
    new = encrypt(pw, cipher)
    assert new != pw
    new = decrypt(new, cipher)
    assert new == pw
