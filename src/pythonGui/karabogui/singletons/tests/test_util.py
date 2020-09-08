# flake8: noqa

from ..util import get_error_message


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


EXPECTED_MESSAGE = {
    CPP_SLOT_ERROR_MESSAGE: 'Command "move" is not allowed in current state "ERROR" of device "MOV_TEST/MOTOR/SERVO_1".',
    PYTHON_SLOT_ERROR_MESSAGE: 'RuntimeError: Problematic execute: ProblematicSlot',
    PYTHON_MULTIPLE_ERROR_MESSAGE: 'RuntimeError: Something went wrong.'
}


def test_get_error_message():
    _assert_error_message(CPP_SLOT_ERROR_MESSAGE)
    _assert_error_message(PYTHON_SLOT_ERROR_MESSAGE)
    _assert_error_message(PYTHON_MULTIPLE_ERROR_MESSAGE)


def _assert_error_message(message):
    assert get_error_message(message) == EXPECTED_MESSAGE[message]
