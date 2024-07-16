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
from threading import Condition, Lock, Thread
from unittest import TestCase, main

from karabind import EventLoop, SignalSlotable


class Tests(TestCase):
    senderId = "sigSlotSender"
    receiverId = "sigSlotReceiver"

    received_value = None
    event_loop = None
    sigSender = None
    sigReceiver = None

    @classmethod
    def setUpClass(cls):
        # Start the EventLoop so that DeviceClient works properly
        cls.event_loop = Thread(target=EventLoop.work)
        cls.event_loop.daemon = True
        cls.event_loop.start()

        # Set up slots.
        cls.sigSender = SignalSlotable(cls.senderId)
        cls.sigSender.start()
        cls.sigReceiver = SignalSlotable(cls.receiverId)
        cls.sigReceiver.start()

    @classmethod
    def tearDownClass(cls):
        del cls.sigSender, cls.sigReceiver
        EventLoop.stop()
        cls.event_loop.join()

    def _return_value(self, val):
        """Return the received value (through _receive_value variable)."""

        with self.check_value_condition:
            self.received_value = val
            self.check_value_condition.notify()

    def call_and_compare(self, sent_value, timeout=3):
        """Call the _return_value function and compare the
        ``returned value`` to ``send value``.

        :param sent_value: a value to be sent
        :param timeout: How much time to wait for the callback to be called.
        """
        # Wait for the callback to be called.
        with self.check_value_condition:
            # Reset received_value.
            self.received_value = None
            self.sigReceiver.call(Tests.receiverId, "_return_value",
                                  sent_value)
            self.assertTrue(self.check_value_condition.wait(timeout),
                            "The callback was not called in time.")

            self.assertEqual(sent_value, self.received_value,
                             "The received value was not expected.")

    def test_type_handle(self):
        """Test if types are handled properly when when a socket is called."""

        # Register _return_value slot.

        self.received_value = None
        self.check_value_condition = Condition(Lock())
        self.sigReceiver.registerSlot(self._return_value)

        # Test how boolean values are handled.
        self.call_and_compare(True)
        self.call_and_compare(False)

        # Test how integers are handled.
        self.call_and_compare(2147483648)
        self.call_and_compare(4294967295)
        self.call_and_compare(0x89213A892)

        # Test how floating points are handled.
        self.call_and_compare(1.7976931348623157e+180)
        self.call_and_compare(float('inf'))

        # Test how strings are handled.
        self.call_and_compare(
            "This are not the droids that you are looking for.")
        # Test if null character is handled properly.
        self.call_and_compare(
            "This are not the droids that you are " + '\0' + "looking for.")
        # Test if a subset of Cyrillic characters is handled properly.
        self.call_and_compare(
            "М Н О П Р С Т У Ф Х Ц Ч Ш Щ Ъ Ы Ь Э Ю Я а б в г д")
        # Test if a subset of Latin-1 Supplement characters is
        # handled properly.
        self.call_and_compare("Ô Õ Ö × Ø Ù Ú Û Ü Ý Þ ß à á â ã ä å æ ç")

        # Test how a random user defined types are handled.
        class SimpleType:
            pass

        with self.assertRaises(TypeError):
            self.call_and_compare(SimpleType())
            # XXX: Additional tests need to be added (e.g. test Hash, ...).


if __name__ == "__main__":
    main()
