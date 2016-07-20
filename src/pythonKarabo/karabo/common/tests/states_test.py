import unittest

from karabo.common.states import State, StateSignifier


class States_TestCase(unittest.TestCase):
    def test_states_signifier(self):
        s = [State.DISABLED, State.COOLED, State.DECREASING]
        signifier = StateSignifier()
        self.assertIs(signifier.returnMostSignificant(s), State.DECREASING)
        s.append(State.RAMPING_UP)
        self.assertIs(signifier.returnMostSignificant(s), State.DECREASING)
        signifier = StateSignifier(staticMoreSignificant=State.ACTIVE,
                                   changingMoreSignificant=State.INCREASING)
        self.assertIs(signifier.returnMostSignificant(s), State.RAMPING_UP)
        s.append(State.INTERLOCKED)
        self.assertIs(signifier.returnMostSignificant(s), State.INTERLOCKED)
        s.append(State.UNKNOWN)
        self.assertIs(signifier.returnMostSignificant(s), State.UNKNOWN)

    def test_states_signifier_non_def_list(self):
        trumpList = [State.INTERLOCKED, State.UNKNOWN, State.KNOWN]
        s = [State.DISABLED, State.CHANGING, State.COOLED, State.DECREASING,
             State.UNKNOWN, State.INTERLOCKED]
        signifier = StateSignifier(trumplist=trumpList)
        self.assertIs(signifier.returnMostSignificant(s), State.CHANGING)

    def test_states_round_trip(self):
        s = State.ROTATING_CLK.name

        self.assertEqual(s, "ROTATING_CLK")
        s2 = State.fromString(s)
        self.assertIs(s2, State.ROTATING_CLK)

    def test_hierarchy(self):
        # direct parentage
        self.assertTrue(State.CHANGING.isDerivedFrom(State.NORMAL))
        # direct parentage the other way round
        self.assertFalse(State.NORMAL.isDerivedFrom(State.CHANGING))
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

    def test_circlefree(self):
        for s in State:
            for i in range(10):
                s = s.parent
                if s is None:
                    break
            else:
                self.fail("all states must reduce to None")


if __name__ == '__main__':
    unittest.main()
