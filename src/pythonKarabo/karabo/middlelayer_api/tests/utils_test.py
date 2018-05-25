from unittest import TestCase, main

from karabo.middlelayer import (
    Float, maximum, mean, minimum, String, State, StateSignifier)


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

        # do without timestamp
        v1 = State.ON
        v2 = State.MOVING
        v3 = State.ON
        v4 = State.OFF
        state = signifier.returnMostSignificant([v1, v2, v3, v4])
        self.assertEqual(state, State.MOVING)
        # no timestamp provided, hence there is no timestamp attached.
        with self.assertRaises(AttributeError):
            print(state.timestamp)

        # timestamp provided for the most significant state, we have timestamp
        v1 = a1.toKaraboValue(State.ERROR)
        state = signifier.returnMostSignificant([v1, v2, v3, v4])
        self.assertEqual(state, State.ERROR)
        self.assertEqual(state.timestamp, v1.timestamp)

        v1 = a1.toKaraboValue(State.OFF)
        state = signifier.returnMostSignificant([v1, v2, v3, v4])
        self.assertEqual(state, State.MOVING)
        # timestamp provided for a less significant, we have no timestamp
        with self.assertRaises(AttributeError):
            print(state.timestamp)

    def test_get_maximum(self):
        a1 = Float(defaultValue=1.0)
        a2 = Float(defaultValue=1.0)
        a3 = Float(defaultValue=1.0)
        a4 = Float(defaultValue=1.0)

        v1 = a1.toKaraboValue(6.0)
        self.assertEqual(v1, 6.0)
        v2 = a2.toKaraboValue(1.3)
        self.assertEqual(v2, 1.3)
        v3 = a3.toKaraboValue(1.7)
        self.assertEqual(v3, 1.7)
        v4 = a4.toKaraboValue(4.0)
        self.assertEqual(v4, 4.0)

        value = maximum([v1, v2, v3, v4])
        self.assertEqual(value, 6.0)
        self.assertEqual(value.timestamp, v4.timestamp)

        with self.assertRaises(TypeError):
            value = maximum(v1)

    def test_get_mimimum(self):
        a1 = Float(defaultValue=1.0)
        a2 = Float(defaultValue=1.0)
        a3 = Float(defaultValue=1.0)
        a4 = Float(defaultValue=1.0)

        v1 = a1.toKaraboValue(6.0)
        self.assertEqual(v1, 6.0)
        v2 = a2.toKaraboValue(1.3)
        self.assertEqual(v2, 1.3)
        v3 = a3.toKaraboValue(1.7)
        self.assertEqual(v3, 1.7)
        v4 = a4.toKaraboValue(4.0)
        self.assertEqual(v4, 4.0)

        value = minimum([v1, v2, v3, v4])
        self.assertEqual(value, 1.3)
        self.assertEqual(value.timestamp, v4.timestamp)

        with self.assertRaises(TypeError):
            value = minimum(v1)

    def test_get_mean(self):
        a1 = Float(defaultValue=1.0)
        a2 = Float(defaultValue=1.0)
        a3 = Float(defaultValue=1.0)
        a4 = Float(defaultValue=1.0)

        v1 = a1.toKaraboValue(6.0)
        self.assertEqual(v1, 6.0)
        v2 = a2.toKaraboValue(1.3)
        self.assertEqual(v2, 1.3)
        v3 = a3.toKaraboValue(1.7)
        self.assertEqual(v3, 1.7)
        v4 = a4.toKaraboValue(4.0)
        self.assertEqual(v4, 4.0)

        value = mean([v1, v2, v3, v4])
        self.assertEqual(value, 3.25)
        self.assertEqual(value.timestamp, v4.timestamp)

        with self.assertRaises(TypeError):
            value = mean(v1)


if __name__ == "__main__":
    main()
