from unittest import TestCase, main

import numpy as np

from karabo.common.states import State
from karabo.native import (
    Bool, Configurable, Float, Int32, MetricPrefix, String, Timestamp, Unit,
    unit_registry as unit, QuantityValue, StringValue, BoolValue,
    VectorDouble)

from ..unitutil import minimum, maximum, removeQuantity, StateSignifier
from ..utils import build_karabo_value


class Tests(TestCase):
    def setUp(self):
        self.timestamp_1 = 1
        self.timestamp_2 = 2
        self.timestamp_3 = 3
        self.timestamp_4 = 4

    def test_signifier(self):
        a1 = String(enum=State)
        a2 = String(enum=State)
        a3 = String(enum=State)
        a4 = String(enum=State)

        v1 = a1.toKaraboValue(State.MOVING)
        v1.timestamp = self.timestamp_1
        self.assertEqual(v1, State.MOVING)
        v2 = a2.toKaraboValue(State.ON)
        v2.timestamp = self.timestamp_2
        self.assertEqual(v2, State.ON)
        v3 = a3.toKaraboValue(State.ON)
        v3.timestamp = self.timestamp_3
        self.assertEqual(v3, State.ON)
        v4 = a4.toKaraboValue(State.OFF)
        v4.timestamp = self.timestamp_4
        self.assertEqual(v4, State.OFF)

        signifier = StateSignifier()
        state = signifier.returnMostSignificant([v1, v2, v3, v4])
        self.assertEqual(state, State.MOVING)
        self.assertEqual(state.timestamp, v4.timestamp)

        v4 = a4.toKaraboValue(State.INIT)
        v4.timestamp = self.timestamp_4
        self.assertEqual(v4, State.INIT)

        state = signifier.returnMostSignificant([v1, v2, v3, v4])
        self.assertEqual(state, State.INIT)
        self.assertEqual(state.timestamp, v4.timestamp)
        self.assertEqual(v1.timestamp, self.timestamp_1)
        self.assertEqual(v2.timestamp, self.timestamp_2)
        self.assertEqual(v3.timestamp, self.timestamp_3)
        self.assertEqual(v4.timestamp, self.timestamp_4)

        v1 = a1.toKaraboValue(State.ON)
        self.assertEqual(v1, State.ON)
        state = signifier.returnMostSignificant([v1, v2, v3, v4])
        self.assertEqual(state, State.INIT)
        self.assertEqual(state.timestamp, v4.timestamp)
        self.assertEqual(state.timestamp, 4)
        self.assertNotEqual(state.timestamp, v1.timestamp)
        self.assertNotEqual(state.timestamp, v2.timestamp)
        self.assertNotEqual(state.timestamp, v3.timestamp)
        self.assertEqual(v1.timestamp, None)
        self.assertEqual(v2.timestamp, self.timestamp_2)
        self.assertEqual(v3.timestamp, self.timestamp_3)
        self.assertEqual(v4.timestamp, self.timestamp_4)

        # do without timestamp
        v1 = State.ON
        v2 = State.MOVING
        v3 = State.ON
        v4 = State.OFF
        state = signifier.returnMostSignificant([v1, v2, v3, v4])
        self.assertEqual(state, State.MOVING)
        self.assertIsNone(state.timestamp)

        # timestamp provided for the most significant state
        v1 = a1.toKaraboValue(State.ERROR)
        v1.timestamp = 20
        state = signifier.returnMostSignificant([v1, v2, v3, v4])
        self.assertEqual(state, State.ERROR)
        self.assertEqual(state.timestamp, v1.timestamp)
        self.assertEqual(state.timestamp, 20)
        with self.assertRaises(AttributeError):
            self.assertNotEqual(state.timestamp, v2.timestamp)
            self.assertNotEqual(state.timestamp, v3.timestamp)
            self.assertNotEqual(state.timestamp, v4.timestamp)

        v1 = a1.toKaraboValue(State.OFF)
        v1.timestamp = self.timestamp_1
        state = signifier.returnMostSignificant([v1, v2, v3, v4])
        self.assertEqual(state, State.MOVING)
        self.assertEqual(state.timestamp, v1.timestamp)
        self.assertEqual(state.timestamp, 1)

    def test_get_maximum(self):
        a1 = Float(defaultValue=1.0)
        a2 = Float(defaultValue=1.0)
        a3 = Float(defaultValue=1.0)
        a4 = Float(defaultValue=1.0)

        v1 = a1.toKaraboValue(6.0)
        v1.timestamp = self.timestamp_1
        self.assertEqual(v1, 6.0)
        v2 = a2.toKaraboValue(1.25)
        v2.timestamp = self.timestamp_2
        self.assertEqual(v2, 1.25)
        v3 = a3.toKaraboValue(1.75)
        v3.timestamp = self.timestamp_3
        self.assertEqual(v3, 1.75)
        v4 = a4.toKaraboValue(4.0)
        v4.timestamp = self.timestamp_4
        self.assertEqual(v4, 4.0)
        value = maximum([v1, v2, v3, v4])
        self.assertEqual(value, 6.0)
        self.assertEqual(value.timestamp, v4.timestamp)
        self.assertEqual(value.timestamp, 4)
        self.assertEqual(v1.timestamp, self.timestamp_1)
        self.assertEqual(v2.timestamp, self.timestamp_2)
        self.assertEqual(v3.timestamp, self.timestamp_3)
        self.assertEqual(v4.timestamp, self.timestamp_4)

        with self.assertRaises(TypeError):
            value = maximum(v1)

    def test_get_mimimum(self):
        a1 = Float(defaultValue=1.0)
        a2 = Float(defaultValue=1.0)
        a3 = Float(defaultValue=1.0)
        a4 = Float(defaultValue=1.0)

        v1 = a1.toKaraboValue(6.0)
        v1.timestamp = self.timestamp_1
        self.assertEqual(v1, 6.0)
        v2 = a2.toKaraboValue(1.25)
        v2.timestamp = self.timestamp_2
        self.assertEqual(v2, 1.25)
        v3 = a3.toKaraboValue(1.75)
        v3.timestamp = self.timestamp_3
        self.assertEqual(v3, 1.75)
        v4 = a4.toKaraboValue(4.0)
        v4.timestamp = self.timestamp_4
        self.assertEqual(v4, 4.0)

        value = minimum([v1, v2, v3, v4])
        self.assertEqual(value, 1.25)
        self.assertEqual(value.timestamp, v4.timestamp)
        self.assertEqual(value.timestamp, 4)
        self.assertEqual(v1.timestamp, self.timestamp_1)
        self.assertEqual(v2.timestamp, self.timestamp_2)
        self.assertEqual(v3.timestamp, self.timestamp_3)
        self.assertEqual(v4.timestamp, self.timestamp_4)

        with self.assertRaises(TypeError):
            value = minimum(v1)

    def test_no_quantity(self):
        @removeQuantity
        def calculate(a, b, c=5, d=10):
            return a + b + c - d

        a = Float().toKaraboValue(6.0)
        b = Float().toKaraboValue(2.0)
        c = Float().toKaraboValue(1.0)
        d = Float().toKaraboValue(1.0)

        summation = a + b + c - d
        self.assertIsInstance(summation, QuantityValue)
        self.assertEqual(summation, 8.0)
        new_summation = calculate(a, b=b, c=c)
        self.assertNotIsInstance(new_summation, QuantityValue)
        self.assertEqual(new_summation, -1)
        new_summation = calculate(a, b=b, d=c)
        self.assertNotIsInstance(new_summation, QuantityValue)
        self.assertEqual(new_summation, 12)

    def test_build_karabo_value(self):
        class A(Configurable):
            boolProperty = Bool(
                defaultValue=False)

            floatProperty = Float(
                defaultValue=2.0,
                unitSymbol=Unit.METER,
                metricPrefixSymbol=MetricPrefix.MILLI)

            stringProperty = String(
                unitSymbol=Unit.AMPERE,
                defaultValue="XFEL")

            integerProperty = Int32(
                defaultValue=-1.0,
                unitSymbol=Unit.SECOND)

            vectorProperty = VectorDouble(
                defaultValue=[2.0, 3.2],
                unitSymbol=Unit.METER,
                metricPrefixSymbol=MetricPrefix.MILLI)

        a = A()
        ts = Timestamp()
        value = build_karabo_value(a, 'floatProperty', 7.6, ts)
        self.assertEqual(type(value), QuantityValue)
        # Values are casted properly
        self.assertEqual(value.value, np.float32(7.6))
        self.assertEqual(value.value.dtype, np.float32)
        self.assertEqual(value.timestamp, ts)
        self.assertEqual(value.units, unit.millimeter)

        value = build_karabo_value(a, 'stringProperty', "New", ts)
        self.assertEqual(type(value), StringValue)
        self.assertEqual(value.value, "New")
        self.assertEqual(value.timestamp, ts)

        value = build_karabo_value(a, 'boolProperty', True, ts)
        self.assertEqual(type(value), BoolValue)
        self.assertEqual(value.value, True)
        self.assertEqual(value.timestamp, ts)

        vector = np.array([4.5, 3.3], dtype=np.float64)
        value = build_karabo_value(a, 'vectorProperty', vector, ts)
        self.assertEqual(type(value), QuantityValue)
        np.testing.assert_array_equal(value.value, vector)
        self.assertEqual(value.timestamp, ts)
        self.assertEqual(value.units, unit.millimeter)

        # Check wrong dtype of vector value
        vector = np.array([4.5, 3.3], dtype=np.int32)
        value = build_karabo_value(a, 'vectorProperty', vector, ts)
        self.assertEqual(type(value), QuantityValue)
        np.testing.assert_array_equal(value.value, vector)
        self.assertEqual(value.value.dtype, np.float64)
        self.assertEqual(value.timestamp, ts)
        self.assertEqual(value.units, unit.millimeter)

        value = build_karabo_value(a, 'integerProperty', 12, ts)
        self.assertEqual(type(value), QuantityValue)
        self.assertEqual(value.value, 12)
        self.assertEqual(value.timestamp, ts)
        self.assertEqual(value.units, unit.second)


if __name__ == "__main__":
    main()
