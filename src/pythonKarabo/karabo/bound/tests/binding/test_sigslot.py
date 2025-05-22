# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.

import threading
import time
import uuid
from contextlib import nullcontext as does_not_raise
from functools import partial

import pytest

from karabo.bound import EventLoop, Hash, SignalSlotable


@pytest.fixture(scope="module")
def eventLoopFixt():
    loop_thread = threading.Thread(target=EventLoop.work)
    loop_thread.start()

    yield  # now all (since scope="module") tests are executed

    EventLoop.stop()
    loop_thread.join(timeout=10)
    assert not loop_thread.is_alive()


class SignalSlotableWithSlots(SignalSlotable):
    def slotOneArg(self, a):
        pass

    def slotVarArgs(self, *args):
        self.reply(len(args))

    def slotTwoArgDef(self, a, b, c=42):
        self.reply(a + b + c)

    # A keyword only argument with default
    def slotKwargs(self, *args, a=77):
        self.reply(len(args), a)


def test_sigslot_register_function(eventLoopFixt):

    timeout = 2000  # timeout in ms

    sigSlot = SignalSlotableWithSlots("sigSlot" + str(uuid.uuid4()))
    sigSlot.start()

    def funcOneArg(a):
        pass

    def funcVarArgs(*args):
        sigSlot.reply(len(args))

    def funcTwoArgDef(a, b, c=42):
        sigSlot.reply(a + b + c)

    # A keyword only argument with default
    def funcKwargs(*args, a=77):
        sigSlot.reply(len(args), a)

    # Ordinary registration - register under <function>.__name__
    sigSlot.registerSlot(funcOneArg)
    # "" => self messaging!
    sigSlot.request("", "funcOneArg", 1).waitForReply(timeout)

    # Register under defined name (and test '/' as part of name)
    sigSlot.registerSlot(funcOneArg, "func/oneArg1")
    req = sigSlot.request("", "func/oneArg1", "no_matter")
    req.waitForReply(timeout)

    # Register function with variable number of arguments.
    # Automatic number of argument detection gives zero:
    sigSlot.registerSlot(funcVarArgs)
    req = sigSlot.request("", "funcVarArgs")
    (nArg, ) = req.waitForReply(timeout)
    assert nArg == 0
    # NOTE: We require STRICT match for number of arguments.
    #       It's in underlying C++ code as well.
    req = sigSlot.request("", "funcVarArgs", 1)
    with pytest.raises(RuntimeError) as excinfo:
        req.waitForReply(timeout)
        assert "mismatched number of args" in str(excinfo.value)

    # We can register '*args' function with fixed number
    # (Also test that '_' can be requested as '.' is needed
    #  for nested slots in Schema)
    sigSlot.registerSlot(funcVarArgs, "func_varArgs1", 1)
    req = sigSlot.request("", "func.varArgs1", "no_matter2")
    (nArg, ) = req.waitForReply(timeout)
    assert nArg == 1

    # Then we must not miss to sent argument
    req = sigSlot.request("", "func_varArgs1")  # here access with '_'
    with pytest.raises(RuntimeError):
        req.waitForReply(timeout)

    #
    # Register function with default argument.
    #
    # Automatic number of argument detection includes the default
    # (cannot change that for backward compatibility...).
    sigSlot.registerSlot(funcTwoArgDef, "funcTwoArgDef3")
    req = sigSlot.request("", "funcTwoArgDef3", 1, 2, 3)
    (theSum, ) = req.waitForReply(timeout)
    assert theSum == 6

    # But it is OK to register with less args to get default
    sigSlot.registerSlot(funcTwoArgDef, numArgs=2)
    req = sigSlot.request("", "funcTwoArgDef", 2, 3)
    (theSum, ) = req.waitForReply(timeout)
    assert theSum == 47  # 2 + 3 + 42

    # Still, we must not miss to sent required arguments
    req = sigSlot.request("", "funcTwoArgDef", 1)
    with pytest.raises(RuntimeError):
        req.waitForReply(timeout)

    #
    # Register function with default keyword only argument.
    #
    # Automatic number of argument detection excludes the default
    sigSlot.registerSlot(funcKwargs)
    req = sigSlot.request("", "funcKwargs")
    (length, a) = req.waitForReply(timeout)
    assert length == 0
    assert a == 77

    # Can specify whatever number of args, but keyword only arg is fix:
    sigSlot.registerSlot(funcKwargs, "funcKwargs1", 1)
    req = sigSlot.request("", "funcKwargs1", -77)
    (length, a) = req.waitForReply(timeout)
    assert length == 1
    assert a == 77

    sigSlot.registerSlot(funcKwargs, "funcKwargs2", 2)
    req = sigSlot.request("", "funcKwargs2", 111, 222)
    (length, a) = req.waitForReply(timeout)
    assert length == 2
    assert a == 77


def test_sigslot_register_method(eventLoopFixt):

    timeout = 2000  # timeout in ms

    sigSlot = SignalSlotableWithSlots("sigSlot" + str(uuid.uuid4()))
    sigSlot.start()

    #
    # Ordinary registration - register under <method>.__name__
    #
    sigSlot.registerSlot(sigSlot.slotOneArg)
    sigSlot.request("", "slotOneArg", 1).waitForReply(timeout)

    # Register under defined name
    sigSlot.registerSlot(sigSlot.slotOneArg, "slotOneArg1")
    req = sigSlot.request("", "slotOneArg1", "no_matter")
    req.waitForReply(timeout)

    #
    # Register method with variable number of arguments.
    #
    # Automatic number of argument detection gives zero:
    sigSlot.registerSlot(sigSlot.slotVarArgs)
    req = sigSlot.request("", "slotVarArgs")
    (nArg, ) = req.waitForReply(timeout)
    assert nArg == 0
    # NOTE: We require STRICT match for number of arguments.
    #       It's in underlying C++ code as well.
    req = sigSlot.request("", "slotVarArgs", "extra_arg")
    with pytest.raises(RuntimeError) as excinfo:
        req.waitForReply(timeout)
        assert "mismatched number of args" in str(excinfo.value)

    # We can register '*args' function with fixed number
    sigSlot.registerSlot(sigSlot.slotVarArgs, "slotVarArgs1", 2)
    req = sigSlot.request("", "slotVarArgs1", "no", "matter")
    (nArg, ) = req.waitForReply(timeout)
    assert 2 == nArg

    # Then we must not miss to sent argument
    req = sigSlot.request("", "slotVarArgs1", "not_enough")
    with pytest.raises(RuntimeError):
        req.waitForReply(timeout)

    #
    # Register method with default argument.
    #
    # Automatic number of argument detection includes the default
    # (cannot change that for backward compatibility...).
    sigSlot.registerSlot(sigSlot.slotTwoArgDef, "slotTwoArgDef3")
    req = sigSlot.request("", "slotTwoArgDef3", 1, 2, 3)
    (theSum, ) = req.waitForReply(timeout)
    assert 6 == theSum

    # But it is OK to register with less args to get default
    sigSlot.registerSlot(sigSlot.slotTwoArgDef, numArgs=2)
    req = sigSlot.request("", "slotTwoArgDef", 1, 2)
    (theSum, ) = req.waitForReply(timeout)
    assert 45 == theSum  # 1 + 2 + 42

    # Still, we must not miss to sent required arguments
    req = sigSlot.request("", "slotTwoArgDef", 1)
    with pytest.raises(RuntimeError):
        req.waitForReply(timeout)

    #
    # Register method with default keyword only argument.
    #
    # Automatic number of argument detection excludes the default
    sigSlot.registerSlot(sigSlot.slotKwargs)
    req = sigSlot.request("", "slotKwargs")
    (length, a) = req.waitForReply(timeout)
    assert 0 == length
    assert 77 == a

    # Can specify whatever number of args, but keyword only arg is fix:
    sigSlot.registerSlot(sigSlot.slotKwargs, "slotKwargs1", 1)
    req = sigSlot.request("", "slotKwargs1", -77)
    (length, a) = req.waitForReply(timeout)
    assert 1 == length
    assert 77 == a

    sigSlot.registerSlot(sigSlot.slotKwargs, "slotKwargs2", 2)
    req = sigSlot.request("", "slotKwargs2", 111, 222)
    (length, a) = req.waitForReply(timeout)
    assert 2 == length
    assert 77 == a


def test_sigslot_register_lambda(eventLoopFixt):

    timeout = 2000  # timeout in ms

    sigSlot = SignalSlotableWithSlots("sigSlot" + str(uuid.uuid4()))
    sigSlot.start()

    # Lambdas have a (not so unique) __name__, taken by default.
    # Auto detection of num(arguments) works, default arg don't harm.
    sigSlot.registerSlot(lambda a, b, c, d=55: sigSlot.reply(a, b, c, d))
    req = sigSlot.request("", "<lambda>", 1, 2, 3, 5)
    ll, m, n, o = req.waitForReply(timeout)
    assert 1 == ll
    assert 2 == m
    assert 3 == n
    assert 5 == o

    # Make actually use of lambda's default argument
    sigSlot.registerSlot(lambda a, b, c, d=55: sigSlot.reply(a, b, c, d),
                         "lam", 3)
    req = sigSlot.request("", "lam", 8, 13, 21)
    ll, m, n, o = req.waitForReply(timeout)
    assert 8 == ll
    assert 13 == m
    assert 21 == n
    assert 55 == o

    # Test that 'partial' works for slots - they have no __name__
    def forPartial(a, b):
        sigSlot.reply(a, b)

    part = partial(forPartial, 42)  # 'default' for a
    # Need to define slotName and numArgs, auto detection fails
    sigSlot.registerSlot(part, "partial", 1)
    req = sigSlot.request("", "partial", 43)
    a, b = req.waitForReply(timeout)
    assert 42 == a
    assert 43 == b


def test_sigslot_unique_id(eventLoopFixt):

    oneId = "one" + str(uuid.uuid4())
    twoId = "two" + str(uuid.uuid4())
    one = SignalSlotable(oneId)
    two = SignalSlotable(twoId, Hash(), 30, Hash('type', 'sigslot'))
    with does_not_raise():
        one.start()
        two.start()
    assert one.getInstanceId() == oneId
    assert two.getInstanceId() == twoId
    one_info = one.getInstanceInfo()
    two_info = two.getInstanceInfo()
    assert one_info['type'] == "unknown"
    assert two_info['type'] == "sigslot"
    assert one_info['heartbeatInterval'] == 10
    assert two_info['heartbeatInterval'] == 30
    avail = one.getAvailableInstances()
    assert isinstance(avail, Hash) is True
    assert avail[oneId + '.countdown'] == 10
    assert avail[twoId + '.countdown'] == 30
    assert avail[oneId + '.instanceInfo.type'] == 'unknown'
    assert avail[twoId + '.instanceInfo.type'] == 'sigslot'
    assert (avail[oneId + '.instanceInfo.karaboVersion'] ==
            avail[twoId + '.instanceInfo.karaboVersion'])
    one_again = SignalSlotable(oneId)
    with pytest.raises(Exception):
        one_again.start()


def test_sigslot_request(eventLoopFixt):

    bobId = "bob" + str(uuid.uuid4())
    bob = SignalSlotable(bobId)
    bob.start()
    called = -1

    def slot0():
        nonlocal called
        called = 0
        bob.reply()
    bob.registerSlot(slot0)

    def slot1(i):
        nonlocal called
        called = i
        bob.reply(i)
    bob.registerSlot(slot1)

    def slot2(i, j):
        nonlocal called
        called = i + j
        bob.reply(i, j)
    bob.registerSlot(slot2)

    def slot3(i, j, k):
        nonlocal called
        called = i + j + k
        bob.reply(i, j, k)
    bob.registerSlot(slot3)

    def slot4(i, j, k, n):  # flake does not like l: 'ambiguous var. name'
        nonlocal called
        called = i + j + k + n
        bob.reply(i, j, k, n)
    bob.registerSlot(slot4)

    aliceId = "alice" + str(uuid.uuid4())
    alice = SignalSlotable(aliceId)
    alice.start()

    handled = False

    def wait_for_handled():
        max_count = 2000
        while not handled and max_count > 0:
            time.sleep(0.01)
            max_count -= 1

    def handle0():
        nonlocal handled
        handled = True

    req = alice.request(bobId, "slot0")
    req.receiveAsync0(handle0)
    wait_for_handled()
    assert handled is True
    assert called == 0
    # test again via return-argument-generic receiveAsync
    handled = False
    req = alice.request(bobId, "slot0")
    req.receiveAsync(handle0)
    wait_for_handled()
    assert 0 == called

    # Test one argument
    handled = False
    i = 0

    def handle1(ii):
        nonlocal handled, i
        handled = True
        i = ii

    req = alice.request(bobId, "slot1", 5)
    req.receiveAsync1(handle1)
    wait_for_handled()
    assert handled is True
    assert 5 == called
    assert 5 == i

    # test again via return-argument-generic receiveAsync
    handled = False
    i = 0
    called = -1
    req = alice.request(bobId, "slot1", 5)
    req.receiveAsync(handle1)
    wait_for_handled()
    assert handled is True
    assert 5 == called
    assert 5 == i

    # test 2 arguments slot
    handled = False
    called == -1
    i, j = 0, 0

    def handle2(ii, jj):
        nonlocal handled, i, j
        handled = True
        i, j = ii, jj

    req = alice.request(bobId, "slot2", 5, 7)
    req.receiveAsync2(handle2)
    wait_for_handled()
    assert 12 == called
    assert 5 == i
    assert 7 == j
    # test again via return-argument-generic receiveAsync
    handled = False
    i = j = 0
    called = -1
    req = alice.request(bobId, "slot2", 5, 7)
    req.receiveAsync(handle2)
    wait_for_handled()
    assert 12 == called
    assert 5 == i
    assert 7 == j

    # Test three arguments
    handled = False
    called = -1
    i = j = k = 0

    def handle3(ii, jj, kk):
        nonlocal handled, i, j, k
        handled = True
        i, j, k = ii, jj, kk

    req = alice.request(bobId, "slot3", 5, 7, 11)
    req.receiveAsync3(handle3)
    wait_for_handled()
    assert handled is True
    assert 23 == called
    assert 5 == i
    assert 7 == j
    assert 11 == k
    # test again via return-argument-generic receiveAsync
    handled = False
    i = j = k = 0
    called = -1
    req = alice.request(bobId, "slot3", 5, 7, 11)
    req.receiveAsync(handle3)
    wait_for_handled()
    assert 23 == called
    assert 5 == i
    assert 7 == j
    assert 11 == k

    # Test four arguments
    handled = False
    called = -1
    i = j = k = n = 0  # flake does not like variable named 'l'

    def handle4(ii, jj, kk, nn):
        nonlocal handled, i, j, k, n
        handled = True
        i, j, k, n = ii, jj, kk, nn

    req = alice.request(bobId, "slot4", 5, 7, 11, 13)
    req.receiveAsync4(handle4)
    wait_for_handled()
    assert 36 == called
    assert 5 == i
    assert 7 == j
    assert 11 == k
    assert 13 == n
    # test again via return-argument-generic receiveAsync
    handled = False
    i = j = k = n = 0
    called = -1
    req = alice.request(bobId, "slot4", 5, 7, 11, 13)
    req.receiveAsync(handle4)
    wait_for_handled()
    assert 36 == called
    assert 5 == i
    assert 7 == j
    assert 11 == k
    assert 13 == n

    # Test error handling
    called = -1

    def slotError():
        nonlocal called
        called = 42
        # Exception in Python code
        raise RuntimeError("What's the universe and the rest?")

    bob.registerSlot(slotError)

    def slotErrorCpp():
        nonlocal called
        called = 43
        # Exception in Karabo C++ code
        h = Hash()
        h["non_existing_key"]

    bob.registerSlot(slotErrorCpp)

    handled = False
    errorMsg = ""
    detailsMsg = ""

    def handleError(msg, details):
        nonlocal errorMsg, detailsMsg, handled
        errorMsg = msg
        detailsMsg = details
        handled = True

    # Slot raising a pure Python exception
    req = alice.request(bobId, "slotError")
    req.receiveAsync(lambda: None, handleError)
    wait_for_handled()
    assert handled is True
    assert 42 == called

    # This also tests a bit the exception handling:
    # errorMsg has the main message,
    # detailsMsg contains C++ and Python traces.
    # print(f'\n*** errorMsg "{errorMsg}"\n*** detailsMsg "{detailsMsg}"')
    msgWhoFailed = "Remote Exception from bob"
    msgSlotFailure = "Error in slot 'slotError' from alice"
    msgRuntimeError = "What's the universe and the rest?"

    assert msgWhoFailed in errorMsg
    assert msgSlotFailure in errorMsg
    assert msgRuntimeError in errorMsg
    assert "Exception Type....:" not in errorMsg  # also C++ trace

    assert msgWhoFailed in detailsMsg
    assert msgSlotFailure in detailsMsg
    assert msgRuntimeError in detailsMsg
    # C++ trace:
    assert "Exception with trace (listed from inner to outer):" in detailsMsg
    assert "Exception Type....:" in detailsMsg
    # Parts of Python trace
    assert "Traceback (most recent call last):" in detailsMsg
    assert __file__ in detailsMsg   # raising is here in this file
    assert "raise RuntimeError" in detailsMsg

    # Check how exception from underlying C++ looks like
    handled = False
    errorMsg = ""
    detailsMsg = ""
    called = -1

    req = alice.request(bobId, "slotErrorCpp")
    req.receiveAsync(lambda: None, handleError)
    wait_for_handled()
    assert handled is True
    assert 43 == called
    # msgWhoFailed = "Remote Exception from bob" as before
    msgSlotFailure = "Error in slot 'slotErrorCpp' from alice"
    msgKeyFailure = "Key 'non_existing_key' does not exist"
    assert msgWhoFailed in errorMsg
    assert msgSlotFailure in errorMsg
    assert msgKeyFailure in errorMsg
    assert "Exception Type....:" in errorMsg  # also C++ trace

    assert msgWhoFailed in detailsMsg
    assert msgSlotFailure in detailsMsg
    assert msgKeyFailure in errorMsg

    # C++ trace:
    assert "Exception with trace (listed from inner to outer):" in detailsMsg
    assert "Exception Type....:  Python Exception" in detailsMsg
    # Parts of Python trace
    assert "Traceback (most recent call last):" in detailsMsg
    assert __file__ in detailsMsg  # raising is here in this file
    assert 'h["non_existing_key"]' in detailsMsg  # the bad line

    # Test timeout handling
    handled = False
    errorMsg = ""
    detailsMsg = ""
    called = -1

    req = alice.request("non/existing/id", "slotNoMatter")
    req.receiveAsync(lambda: None, handleError, timeoutMs=1)
    wait_for_handled()

    assert "Timeout of asynchronous request" in errorMsg
    assert "Timeout of asynchronous request" in detailsMsg
    assert "Exception Type....:  Timeout Exception" in detailsMsg

    # Forbidden number of return values
    req = alice.request("doesnt/matter/id", "slotNoMatter")
    try:
        req.receiveAsync(lambda: None, numCallbackArgs=5)
    except RuntimeError as e:
        assert "Detected/specified 5 (> 4) arguments" in str(e)
    else:
        assert False, "Did not catch RuntimeError"

    # So far we registered local funcs and lambda, now check
    # member funcs and objects with __call__
    # 1) Object with ordinary __call__ member
    handled = False
    called = -1
    i = 0

    class Handler1:
        def __call__(self, ii):
            nonlocal handled, i
            handled, i = True, ii

    handle1 = Handler1()
    req = alice.request(bobId, "slot1", 5)
    # Number of arguments is properly detected:
    req.receiveAsync(handle1)
    wait_for_handled()
    assert handled is True
    assert 5 == i
    assert 5 == called

    # 2) Object with static __call__ member
    handled = False
    called = -1
    i = 0

    class Handler1a:
        @staticmethod
        def __call__(ii):
            nonlocal handled, i
            handled, i = True, ii

    handle1 = Handler1a()
    req = alice.request(bobId, "slot1", 6)
    # Also here number of arguments is properly detected:
    req.receiveAsync(handle1)
    wait_for_handled()
    assert handled is True
    assert 6 == called
    assert 6 == i

    # 3) Just a method of an object
    handled = False
    called = -1
    i = 0

    req = alice.request(bobId, "slot1", 7)
    # re-use __call__ member of object from above
    req.receiveAsync(handle1.__call__)
    wait_for_handled()
    assert handled is True
    assert 7 == called
    assert 7 == i

    # 4) Bad handler with a non-callable __call__ attribute
    class HandlerBad:
        def __init__(self):
            self.__call__ = None

    handleBad = HandlerBad()
    req = alice.request("doesnt/matter/id", "slotNoMatter")
    with pytest.raises(RuntimeError):
        req.receiveAsync(handleBad)
    called = -1  # reset for next test

    # wait for reply ...
    assert called == -1
    to = 2000  # timeout in ms
    res = alice.request(bobId, "slot0").waitForReply(to)
    assert len(res) == 0
    assert called == 0

    res = alice.request(bobId, "slot1", 5).waitForReply(to)
    assert len(res) == 1
    assert res[0] == 5
    assert called == 5

    req = alice.request(bobId, "slot2", 5, 7)
    res = req.waitForReply(to)
    assert len(res) == 2
    assert res[0] == 5
    assert res[1] == 7
    assert called == 12

    req = alice.request(bobId, "slot3", 5, 7, 11)
    res = req.waitForReply(to)
    assert len(res) == 3
    assert res[0] == 5
    assert res[1] == 7
    assert res[2] == 11
    assert called == 23

    req = alice.request(bobId, "slot4", 5, 7, 11, 13)
    res = req.waitForReply(to)
    assert len(res) == 4
    assert res[0] == 5
    assert res[1] == 7
    assert res[2] == 11
    assert res[3] == 13
    assert called == 36

    # Now error caused by raising Python exception
    req = alice.request(bobId, "slotError")
    with pytest.raises(RuntimeError) as excinfo:
        res = req.waitForReply(to)

    # The error string contains both, user friendly and detailed
    # message, separated by "\nDETAILS: "
    exceptStrs = str(excinfo.value).split("\nDETAILS: ", 1)

    assert len(exceptStrs) == 2, exceptStrs
    (friendlyMsg, details) = exceptStrs
    msgWhoFailed = "Remote Exception from bob"
    msgSlotFailure = "Error in slot 'slotError' from alice"
    msgRuntimeError = ("RuntimeError: "
                       "What's the universe and the rest?")
    assert msgWhoFailed in friendlyMsg
    assert msgSlotFailure in friendlyMsg
    assert msgRuntimeError in friendlyMsg
    assert msgWhoFailed in details, str(exceptStrs)
    assert msgSlotFailure in details
    assert msgRuntimeError in details
    # C++ trace:
    assert "Exception with trace (listed from inner to outer):" in details
    assert "Exception Type....:" in details
    # Parts of Python trace
    assert "Traceback (most recent call last):" in details
    assert __file__ in details  # raising is in this file
    assert "raise RuntimeError" in details

    # Now error caused by throwing C++ exception in C++ called from Py
    req = alice.request(bobId, "slotErrorCpp")
    with pytest.raises(RuntimeError) as excinfo:
        res = req.waitForReply(to)

    # Again, friendly and detailed message, separated by
    # "\nDETAILS: "
    exceptStrs = str(excinfo.value).split("\nDETAILS: ", 1)
    assert len(exceptStrs) == 2, exceptStrs
    (friendlyMsg, details) = exceptStrs
    msgWhoFailed = "Remote Exception from bob"
    msgSlotFailure = "Error in slot 'slotErrorCpp' from alice"
    msgKeyFailure = "Key 'non_existing_key' does not exist"
    assert msgWhoFailed in friendlyMsg
    assert msgSlotFailure in friendlyMsg
    assert msgKeyFailure in friendlyMsg

    assert msgWhoFailed in details
    assert msgSlotFailure in details
    assert msgKeyFailure in details

    # C++ trace:
    assert "Exception with trace (listed from inner to outer):" in details
    assert "Exception Type....:  Python Exception" in details
    # Parts of Python trace
    assert "Traceback (most recent call last):" in details
    assert __file__ in details   # raising is this file
    assert 'h["non_existing_key"]' in details  # the bad line

    # Now timeout error
    req = alice.request("not/an/instance", "slotDoesNotMatter")
    with pytest.raises(TimeoutError):
        req.waitForReply(2)  # ms - short timeout intended

    del req


def test_sigslot_requestNoWait(eventLoopFixt):
    greeter = SignalSlotable("greeter")
    responder = SignalSlotable("responder")

    def slotA(value):
        responder.reply(value * 2)

    responder.registerSlot(slotA)

    replyCalled = repliedValue = None

    def slotReplyOfA(replyValue):
        nonlocal replyCalled, repliedValue
        repliedValue = replyValue
        replyCalled = True

    greeter.registerSlot(slotReplyOfA)

    greeter.start()
    responder.start()

    greeter.requestNoWait("responder", "slotA", "slotReplyOfA", 21)

    def wait_for_replyCalled():
        max_count = 2000
        while replyCalled is None and max_count > 0:
            time.sleep(0.01)
            max_count -= 1

    wait_for_replyCalled()

    assert replyCalled is True
    assert repliedValue == 42


def test_sigslot_asyncreply(eventLoopFixt):

    bobId = "bob" + str(uuid.uuid4())
    bob = SignalSlotable(bobId)
    bob.start()
    called = -1

    def asyncSlot0():
        aReply = bob.createAsyncReply()

        def replyer():
            nonlocal called
            called = 0
            aReply()

        # Call replyer on event loop after 100 ms
        EventLoop.post(replyer, 0.1)

    bob.registerSlot(asyncSlot0)

    def asyncSlot1(i):
        aReply = bob.createAsyncReply()

        def replyer():
            nonlocal called
            called = i
            aReply(i)

        # Call replyer on event loop after 10 ms
        EventLoop.post(replyer, 0.01)

    bob.registerSlot(asyncSlot1)

    def asyncSlot2(i, j):
        aReply = bob.createAsyncReply()

        def replyer():
            nonlocal called
            called = i + j
            aReply(i, j)

        # Call replyer on event loop
        EventLoop.post(replyer)

    bob.registerSlot(asyncSlot2)

    def asyncSlot3(i, j, k):
        aReply = bob.createAsyncReply()

        def replyer():
            nonlocal called
            called = i + j + k
            aReply(i, j, k)

        # Call replyer on event loop
        EventLoop.post(replyer)

    bob.registerSlot(asyncSlot3)

    def asyncSlot4(i, j, k, n):
        aReply = bob.createAsyncReply()

        def replyer():
            nonlocal called
            called = i + j + k + n
            aReply(i, j, k, n)

        # Call replyer on event loop
        EventLoop.post(replyer)

    bob.registerSlot(asyncSlot4)

    def asyncSlotError(errorMessage, details):
        aReply = bob.createAsyncReply()

        def replyer():
            aReply.error(errorMessage, details)

        # Call replyer on event loop
        EventLoop.post(replyer)

    bob.registerSlot(asyncSlotError)

    # setup other SignalSlotable to call the slots
    aliceId = "alice" + str(uuid.uuid4())
    alice = SignalSlotable(aliceId)
    alice.start()

    assert called == -1
    to = 2000  # timeout in ms
    req = alice.request(bobId, "asyncSlot0")
    res = req.waitForReply(to)
    assert 0 == len(res)
    assert 0 == called

    called = -1
    req = alice.request(bobId, "asyncSlot1", 5)
    res = req.waitForReply(to)
    assert 1 == len(res)
    assert 5 == res[0]
    assert 5 == called

    called = -1
    req = alice.request(bobId, "asyncSlot2", 5, 7)
    res = req.waitForReply(to)
    assert 2 == len(res)
    assert 5 == res[0]
    assert 7 == res[1]
    assert 12 == called

    called = -1
    req = alice.request(bobId, "asyncSlot3", 5, 7, 11)
    res = req.waitForReply(to)
    assert 3 == len(res)
    assert 5 == res[0]
    assert 7 == res[1]
    assert 11 == res[2]
    assert 23 == called

    called = -1
    req = alice.request(bobId, "asyncSlot4", 5, 7, 11, 13)
    res = req.waitForReply(to)
    assert 4 == len(res)
    assert 5 == res[0]
    assert 7 == res[1]
    assert 11 == res[2]
    assert 13 == res[3]
    assert 36 == called
    del res

    msg = "Bad things happen!"
    details = "No details nor trace!"
    req = alice.request(bobId, "asyncSlotError", msg, details)
    with pytest.raises(RuntimeError) as excinfo:
        req.waitForReply(to)

    # Again, friendly and detailed message, separated by
    # "\nDETAILS: "
    exceptStrs = str(excinfo.value).split("\nDETAILS: ", 1)
    assert 2 == len(exceptStrs), exceptStrs
    (friendlyMsg, detailsMsg) = exceptStrs
    msgWhoFailed = "Remote Exception from bob"
    assert msgWhoFailed in friendlyMsg
    assert msg in friendlyMsg
    assert msgWhoFailed in detailsMsg
    assert details in detailsMsg
    del req


def test_sigslot_asyncConnect(eventLoopFixt):
    signaler = SignalSlotable("signalInstance")
    signaler.registerSignal("signal", int)
    signaler.registerSignal("not_connected_signal", str)
    signaler.start()

    slotter = SignalSlotable("slotInstance")

    slotCalled = False
    inSlot = -10

    def slotFunc(i):
        nonlocal slotCalled, inSlot
        inSlot += i
        slotCalled = True

    slotter.registerSlot(slotFunc, "slot")
    slotter.start()

    # First test successful connectAsync
    failureMsg = None
    failureDetails = None

    def connectCallback(failMsg, failDetails):
        nonlocal failureMsg, failureDetails
        failureMsg = failMsg
        failureDetails = failDetails

    slotter.asyncConnect("signalInstance", "signal", "slot", connectCallback)

    def wait_for_callback():
        max_count = 2000
        while failureMsg is None and failureDetails is None and max_count > 0:
            time.sleep(0.005)
            max_count -= 1

    wait_for_callback()

    assert failureMsg is not None  # callback called
    assert failureMsg == ""        # connect succeeded

    signaler.emit("signal", 52)

    def wait_for_slotCalled(max_count=2000):
        count = max_count
        while not slotCalled and count > 0:
            time.sleep(0.005)
            count -= 1

    wait_for_slotCalled()

    assert slotCalled
    assert 42 == inSlot

    ##################################################
    # Test failure handling

    # Non-existing signal gives failure - but in Karabo 3 only since
    # signalInstance is in same process, otherwise just subscribes to broker
    failureMsg = failureDetails = None

    slotter.asyncConnect("signalInstance", "NOT_A_signal", "slot",
                         connectCallback)
    wait_for_callback()

    assert failureMsg is not None  # callback called
    assert "no signal 'NOT_A_signal'" in failureMsg
    # Details contain the same, but also C++ exception etails
    assert "no signal 'NOT_A_signal'" in failureDetails
    assert "Exception Type....:  SignalSlot Exception" in failureDetails

    # Non-existing slot gives failure
    failureMsg = failureDetails = None

    slotter.asyncConnect("signalInstance", "signal", "NOT_A_slot",
                         connectCallback)
    wait_for_callback()

    assert failureMsg is not None  # callback called
    assert "no slot 'NOT_A_slot'" in failureMsg
    # Details contain the same, but also C++ exception etails
    assert "no slot 'NOT_A_slot'" in failureDetails
    assert "Exception Type....:  SignalSlot Exception" in failureDetails

    #################################################################
    # Now asyncronously disconnect
    failureMsg = failureDetails = None

    slotter.asyncDisconnect("signalInstance", "signal", "slot",
                            connectCallback)
    wait_for_callback()

    assert failureMsg is not None  # callback called
    assert failureMsg == ""        # successfully disconnected

    slotCalled = False
    signaler.emit("signal", 77)

    # wait only a bit (10 means 50 ms) for something not coming
    wait_for_slotCalled(10)
    assert not slotCalled

    #################################################################
    # Finally a failing asyncDisconnect
    failureMsg = failureDetails = None

    slotter.asyncDisconnect("signalInstance", "signal", "not_a_slot",
                            connectCallback)
    wait_for_callback()

    assert failureMsg is not None  # callback called
    assert "no slot 'not_a_slot'" in failureMsg
    assert "no slot 'not_a_slot'" in failureDetails
    assert "Exception Type....:  SignalSlot Exception" in failureDetails
