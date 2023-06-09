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
import unittest

from karabo.common.api import State, StateSignifier


class States_TestCase(unittest.TestCase):
    def test_states_signifier_default(self):
        s = [State.DISABLED, State.ON, State.STOPPED]
        signifier = StateSignifier(
            staticMoreSignificant=State.PASSIVE,
            changingMoreSignificant=State.DECREASING,
        )
        self.assertIs(signifier.returnMostSignificant(s), State.STOPPED)
        s.append(State.RUNNING)
        self.assertIs(signifier.returnMostSignificant(s), State.RUNNING)
        s.append(State.PAUSED)
        self.assertIs(signifier.returnMostSignificant(s), State.PAUSED)
        s.append(State.HEATING)
        self.assertIs(signifier.returnMostSignificant(s), State.HEATING)
        s.append(State.INCREASING)
        self.assertIs(signifier.returnMostSignificant(s), State.INCREASING)
        s.append(State.COOLING)
        self.assertIs(signifier.returnMostSignificant(s), State.COOLING)
        s.append(State.DECREASING)
        self.assertIs(signifier.returnMostSignificant(s), State.DECREASING)
        s.append(State.MOVING)
        self.assertIs(signifier.returnMostSignificant(s), State.DECREASING)
        s.append(State.CHANGING)
        self.assertIs(signifier.returnMostSignificant(s), State.CHANGING)
        s.append(State.INTERLOCKED)
        self.assertIs(signifier.returnMostSignificant(s), State.INTERLOCKED)
        s.append(State.ERROR)
        self.assertIs(signifier.returnMostSignificant(s), State.ERROR)
        s.append(State.INIT)
        self.assertIs(signifier.returnMostSignificant(s), State.INIT)
        s.append(State.UNKNOWN)
        self.assertIs(signifier.returnMostSignificant(s), State.UNKNOWN)

    def test_states_signifier_active_decreasing(self):
        s = [State.DISABLED, State.ON, State.STOPPED]
        signifier = StateSignifier(
            staticMoreSignificant=State.ACTIVE,
            changingMoreSignificant=State.DECREASING,
        )
        self.assertIs(signifier.returnMostSignificant(s), State.ON)
        s.append(State.RUNNING)
        self.assertIs(signifier.returnMostSignificant(s), State.RUNNING)
        s.append(State.PAUSED)
        self.assertIs(signifier.returnMostSignificant(s), State.PAUSED)
        s.append(State.HEATING)
        self.assertIs(signifier.returnMostSignificant(s), State.HEATING)
        s.append(State.INCREASING)
        self.assertIs(signifier.returnMostSignificant(s), State.INCREASING)
        s.append(State.COOLING)
        self.assertIs(signifier.returnMostSignificant(s), State.COOLING)
        s.append(State.DECREASING)
        self.assertIs(signifier.returnMostSignificant(s), State.DECREASING)
        s.append(State.MOVING)
        self.assertIs(signifier.returnMostSignificant(s), State.DECREASING)
        s.append(State.CHANGING)
        self.assertIs(signifier.returnMostSignificant(s), State.CHANGING)
        s.append(State.INTERLOCKED)
        self.assertIs(signifier.returnMostSignificant(s), State.INTERLOCKED)
        s.append(State.ERROR)
        self.assertIs(signifier.returnMostSignificant(s), State.ERROR)
        s.append(State.INIT)
        self.assertIs(signifier.returnMostSignificant(s), State.INIT)
        s.append(State.UNKNOWN)
        self.assertIs(signifier.returnMostSignificant(s), State.UNKNOWN)

    def test_states_signifier_passive_increasing(self):
        s = [State.DISABLED, State.ON, State.STOPPED]
        signifier = StateSignifier(
            staticMoreSignificant=State.PASSIVE,
            changingMoreSignificant=State.INCREASING,
        )
        self.assertIs(signifier.returnMostSignificant(s), State.STOPPED)
        s.append(State.RUNNING)
        self.assertIs(signifier.returnMostSignificant(s), State.RUNNING)
        s.append(State.PAUSED)
        self.assertIs(signifier.returnMostSignificant(s), State.PAUSED)
        s.append(State.COOLING)  # decrease
        self.assertIs(signifier.returnMostSignificant(s), State.COOLING)
        s.append(State.DECREASING)  # decrease parent
        self.assertIs(signifier.returnMostSignificant(s), State.DECREASING)
        s.append(State.HEATING)  # increase
        self.assertIs(signifier.returnMostSignificant(s), State.HEATING)
        s.append(State.INCREASING)  # increase parent
        self.assertIs(signifier.returnMostSignificant(s), State.INCREASING)
        s.append(State.MOVING)  # any other changing state
        self.assertIs(signifier.returnMostSignificant(s), State.INCREASING)
        s.append(State.CHANGING)
        self.assertIs(signifier.returnMostSignificant(s), State.INCREASING)
        s.append(State.INTERLOCKED)
        self.assertIs(signifier.returnMostSignificant(s), State.INTERLOCKED)
        s.append(State.ERROR)
        self.assertIs(signifier.returnMostSignificant(s), State.ERROR)
        s.append(State.INIT)
        self.assertIs(signifier.returnMostSignificant(s), State.INIT)
        s.append(State.UNKNOWN)
        self.assertIs(signifier.returnMostSignificant(s), State.UNKNOWN)

    def test_states_signifier_active_increasing(self):
        s = [State.DISABLED, State.ON, State.STOPPED]
        signifier = StateSignifier(
            staticMoreSignificant=State.ACTIVE,
            changingMoreSignificant=State.INCREASING,
        )
        self.assertIs(signifier.returnMostSignificant(s), State.ON)
        s.append(State.RUNNING)
        self.assertIs(signifier.returnMostSignificant(s), State.RUNNING)
        s.append(State.PAUSED)
        self.assertIs(signifier.returnMostSignificant(s), State.PAUSED)
        s.append(State.COOLING)  # decrease
        self.assertIs(signifier.returnMostSignificant(s), State.COOLING)
        s.append(State.DECREASING)  # decrease parent
        self.assertIs(signifier.returnMostSignificant(s), State.DECREASING)
        s.append(State.HEATING)  # increase
        self.assertIs(signifier.returnMostSignificant(s), State.HEATING)
        s.append(State.INCREASING)  # increase parent
        self.assertIs(signifier.returnMostSignificant(s), State.INCREASING)
        s.append(State.MOVING)
        self.assertIs(signifier.returnMostSignificant(s), State.INCREASING)
        s.append(State.CHANGING)
        self.assertIs(signifier.returnMostSignificant(s), State.INCREASING)
        s.append(State.INTERLOCKED)
        self.assertIs(signifier.returnMostSignificant(s), State.INTERLOCKED)
        s.append(State.ERROR)
        self.assertIs(signifier.returnMostSignificant(s), State.ERROR)
        s.append(State.INIT)
        self.assertIs(signifier.returnMostSignificant(s), State.INIT)
        s.append(State.UNKNOWN)
        self.assertIs(signifier.returnMostSignificant(s), State.UNKNOWN)

    def test_acquiring_changing_on_passive(self):
        s = [State.ON, State.OFF]
        signifier = StateSignifier(
            staticMoreSignificant=State.PASSIVE,
            changingMoreSignificant=State.DECREASING,
        )
        self.assertIs(signifier.returnMostSignificant(s), State.OFF)
        s.append(State.ACQUIRING)
        self.assertIs(signifier.returnMostSignificant(s), State.ACQUIRING)
        s.append(State.CHANGING)
        self.assertIs(signifier.returnMostSignificant(s), State.CHANGING)

    def test_acquiring_changing_on_active(self):
        s = [State.ON, State.OFF]
        signifier = StateSignifier(
            staticMoreSignificant=State.ACTIVE,
            changingMoreSignificant=State.DECREASING,
        )
        self.assertIs(signifier.returnMostSignificant(s), State.ON)
        s.append(State.ACQUIRING)
        self.assertIs(signifier.returnMostSignificant(s), State.ACQUIRING)
        s.append(State.CHANGING)
        self.assertIs(signifier.returnMostSignificant(s), State.CHANGING)

    def test_states_signifier_non_def_list(self):
        trumpList = [State.INTERLOCKED, State.UNKNOWN, State.KNOWN]
        s = [
            State.DISABLED,
            State.CHANGING,
            State.ON,
            State.RUNNING,
            State.PAUSED,
            State.UNKNOWN,
            State.INTERLOCKED,
        ]
        signifier = StateSignifier(trumplist=trumpList)
        self.assertIs(signifier.returnMostSignificant(s), State.CHANGING)

    def test_states_round_trip(self):
        s = State.ROTATING_CLK.name

        self.assertEqual(s, "ROTATING_CLK")
        s2 = State.fromString(s)
        self.assertIs(s2, State.ROTATING_CLK)

    def test_hierarchy(self):
        # direct parentage
        self.assertTrue(State.RUNNING.isDerivedFrom(State.NORMAL))
        self.assertTrue(State.CHANGING.isDerivedFrom(State.NORMAL))
        self.assertTrue(State.PAUSED.isDerivedFrom(State.DISABLED))
        self.assertTrue(State.INCREASING.isDerivedFrom(State.CHANGING))
        self.assertTrue(State.DECREASING.isDerivedFrom(State.CHANGING))
        # direct parentage the other way round
        self.assertFalse(State.NORMAL.isDerivedFrom(State.CHANGING))
        self.assertFalse(State.NORMAL.isDerivedFrom(State.RUNNING))
        # no parentage
        self.assertFalse(State.CHANGING.isDerivedFrom(State.ERROR))
        # the other way round
        self.assertFalse(State.ERROR.isDerivedFrom(State.CHANGING))
        # longer list of ancestors
        self.assertTrue(State.HEATED.isDerivedFrom(State.NORMAL))
        # longer list of ancestors the other way round
        self.assertFalse(State.KNOWN.isDerivedFrom(State.INCREASING))

    def test_parent(self):
        for s in State:
            if s.parent is None:
                continue
            self.assertIsInstance(s.parent, State)
            self.assertEqual(s.value, s.name)

    def test_example(self):
        self.assertIs(State.OFF.parent, State.PASSIVE)
        self.assertNotEqual(State.OFF, State.OPENED)
        self.assertIs(State.INIT.parent, None)
        self.assertIs(State.STOPPED.parent, State.PASSIVE)
        self.assertIs(State.PAUSED.parent, State.DISABLED)

    def test_circlefree(self):
        for s in State:
            for i in range(10):
                s = s.parent
                if s is None:
                    break
            else:
                self.fail("all states must reduce to None")


if __name__ == "__main__":
    unittest.main()
