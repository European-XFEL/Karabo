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
import unittest
from functools import partial

from karabo.bound import EventLoop, Hash, SignalSlotable

# To switch on debugging, also import these:
# from karabo.bound import Logger, Hash


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


class Xms_TestCase(unittest.TestCase):
    bob_id = "bob"
    alice_id = "alice"

    def setUp(self):
        # To switch on logging to debug:
        # Logger.configure(Hash("priority", "DEBUG"))
        # Logger.useOstream()

        # start event loop
        self._eventLoopThread = threading.Thread(target=EventLoop.work)
        self._eventLoopThread.daemon = True
        self._eventLoopThread.start()

    def tearDown(self):
        # Stop the event loop
        EventLoop.stop()
        self._eventLoopThread.join()
        del self._eventLoopThread

    def test_xms_slotRegistration(self):
        timeout = 2000  # timeout in ms

        sigSlot = SignalSlotableWithSlots("sigSlot")
        sigSlot.start()

        with self.subTest(msg="register function"):
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
            self.assertEqual(0, nArg)
            # TODO: Do we want to guarantee this 'feature' of ignored
            #       extra arguments? It's in underlying C++ code.
            req = sigSlot.request("", "funcVarArgs", 1)
            (nArg, ) = req.waitForReply(timeout)
            self.assertEqual(0, nArg)  # i.e. input argument ignored!

            # We can register '*args' function with fixed number
            # (Also test that '_' can be requested as '.' is needed
            #  for nested slots in Schema)
            sigSlot.registerSlot(funcVarArgs, "func_varArgs1", 1)
            req = sigSlot.request("", "func.varArgs1", "no_matter2")
            (nArg, ) = req.waitForReply(timeout)
            self.assertEqual(1, nArg)

            # Then we must not miss to sent argument
            req = sigSlot.request("", "func_varArgs1")  # here access with '_'
            with self.assertRaises(RuntimeError):
                req.waitForReply(timeout)

            #
            # Register function with default argument.
            #
            # Automatic number of argument detection includes the default
            # (cannot change that for backward compatibility...).
            sigSlot.registerSlot(funcTwoArgDef, "funcTwoArgDef3")
            req = sigSlot.request("", "funcTwoArgDef3", 1, 2, 3)
            (theSum, ) = req.waitForReply(timeout)
            self.assertEqual(6, theSum)

            # But it is OK to register with less args to get default
            sigSlot.registerSlot(funcTwoArgDef, numArgs=2)
            req = sigSlot.request("", "funcTwoArgDef", 2, 3)
            (theSum, ) = req.waitForReply(timeout)
            self.assertEqual(47, theSum)  # 2 + 3 + 42

            # Still, we must not miss to sent required arguments
            req = sigSlot.request("", "funcTwoArgDef", 1)
            with self.assertRaises(RuntimeError):
                req.waitForReply(timeout)

            #
            # Register function with default keyword only argument.
            #
            # Automatic number of argument detection excludes the default
            sigSlot.registerSlot(funcKwargs)
            req = sigSlot.request("", "funcKwargs")
            (length, a) = req.waitForReply(timeout)
            self.assertEqual(0, length)
            self.assertEqual(77, a)

            # Can specify whatever number of args, but keyword only arg is fix:
            sigSlot.registerSlot(funcKwargs, "funcKwargs1", 1)
            req = sigSlot.request("", "funcKwargs1", -77)
            (length, a) = req.waitForReply(timeout)
            self.assertEqual(1, length)
            self.assertEqual(77, a)

            sigSlot.registerSlot(funcKwargs, "funcKwargs2", 2)
            req = sigSlot.request("", "funcKwargs2", 111, 222)
            (length, a) = req.waitForReply(timeout)
            self.assertEqual(2, length)
            self.assertEqual(77, a)

        with self.subTest(msg="register member method"):
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
            self.assertEqual(0, nArg)
            # TODO: Do we want to guarantee this 'feature' of ignored
            #       extra arguments? It's in underlying C++ code.
            req = sigSlot.request("", "slotVarArgs", "extra_arg")
            (nArg, ) = req.waitForReply(timeout)
            self.assertEqual(0, nArg)  # i.e. input argument ignored!

            # We can register '*args' function with fixed number
            sigSlot.registerSlot(sigSlot.slotVarArgs, "slotVarArgs1", 2)
            req = sigSlot.request("", "slotVarArgs1", "no", "matter")
            (nArg, ) = req.waitForReply(timeout)
            self.assertEqual(2, nArg)

            # Then we must not miss to sent argument
            req = sigSlot.request("", "slotVarArgs1", "not_enough")
            with self.assertRaises(RuntimeError):
                req.waitForReply(timeout)

            #
            # Register method with default argument.
            #
            # Automatic number of argument detection includes the default
            # (cannot change that for backward compatibility...).
            sigSlot.registerSlot(sigSlot.slotTwoArgDef, "slotTwoArgDef3")
            req = sigSlot.request("", "slotTwoArgDef3", 1, 2, 3)
            (theSum, ) = req.waitForReply(timeout)
            self.assertEqual(6, theSum)

            # But it is OK to register with less args to get default
            sigSlot.registerSlot(sigSlot.slotTwoArgDef, numArgs=2)
            req = sigSlot.request("", "slotTwoArgDef", 1, 2)
            (theSum, ) = req.waitForReply(timeout)
            self.assertEqual(45, theSum)  # 1 + 2 + 42

            # Still, we must not miss to sent required arguments
            req = sigSlot.request("", "slotTwoArgDef", 1)
            with self.assertRaises(RuntimeError):
                req.waitForReply(timeout)

            #
            # Register method with default keyword only argument.
            #
            # Automatic number of argument detection excludes the default
            sigSlot.registerSlot(sigSlot.slotKwargs)
            req = sigSlot.request("", "slotKwargs")
            (length, a) = req.waitForReply(timeout)
            self.assertEqual(0, length)
            self.assertEqual(77, a)

            # Can specify whatever number of args, but keyword only arg is fix:
            sigSlot.registerSlot(sigSlot.slotKwargs, "slotKwargs1", 1)
            req = sigSlot.request("", "slotKwargs1", -77)
            (length, a) = req.waitForReply(timeout)
            self.assertEqual(1, length)
            self.assertEqual(77, a)

            sigSlot.registerSlot(sigSlot.slotKwargs, "slotKwargs2", 2)
            req = sigSlot.request("", "slotKwargs2", 111, 222)
            (length, a) = req.waitForReply(timeout)
            self.assertEqual(2, length)
            self.assertEqual(77, a)

        with self.subTest(msg="register partial, lambda"):

            # Lambdas have a (not so unique) __name__, taken by default.
            # Auto detection of num(arguments) works, default arg don't harm.
            sigSlot.registerSlot(lambda a, b, c, d=55: sigSlot.reply(a, b,
                                                                     c, d))
            req = sigSlot.request("", "<lambda>", 1, 2, 3, 5)
            l, m, n, o = req.waitForReply(timeout)
            self.assertEqual(1, l)
            self.assertEqual(2, m)
            self.assertEqual(3, n)
            self.assertEqual(5, o)

            # Make actually use of lambda's default argument
            sigSlot.registerSlot(lambda a, b, c, d=55: sigSlot.reply(a, b,
                                                                     c, d),
                                 "lam", 3)
            req = sigSlot.request("", "lam", 8, 13, 21)
            l, m, n, o = req.waitForReply(timeout)
            self.assertEqual(8, l)
            self.assertEqual(13, m)
            self.assertEqual(21, n)
            self.assertEqual(55, o)

            # Test that 'partial' works for slots - they have no __name__
            def forPartial(a, b):
                sigSlot.reply(a, b)

            part = partial(forPartial, 42)  # 'default' for a
            # Need to define slotName and numArgs, auto detection fails
            sigSlot.registerSlot(part, "partial", 1)
            req = sigSlot.request("", "partial", 43)
            a, b = req.waitForReply(timeout)
            self.assertEqual(42, a)
            self.assertEqual(43, b)

    def setUpAliceBob(self):

        # setup bob SignalSlotable with slots and asyncSlots for 0-4 arguments
        # that also reply 0-4 values
        # (asyncSlots 'delay' their response using AsyncReply)
        self.called = -1
        self.bob = SignalSlotable(self.bob_id)
        self.bob.start()

        def slotError():
            self.called = 42
            raise RuntimeError("What's the universe and the rest?")
        self.bob.registerSlot(slotError)

        def slotErrorCpp():
            # A slot triggering an exception in Karabo C++ code
            self.called = 43
            h = Hash()
            h["non_existing_key"]
        self.bob.registerSlot(slotErrorCpp)

        def slot0():
            self.called = 0
            self.bob.reply()
        self.bob.registerSlot(slot0)

        def slot1(i):
            self.called = i
            self.bob.reply(i)
        self.bob.registerSlot(slot1)

        def slot2(i, j):
            self.called = i + j
            self.bob.reply(i, j)
        self.bob.registerSlot(slot2)

        def slot3(i, j, k):
            self.called = i + j + k
            self.bob.reply(i, j, k)
        self.bob.registerSlot(slot3)

        def slot4(i, j, k, l1):  # flake does not like l: 'ambiguous var. name'
            self.called = i + j + k + l1
            self.bob.reply(i, j, k, l1)
        self.bob.registerSlot(slot4)

        def asyncSlot0():
            aReply = self.bob.createAsyncReply()

            def replyer():
                self.called = 0
                aReply()

            # Call replyer on event loop after 100 ms
            EventLoop.post(replyer, 0.1)
        self.bob.registerSlot(asyncSlot0)

        def asyncSlot1(i):
            aReply = self.bob.createAsyncReply()

            def replyer():
                self.called = i
                aReply(i)

            # Call replyer on event loop after 10 ms
            EventLoop.post(replyer, 0.01)
        self.bob.registerSlot(asyncSlot1)

        def asyncSlot2(i, j):
            aReply = self.bob.createAsyncReply()

            def replyer():
                self.called = i + j
                aReply(i, j)

            # Call replyer on event loop
            EventLoop.post(replyer)
        self.bob.registerSlot(asyncSlot2)

        def asyncSlot3(i, j, k):
            aReply = self.bob.createAsyncReply()

            def replyer():
                self.called = i + j + k
                aReply(i, j, k)

            # Call replyer on event loop
            EventLoop.post(replyer)
        self.bob.registerSlot(asyncSlot3)

        def asyncSlot4(i, j, k, l1):
            aReply = self.bob.createAsyncReply()

            def replyer():
                self.called = i + j + k + l1
                aReply(i, j, k, l1)

            # Call replyer on event loop
            EventLoop.post(replyer)
        self.bob.registerSlot(asyncSlot4)

        def asyncSlotError(errorMessage, details):
            aReply = self.bob.createAsyncReply()

            def replyer():
                aReply.error(errorMessage, details)

            # Call replyer on event loop
            EventLoop.post(replyer)
        self.bob.registerSlot(asyncSlotError)

        # setup other SignalSlotable to call the slots
        self.alice = SignalSlotable(self.alice_id)
        self.alice.start()

    def tearDownAliceBob(self):
        self.alice = None
        self.bob = None

    def test_xms_request(self):
        self.setUpAliceBob()

        with self.subTest(msg="async receive"):
            self.assertEqual(-1, self.called)  # initial value

            # Helper to check when registered handlers are called
            handled = False

            def wait_for_handled():
                max_count = 1000
                while not handled and max_count > 0:
                    time.sleep(0.01)
                    max_count -= 1

            # Test no arguments
            def handle0():
                nonlocal handled
                handled = True
            req = self.alice.request(self.bob_id, "slot0")
            req.receiveAsync0(handle0)
            wait_for_handled()
            self.assertEqual(0, self.called)
            # test again via return-argument-generic receiveAsync
            handled = False
            self.called = -1
            req = self.alice.request(self.bob_id, "slot0")
            req.receiveAsync(handle0)
            wait_for_handled()
            self.assertEqual(0, self.called)

            # Test one argument
            handled = False
            i = 0

            def handle1(ii):
                nonlocal handled, i
                handled = True
                i = ii
            req = self.alice.request(self.bob_id, "slot1", 5)
            req.receiveAsync1(handle1)
            wait_for_handled()
            self.assertEqual(5, self.called)
            self.assertEqual(5, i)
            # test again via return-argument-generic receiveAsync
            handled = False
            i = 0
            self.called = -1
            req = self.alice.request(self.bob_id, "slot1", 5)
            req.receiveAsync(handle1)
            wait_for_handled()
            self.assertEqual(5, self.called)
            self.assertEqual(5, i)

            # Test two arguments
            handled = False
            i = j = 0

            def handle2(ii, jj):
                nonlocal handled, i, j
                handled = True
                i, j = ii, jj
            req = self.alice.request(self.bob_id, "slot2", 5, 7)
            req.receiveAsync2(handle2)
            wait_for_handled()
            self.assertEqual(12, self.called)
            self.assertEqual(5, i)
            self.assertEqual(7, j)
            # test again via return-argument-generic receiveAsync
            handled = False
            i = j = 0
            self.called = -1
            req = self.alice.request(self.bob_id, "slot2", 5, 7)
            req.receiveAsync(handle2)
            wait_for_handled()
            self.assertEqual(12, self.called)
            self.assertEqual(5, i)
            self.assertEqual(7, j)

            # Test three arguments
            handled = False
            i = j = k = 0

            def handle3(ii, jj, kk):
                nonlocal handled, i, j, k
                handled = True
                i, j, k = ii, jj, kk
            req = self.alice.request(self.bob_id, "slot3", 5, 7, 11)
            req.receiveAsync3(handle3)
            wait_for_handled()
            self.assertEqual(23, self.called)
            self.assertEqual(5, i)
            self.assertEqual(7, j)
            self.assertEqual(11, k)
            # test again via return-argument-generic receiveAsync
            handled = False
            i = j = k = 0
            self.called = -1
            req = self.alice.request(self.bob_id, "slot3", 5, 7, 11)
            req.receiveAsync(handle3)
            wait_for_handled()
            self.assertEqual(23, self.called)
            self.assertEqual(5, i)
            self.assertEqual(7, j)
            self.assertEqual(11, k)

            # Test four arguments
            handled = False
            i = j = k = l1 = 0  # flake does not like variable named 'l'

            def handle4(ii, jj, kk, ll):
                nonlocal handled, i, j, k, l1
                handled = True
                i, j, k, l1 = ii, jj, kk, ll
            req = self.alice.request(self.bob_id, "slot4", 5, 7, 11, 13)
            req.receiveAsync4(handle4)
            wait_for_handled()
            self.assertEqual(36, self.called)
            self.assertEqual(5, i)
            self.assertEqual(7, j)
            self.assertEqual(11, k)
            self.assertEqual(13, l1)
            # test again via return-argument-generic receiveAsync
            handled = False
            i = j = k = l1, 0
            self.called = -1
            req = self.alice.request(self.bob_id, "slot4", 5, 7, 11, 13)
            req.receiveAsync(handle4)
            wait_for_handled()
            self.assertEqual(36, self.called)
            self.assertEqual(5, i)
            self.assertEqual(7, j)
            self.assertEqual(11, k)
            self.assertEqual(13, l1)

            # Test error handling
            handled = False
            errorMsg = ""
            detailsMsg = ""
            self.called = -1

            def handleError(msg, details):
                nonlocal errorMsg, detailsMsg, handled
                errorMsg = msg
                detailsMsg = details
                handled = True

            # Slot raising a pure Python exception
            req = self.alice.request(self.bob_id, "slotError")
            req.receiveAsync(lambda: None, handleError)
            wait_for_handled()
            self.assertEqual(42, self.called)

            # This also tests a bit the exception handling:
            # errorMsg has the main message,
            # detailsMsg contains C++ and Python traces.
            msgWhoFailed = "Remote Exception from bob"
            msgSlotFailure = 'Error in slot "slotError"'
            msgRuntimeError = "What's the universe and the rest?"
            self.assertIn(msgWhoFailed, errorMsg)
            self.assertIn(msgSlotFailure, errorMsg)
            self.assertIn(msgRuntimeError, errorMsg)
            self.assertNotIn("Exception Type....:", errorMsg)  # also C++ trace

            self.assertIn(msgWhoFailed, detailsMsg)
            self.assertIn(msgSlotFailure, detailsMsg)
            self.assertIn(msgRuntimeError, detailsMsg)
            # C++ trace:
            self.assertIn("Exception with trace (listed from inner to outer):",
                          detailsMsg)
            self.assertIn("Exception Type....:", detailsMsg)
            # Parts of Python trace
            self.assertIn("Traceback (most recent call last):", detailsMsg)
            self.assertIn(__file__, detailsMsg)  # raising is here in this file
            self.assertIn("raise RuntimeError", detailsMsg)

            # Check how exception from underlying C++ looks like
            handled = False
            errorMsg = ""
            detailsMsg = ""
            self.called = -1

            req = self.alice.request(self.bob_id, "slotErrorCpp")
            req.receiveAsync(lambda: None, handleError)
            wait_for_handled()
            self.assertEqual(43, self.called)
            # msgWhoFailed = "Remote Exception from bob" as before
            msgSlotFailure = 'Error in slot "slotErrorCpp"'
            msgKeyFailure = "Key 'non_existing_key' does not exist"
            self.assertIn(msgWhoFailed, errorMsg)
            self.assertIn(msgSlotFailure, errorMsg)
            self.assertIn(msgKeyFailure, errorMsg)
            self.assertIn("Exception Type....:", errorMsg)  # also C++ trace

            self.assertIn(msgWhoFailed, detailsMsg)
            self.assertIn(msgSlotFailure, detailsMsg)
            self.assertIn(msgKeyFailure, errorMsg)

            # C++ trace:
            self.assertIn("Exception with trace (listed from inner to outer):",
                          detailsMsg)
            self.assertIn("Exception Type....:  Python Exception", detailsMsg)
            # Parts of Python trace
            self.assertIn("Traceback (most recent call last):", detailsMsg)
            self.assertIn(__file__, detailsMsg)  # raising is here in this file
            self.assertIn('h["non_existing_key"]', detailsMsg)  # the bad line

            # Test timeout handling
            handled = False
            errorMsg = ""
            detailsMsg = ""

            req = self.alice.request("non/existing/id", "slotNoMatter")
            req.receiveAsync(lambda: None, handleError, timeoutMs=1)
            wait_for_handled()
            self.assertIn("Timeout of asynchronous request", errorMsg)

            self.assertIn("Timeout of asynchronous request", detailsMsg)
            self.assertIn("Exception Type....:  Timeout Exception", detailsMsg)

            # Forbidden number of return values
            req = self.alice.request("doesnt/matter/id", "slotNoMatter")
            try:
                req.receiveAsync(lambda: None, numCallbackArgs=5)
            except RuntimeError as e:
                self.assertIn("Detected/specified 5 (> 4) arguments", str(e))
            else:
                self.assertTrue(False, "Did not catch RuntimeError")

            # So far we registered local funcs and lambda, now check
            # member funcs and objects with __call__
            # 1) Object with ordinary __call__ member
            handled = False
            i = 0

            class Handler1:
                def __call__(self, ii):
                    nonlocal handled, i
                    handled, i = True, ii

            handle1 = Handler1()
            req = self.alice.request(self.bob_id, "slot1", 5)
            # Number of arguments is properly detected:
            req.receiveAsync(handle1)
            wait_for_handled()
            self.assertEqual(5, self.called)
            self.assertEqual(5, i)

            # 2) Object with static __call__ member
            handled = False
            i = 0

            class Handler1a:
                @staticmethod
                def __call__(ii):
                    nonlocal handled, i
                    handled, i = True, ii

            handle1 = Handler1a()
            req = self.alice.request(self.bob_id, "slot1", 6)
            # Also here number of arguments is properly detected:
            req.receiveAsync(handle1)
            wait_for_handled()
            self.assertEqual(6, self.called)
            self.assertEqual(6, i)

            # 3) Just a method of an object
            handled = False
            i = 0

            req = self.alice.request(self.bob_id, "slot1", 7)
            # re-use __call__ member of object from above
            req.receiveAsync(handle1.__call__)
            wait_for_handled()
            self.assertEqual(7, self.called)
            self.assertEqual(7, i)

            # 4) Bad handler with a non-callable __call__ attribute
            class HandlerBad:
                def __init__(self):
                    self.__call__ = None

            handleBad = HandlerBad()
            req = self.alice.request("doesnt/matter/id", "slotNoMatter")
            with self.assertRaises(RuntimeError):
                req.receiveAsync(handleBad)

            self.called = -1  # reset for next test

        with self.subTest(msg="wait for reply"):
            self.assertEqual(-1, self.called)  # initial value
            to = 2000  # timeout in ms
            res = self.alice.request(self.bob_id, "slot0").waitForReply(to)
            self.assertEqual(0, len(res))
            self.assertEqual(0, self.called)

            res = self.alice.request(self.bob_id, "slot1", 5).waitForReply(to)
            self.assertEqual(1, len(res))
            self.assertEqual(5, res[0])
            self.assertEqual(5, self.called)

            req = self.alice.request(self.bob_id, "slot2", 5, 7)
            res = req.waitForReply(to)
            self.assertEqual(2, len(res))
            self.assertEqual(5, res[0])
            self.assertEqual(7, res[1])
            self.assertEqual(12, self.called)

            req = self.alice.request(self.bob_id, "slot3", 5, 7, 11)
            res = req.waitForReply(to)
            self.assertEqual(3, len(res))
            self.assertEqual(5, res[0])
            self.assertEqual(7, res[1])
            self.assertEqual(11, res[2])
            self.assertEqual(23, self.called)

            req = self.alice.request(self.bob_id, "slot4", 5, 7, 11, 13)
            res = req.waitForReply(to)
            self.assertEqual(4, len(res))
            self.assertEqual(5, res[0])
            self.assertEqual(7, res[1])
            self.assertEqual(11, res[2])
            self.assertEqual(13, res[3])
            self.assertEqual(36, self.called)

            # Now error caused by raising Python exception
            req = self.alice.request(self.bob_id, "slotError")
            with self.assertRaises(RuntimeError) as cm:
                res = req.waitForReply(to)

            # The error string contains both, user friendly and detailed
            # message, separated by "\nDETAILS: "
            exceptStrs = str(cm.exception).split("\nDETAILS: ", 1)

            self.assertEqual(2, len(exceptStrs), exceptStrs)
            (friendlyMsg, details) = exceptStrs
            msgWhoFailed = "Remote Exception from bob"
            msgSlotFailure = 'Error in slot "slotError"'
            msgRuntimeError = ("RuntimeError: "
                               "What's the universe and the rest?")
            self.assertIn(msgWhoFailed, friendlyMsg)
            self.assertIn(msgSlotFailure, friendlyMsg)
            self.assertIn(msgRuntimeError, friendlyMsg)
            self.assertIn(msgWhoFailed, details, str(exceptStrs))
            self.assertIn(msgSlotFailure, details)
            self.assertIn(msgRuntimeError, details)
            # C++ trace:
            self.assertIn("Exception with trace (listed from inner to outer):",
                          details)
            self.assertIn("Exception Type....:", details)
            # Parts of Python trace
            self.assertIn("Traceback (most recent call last):", details)
            self.assertIn(__file__, details)  # raising is in this file
            self.assertIn("raise RuntimeError", details)

            # Now error caused by throwing C++ exception in C++ called from Py
            req = self.alice.request(self.bob_id, "slotErrorCpp")
            with self.assertRaises(RuntimeError) as cm:
                res = req.waitForReply(to)

            # Again, friendly and detailed message, separated by
            # "\nDETAILS: "
            exceptStrs = str(cm.exception).split("\nDETAILS: ", 1)
            self.assertEqual(2, len(exceptStrs), exceptStrs)
            (friendlyMsg, details) = exceptStrs
            msgWhoFailed = "Remote Exception from bob"
            msgSlotFailure = 'Error in slot "slotErrorCpp"'
            msgKeyFailure = "Key 'non_existing_key' does not exist"
            self.assertIn(msgWhoFailed, friendlyMsg)
            self.assertIn(msgSlotFailure, friendlyMsg)
            self.assertIn(msgKeyFailure, friendlyMsg)

            self.assertIn(msgWhoFailed, details)
            self.assertIn(msgSlotFailure, details)
            self.assertIn(msgKeyFailure, details)

            # C++ trace:
            self.assertIn("Exception with trace (listed from inner to outer):",
                          details)
            self.assertIn("Exception Type....:  Python Exception",
                          details)
            # Parts of Python trace
            self.assertIn("Traceback (most recent call last):", details)
            self.assertIn(__file__, details)  # raising is this file
            self.assertIn('h["non_existing_key"]', details)  # the bad line

            # Now timeout error
            req = self.alice.request("not/an/instance", "slotDoesNotMatter")
            with self.assertRaises(TimeoutError):
                req.waitForReply(2)  # ms - short timeout intended

            del req
            self.called = -1

        with self.subTest(msg="wait for AsyncReply"):
            self.assertEqual(-1, self.called)  # initial value
            to = 2000  # timeout in ms
            req = self.alice.request(self.bob_id, "asyncSlot0")
            res = req.waitForReply(to)
            self.assertEqual(0, len(res))
            self.assertEqual(0, self.called)

            req = self.alice.request(self.bob_id, "asyncSlot1", 5)
            res = req.waitForReply(to)
            self.assertEqual(1, len(res))
            self.assertEqual(5, res[0])
            self.assertEqual(5, self.called)

            req = self.alice.request(self.bob_id, "asyncSlot2", 5, 7)
            res = req.waitForReply(to)
            self.assertEqual(2, len(res))
            self.assertEqual(5, res[0])
            self.assertEqual(7, res[1])
            self.assertEqual(12, self.called)

            req = self.alice.request(self.bob_id, "asyncSlot3", 5, 7, 11)
            res = req.waitForReply(to)
            self.assertEqual(3, len(res))
            self.assertEqual(5, res[0])
            self.assertEqual(7, res[1])
            self.assertEqual(11, res[2])
            self.assertEqual(23, self.called)

            req = self.alice.request(self.bob_id, "asyncSlot4", 5, 7, 11, 13)
            res = req.waitForReply(to)
            self.assertEqual(4, len(res))
            self.assertEqual(5, res[0])
            self.assertEqual(7, res[1])
            self.assertEqual(11, res[2])
            self.assertEqual(13, res[3])
            self.assertEqual(36, self.called)

            msg = "Bad things happen!"
            details = "No details nor trace!"
            req = self.alice.request(self.bob_id, "asyncSlotError",
                                     msg, details)
            with self.assertRaises(RuntimeError) as cm:
                res = req.waitForReply(to)

            # Again, friendly and detailed message, separated by
            # "\nDETAILS: "
            exceptStrs = str(cm.exception).split("\nDETAILS: ", 1)
            self.assertEqual(2, len(exceptStrs), exceptStrs)
            (friendlyMsg, detailsMsg) = exceptStrs
            msgWhoFailed = "Remote Exception from bob"
            self.assertIn(msgWhoFailed, friendlyMsg)
            self.assertIn(msg, friendlyMsg)
            self.assertIn(msgWhoFailed, detailsMsg)
            self.assertIn(details, detailsMsg)

            del req
            self.called = -1

        self.tearDownAliceBob()
