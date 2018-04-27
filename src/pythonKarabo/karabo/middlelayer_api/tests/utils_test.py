from unittest import TestCase, main

from karabo.middlelayer import (
    String, State, StateSignifier)


class Tests(TestCase):
    def setUp(self):
        pass

    def test_signifier(self):

        a1 = String(enum=State)
        a2 = String(enum=State)
        a3 = String(enum=State)
        a4 = String(enum=State)

        v1 = a1.toKaraboValue(State.MOVING)
        self.assertEqual(v1, State.MOVING)
        v2 = a2.toKaraboValue(State.ON)
        self.assertEqual(v2, State.ON)
        v3 = a3.toKaraboValue(State.ON)
        self.assertEqual(v3, State.ON)
        v4 = a4.toKaraboValue(State.OFF)
        self.assertEqual(v4, State.OFF)

        signifier = StateSignifier()
        state = signifier.returnMostSignificant([v1, v2, v3, v4])
        self.assertEqual(state, State.MOVING)
        self.assertEqual(state.timestamp, v4.timestamp)

        v4 = a4.toKaraboValue(State.INIT)
        self.assertEqual(v4, State.INIT)

        state = signifier.returnMostSignificant([v1, v2, v3, v4])
        self.assertEqual(state, State.INIT)
        self.assertEqual(state.timestamp, v4.timestamp)

        v1 = a1.toKaraboValue(State.ON)
        self.assertEqual(v1, State.ON)
        state = signifier.returnMostSignificant([v1, v2, v3, v4])
        self.assertEqual(state, State.INIT)
        self.assertEqual(state.timestamp, v1.timestamp)


if __name__ == "__main__":
    main()
