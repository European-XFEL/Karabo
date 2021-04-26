from functools import partial
import unittest
import threading
import time

from karabo.bound import SignalSlotable, EventLoop
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
            # (Also test that '_' can be requested as '.' as needed
            #  for nested slots in Schema)
            sigSlot.registerSlot(funcVarArgs, "func_varArgs1", 1)
            req = sigSlot.request("", "func.varArgs1", "no_matter2")
            (nArg, ) = req.waitForReply(timeout)
            self.assertEqual(1, nArg)

            # Then we must not miss to sent argument
            # FIXME: Not yet 2.11. - interface change
            # req = sigSlot.request("", "func_varArgs1")
            # with self.assertRaises(RuntimeError):
            #     req.waitForReply(timeout)

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
            # FIXME: Not yet 2.11. - interface change
            # req = sigSlot.request("", "funcTwoArgDef", 1)
            # with self.assertRaises(RuntimeError):
            #     req.waitForReply(timeout)

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
            # FIXME: Not yet 2.11. - interface change
            # req = sigSlot.request("", "slotVarArgs1", "not_enough")
            # with self.assertRaises(RuntimeError):
            #     req.waitForReply(timeout)

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
            # FIXME: Not yet 2.11. - interface change
            # req = sigSlot.request("", "slotTwoArgDef", 1)
            # with self.assertRaises(RuntimeError):
            #     req.waitForReply(timeout)

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

        # setup bob SignalSlotable with slots for 0-4 arguments
        # that also reply 0-4 values
        self.called = -1
        self.bob = SignalSlotable(self.bob_id)
        self.bob.start()

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

            del req
            self.called = -1
        self.tearDownAliceBob()
