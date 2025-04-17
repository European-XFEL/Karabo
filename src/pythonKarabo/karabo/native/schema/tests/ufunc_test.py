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
from platform import system
from unittest import TestCase, main, skip

import numpy as np
from numpy import e, pi
from pint import DimensionalityError, __version__ as pint_version

from karabo.native.data import MetricPrefix, Timestamp, Unit
from karabo.native.schema import (
    Int32, QuantityValue as QV, VectorDouble, VectorInt32)
from packaging import version


def _is_numpy_old():
    """In older version of numpy the floor division (floor_divide, remainder,
    fmod) by zero raises no warning/error"""

    return np.__version__ <= "1.19.5"


class Tests(TestCase):
    def setUp(self):
        self.N = 4
        self.mm_10 = QV(1, unit='mm', timestamp=10)
        self.m_100 = QV(1, unit='m', timestamp=100)

        # ts1 < ts2 <ts3
        self.ts1 = Timestamp("2009-04-20T10:32:22 UTC")
        self.ts2 = Timestamp("2011-04-20T10:32:22 UTC")
        self.ts3 = Timestamp("2019-04-20T10:32:22 UTC")

        self.zero = QV([0, 0, 0], timestamp=self.ts1)
        self.one = QV([1, 1, 1], timestamp=self.ts1)
        self.false = QV([False] * 3, timestamp=self.ts1)
        self.true = QV([True] * 3, timestamp=self.ts1)
        self.one_noTS = QV([1, 1, 1])
        self.nan_vec = QV([np.nan, np.nan, np.nan], unit='m',
                          timestamp=self.ts1)

        self.v0 = QV([0, 0, 0], unit='m', timestamp=self.ts1)
        self.v1 = QV([1, 1, 1], unit='m', timestamp=self.ts2)
        self.v2 = QV([0, 2, 4], unit='m', timestamp=self.ts3)
        self.v1_noTS = QV([1, 1, 1], unit='m')
        self.v1_mm = QV([1, 1, 1], unit='mm', timestamp=self.ts2)

        self.f1 = QV([1., 2., 3.], unit='s', timestamp=self.ts1)
        self.s1 = QV([1, 1, 1], unit='s', timestamp=self.ts1)

        # trigonometric values
        self.zero_rad = QV([0, 0, 0], unit='rad', timestamp=self.ts1)
        pi2 = pi / 2
        self.half_pi = QV([pi2, pi2, pi2], unit='rad', timestamp=self.ts1)

        # a set for functions which return `numpy.array` instead of Karabo QV
        self.np_array_func = set()

    def test_numpy_add(self):
        a = VectorInt32(minSize=2, maxSize=3)
        b = VectorInt32(minSize=2, maxSize=3)
        v1 = a.toKaraboValue([2, 3, 4])
        v2 = b.toKaraboValue([5, 7, 9])
        ret = np.add(v1, v2)

        np.testing.assert_array_equal(ret, np.array(ret))
        self.assertIsNone(ret.timestamp)

        ts = Timestamp("2009-04-20T10:32:22 UTC")
        v2.timestamp = ts
        ret = np.add(v1, v2)
        self.assertEqual(ret.timestamp, ts)

        ts2 = Timestamp("2012-04-20T10:32:22 UTC")
        v1.timestamp = ts2
        ret = np.add(v1, v2)
        self.assertEqual(ret.timestamp, ts2)

        a = Int32()
        b = Int32()
        c = Int32()
        d = Int32()
        v1 = a.toKaraboValue(1)
        v2 = b.toKaraboValue(2)
        v3 = c.toKaraboValue(3)
        v4 = d.toKaraboValue(4)

        ret = np.add([v1, v2], [v3, v4])
        with self.assertRaises(AttributeError):
            self.assertIsNone(ret.timestamp)

    def assertArrayEqual(self, a1, a2, msg=None):
        """Asserts two arrays are equal (units measurements are respected)"""
        a1_base = a1.to_base_units()
        a2_base = a2.to_base_units()

        # Check unit measurements first
        if a1_base.units != a2_base.units:
            raise DimensionalityError(a1_base.units, a2_base.units)

        v1, v2 = a1_base.value, a2_base.value

        # if all values in the list are ints, we can just compare
        if np.issubdtype(v1.dtype, np.integer):
            np.testing.assert_array_equal(v1, v2, err_msg=msg)

        # if something is float, trying to compare approximately
        # NaN also falls here
        elif np.issubdtype(v1.dtype, np.inexact):
            for x1, x2 in zip(v1, v2):
                np.testing.assert_allclose(x1, x2, equal_nan=True, atol=1e-13)

        # something went wrong
        else:
            self.fail(msg=msg)

    UNITS = {
        'meter': (Unit.METER, MetricPrefix.NONE),
        'millimeter': (Unit.METER, MetricPrefix.MILLI),
        'second': (Unit.SECOND, MetricPrefix.NONE),
        'degree': (Unit.DEGREE, MetricPrefix.NONE),
        'radian': (Unit.RADIAN, MetricPrefix.NONE),
    }

    def descriptors_from_QV(self, qv):
        """
        Produces an object with the same content, but
        using descriptors
        :param qv: source QuantityValue object
        :return: vector with descriptors
        """
        value, u = qv.to_tuple()
        if u:
            units, power = u[0]
            assert units in self.UNITS.keys()
            unit_symbol, metric_prefix = self.UNITS[units]
            if isinstance(value[0], np.integer):
                v = VectorInt32(unitSymbol=unit_symbol,
                                metricPrefixSymbol=metric_prefix)
            else:
                v = VectorDouble(unitSymbol=unit_symbol,
                                 metricPrefixSymbol=metric_prefix)

        # no unit measurement symbol is provided
        else:
            power = None
            if isinstance(value[0], np.integer):
                v = VectorInt32(unitSymbol=Unit.NOT_ASSIGNED)
            else:
                v = VectorDouble(unitSymbol=Unit.NOT_ASSIGNED)
        vec = v.toKaraboValue(value)
        if power and power != 1:
            vec = vec * (vec.units ** (power - 1))
        ts = qv.timestamp
        vec.timestamp = ts
        return vec

    def _assert_binary_ufunc(self, ufunc, A, B, expected, msg, debug):
        C = ufunc(A, B)
        if debug:
            print(f'{ufunc.__name__}({A},{B}) = {expected}')

        if isinstance(C, QV):
            # the result is still Karabo's `QuantityValue`
            self.assertArrayEqual(C, expected, msg=msg)

            # Check that the result has the newest timestamp, but take
            # care that there might be a `None` timestamp
            if getattr(A, "timestamp", None) is None:
                newest_ts = B.timestamp
            elif getattr(B, "timestamp", None) is None:
                newest_ts = A.timestamp
            else:
                newest_ts = max(A.timestamp, B.timestamp)
            self.assertEqual(newest_ts, C.timestamp, msg=msg)

        elif isinstance(C, np.ndarray):
            # the result is `ndarray` and thus doesn't have units and timestamp
            self.np_array_func.update([ufunc.__name__])

            np.testing.assert_allclose(C, expected)

        else:
            raise TypeError(msg)

    def assertBinaryUfunc(self, ufunc, A, B, expected, msg='', debug=False):
        """
        Define:
            C = ufunc(A, B)

        Check whether:
            - C = expected, elementwise, units are respected
            - C.t = max(A.t, B.t)
        """

        self._assert_binary_ufunc(ufunc, A, B, expected, debug=debug,
                                  msg=f"in QuantityValue part of "
                                  f"'{ufunc.__name__}', {msg}")

        # Now the same for descriptor-based objects
        dA = self.descriptors_from_QV(A)
        if isinstance(B, QV):
            dB = self.descriptors_from_QV(B)
        else:
            dB = B
        self._assert_binary_ufunc(ufunc, dA, dB, expected, debug=debug,
                                  msg=f"in descriptor-based part of "
                                  f"'{ufunc.__name__}', {msg}")

    def _assert_unary_ufunc(self, ufunc, A, expected, msg, debug):
        """
        Define:
            C = ufunc(A)

        Check whether:
            - C = expected, elementwise, units are respected
            - C.t = expected.t
        """
        C = ufunc(A)
        if debug:
            print(f'{ufunc.__name__}({A}) = {expected}')

        if isinstance(C, QV):
            self.assertArrayEqual(C, expected, msg=msg)
            self.assertEqual(C.timestamp, expected.timestamp, msg=msg)
        elif isinstance(C, np.ndarray):
            self.np_array_func.update([ufunc.__name__])
            np.testing.assert_allclose(C, expected)
        else:
            raise TypeError(msg)

    def assertUnaryUfunc(self, ufunc, A, expected, msg='', debug=False):
        """
        Define:
            C = ufunc(A)

        Check whether:
            - C = expected, element-wise, units are respected
            - C.t = expected.t
        """
        self._assert_unary_ufunc(ufunc, A, expected, debug=debug,
                                 msg=f"in QuantityValue part of "
                                 f"'{ufunc.__name__}({A})', {msg}")

        # Now the same for descriptor-based objects
        dA = self.descriptors_from_QV(A)
        self._assert_unary_ufunc(ufunc, dA, expected, debug=debug,
                                 msg=f"in descriptor-based part of "
                                 f"'{ufunc.__name__}', {msg}")

    """
    ** Math operations **
    """

    def test_add(self):
        ufunc = np.add

        # A + A = 2A
        self.assertBinaryUfunc(ufunc, self.v1, self.v1, 2 * self.v1)

        # A + 0 = A
        self.assertBinaryUfunc(ufunc, self.v1, self.v0, self.v1)

        # Compatible units
        self.assertBinaryUfunc(ufunc, self.v1_mm, self.v0, self.v1_mm)
        # Machine precision comes into play:
        self.assertBinaryUfunc(ufunc, self.v1, self.v1_mm, 1.001 * self.v1)
        self.assertBinaryUfunc(ufunc, self.v1, self.v1_mm, 1001 * self.v1_mm)

        # Incompatible units
        v4 = QV([1, 2, 3], unit='s', timestamp=self.ts3)
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, v4, self.v0, v4)

    def test_subtract(self):
        ufunc = np.subtract

        # A - 0 = A
        self.assertBinaryUfunc(ufunc, self.v1, self.v0, self.v1)
        # A - A = 0
        self.assertBinaryUfunc(ufunc, self.v2, self.v2, self.v0)

        # Compatible units
        v4 = QV([1, 1, 1], unit='mm', timestamp=self.ts3)
        self.assertBinaryUfunc(ufunc, v4, self.v0, v4)
        self.assertBinaryUfunc(ufunc, v4, self.v1_mm, self.v0)

        # Incompatible units
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, self.s1, self.v0, self.s1)

        # Timestamp is missing
        self.assertBinaryUfunc(ufunc, self.v1, self.v1_noTS, self.v0)

    def test_multiply(self):
        """
        Multiply arguments element-wise.
        Units are also multiplied.
        """
        ufunc = np.multiply

        v2_m2 = QV(self.v2.value, unit='m**2')
        self.assertBinaryUfunc(ufunc, self.v1, self.v2, v2_m2)
        one_dimless = QV(self.v1.value, timestamp=self.ts1)
        # A * 1 = A
        self.assertBinaryUfunc(ufunc, self.v2, one_dimless, self.v2)
        self.assertBinaryUfunc(ufunc, one_dimless, self.v2, self.v2)

        # Timestamp is missing
        self.assertBinaryUfunc(ufunc, self.v2, self.v1_noTS, v2_m2)

        # Compatible units
        v4 = QV([1, 1, 1], unit='mm', timestamp=self.ts3)
        self.assertBinaryUfunc(ufunc, v4, self.v1,  # 1m × 1mm = 1 m×mm
                               QV([1, 1, 1], unit='m * mm'))
        self.assertBinaryUfunc(ufunc, v4, self.v1,  # 1m × 1mm = 1 mm×m
                               QV([1, 1, 1], unit='mm * m'))
        self.assertBinaryUfunc(ufunc, v4, self.v1,  # 1m × 1mm = 0.001 m²
                               QV([0.001, 0.001, 0.001], unit='m ** 2'))

        # No incompatible units here:
        v5 = QV([1, 2, 3], unit='s', timestamp=self.ts3)
        self.assertBinaryUfunc(ufunc, v5, self.v0,
                               QV([0, 0, 0], unit='m * s', timestamp=self.ts1))
        # [1m, 1m, 1m] × [1s, 2s, 3s] = [1000mm, 2000mm, 3000s]
        self.assertBinaryUfunc(ufunc, self.v1, v5,
                               QV([1000, 2000, 3000], unit='mm * s',
                                  timestamp=self.ts1))

    def test_divide(self):
        """
        Returns a true division of the inputs, element-wise.
        """
        ufunc = np.divide

        # A [m] / 1 [m] = A [dimensionless]
        v2_nodim = QV(self.v2.value, timestamp=self.ts2)
        self.assertBinaryUfunc(ufunc, self.v2, self.v1, v2_nodim)

        # division by zero
        # 1 [m] / 0 [m] = inf [dimensionless]
        v4 = QV([np.inf, np.inf, np.inf], timestamp=self.ts1)
        with self.assertWarns(RuntimeWarning):
            self.assertBinaryUfunc(ufunc, self.v1, self.v0, v4)

        # [0, 2, 4] / [0, 0, 0 ] = [nan, inf, inf]
        v5 = QV([np.nan, np.inf, np.inf], timestamp=self.ts1)
        with self.assertWarns(RuntimeWarning):
            self.assertBinaryUfunc(ufunc, self.v2, self.v0, v5)

        # Compatible units:
        v6 = QV([0, 2000, 4000], timestamp=self.ts3)
        # [0m, 2m, 4m] / [1mm, 1mm, 1mm] = [0, 2000, 4000] dimensionless
        self.assertBinaryUfunc(ufunc, self.v2, self.v1_mm, v6)
        # No incompatible units here:
        self.assertBinaryUfunc(ufunc, self.v2, self.s1,
                               QV([0, 2, 4], unit='m / s', timestamp=self.ts1))

        # Timestamp is missing
        self.assertBinaryUfunc(ufunc, self.v2, self.v1_noTS, v2_nodim)

    def test_logaddexp(self):
        """Logarithm of the sum of exponentiations of the inputs.

        Calculates ``log(exp(x1) + exp(x2))``. This function is useful in
        statistics where the calculated probabilities of events may be so small
        as to exceed the range of normal floating point numbers.  In such cases
        the logarithm of the calculated probability is stored. This function
        allows adding probabilities stored in such a fashion.

        XXX: Returns numpy.array
        """
        ufunc = np.logaddexp

        ret = np.log(2 * np.exp(1))
        v_ret = QV(np.array([ret, ret, ret]), timestamp=self.one.timestamp)

        self.assertBinaryUfunc(ufunc, self.one, self.one, v_ret)

        # Only dimensionless values are allowed:
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, self.v1, self.v1, v_ret)

        # FIXME: Since this is a np.array, the returned value HAS NO timestamp
        self.assertBinaryUfunc(ufunc, self.one, self.one_noTS, v_ret)

    def test_logaddexp2(self):
        """Logarithm of the sum of exponentiations of the inputs in base-2.

        XXX: Returns numpy.array

        Calculates ``log2(2**x1 + 2**x2)``. This function is useful in machine
        learning when the calculated probabilities of events may be so small as
        to exceed the range of normal floating point numbers.  In such cases
        the base-2 logarithm of the calculated probability can be used instead.
        This function allows adding probabilities stored in such a fashion
        """
        ufunc = np.logaddexp2

        v_ret = QV(np.array([2, 2, 2]), timestamp=self.one.timestamp)

        self.assertBinaryUfunc(ufunc, self.one, self.one, v_ret)

        # Only dimensionless values are allowed:
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, self.v1, self.v1, v_ret)

        # FIXME: Since this is a np.array, the returned value HAS NO timestamp
        self.assertBinaryUfunc(ufunc, self.one, self.one_noTS, v_ret)

    def test_true_divide(self):
        """`numpy.true_divide` is the same as `numpy.divide`, already tested"""
        assert np.true_divide is np.divide
        self.test_divide()

    def test_floor_divide(self):
        """
        Return the largest integer smaller or equal to the division of the
        inputs. It is equivalent to the Python ``//`` operator and pairs with
        the Python ``%`` (`remainder`), function so that
        ``b = a % b + b * (a // b)`` up to roundoff.
        """
        ufunc = np.floor_divide

        # A [m] // 1 [m] = A [dimensionless]
        v2_nodim = QV(self.v2.value, timestamp=self.ts2)
        self.assertBinaryUfunc(ufunc, self.v2, self.v1, v2_nodim)

        # 3 [m] // 2 [m] = 1 [dimensionless]
        two, three = 2 * self.v1, 3 * self.v1
        self.assertBinaryUfunc(ufunc, three, two, self.one)

        # FIXME: floor division  by zero behaves differently for `int` and
        #  `float` arguments: int returns ZERO for and an `int` first argument
        #  but NAN for the first argument of type `float`
        # 1 [m] // 0 [m] = 0 [dimensionless]
        # first argument is of type `int`
        with self.assertWarns(RuntimeWarning):
            self.assertBinaryUfunc(ufunc, self.v1, self.v0, self.zero)

        # first argument is of type `float`
        one_float_vec = QV([1., 1., 1.], unit='m', timestamp=self.ts1)

        if _is_numpy_old():
            self.assertBinaryUfunc(ufunc, one_float_vec, self.zero,
                                   self.nan_vec)
        else:
            with self.assertRaises(AssertionError):
                self.assertBinaryUfunc(ufunc, one_float_vec, self.zero,
                                       self.nan_vec)

        # [0, 2, 4] // [0, 0, 0 ] = [0, 0, 0]
        if np.__version__ == "1.19.5":
            with self.assertWarns(RuntimeWarning):
                self.assertBinaryUfunc(ufunc, self.v2, self.v0, self.zero)
        else:
            self.assertBinaryUfunc(ufunc, self.v2, self.v0, self.zero)

        # Compatible units:
        # A [m] // 1 [mm] = 1000 * A [dimensionless]
        v6 = QV([0, 2000, 4000], timestamp=self.ts3)
        self.assertBinaryUfunc(ufunc, self.v2, self.v1_mm, v6)
        # No incompatible units here:
        # A [m] // 1 [s] = A [m/s]
        self.assertBinaryUfunc(ufunc, self.v2, self.s1,
                               QV([0, 2, 4], unit='m / s', timestamp=self.ts1))

        # Timestamp is missing
        self.assertBinaryUfunc(ufunc, self.v2, self.v1_noTS, v2_nodim)

    def test_negative(self):
        # - (1 [m]) = (-1 [m])
        self.assertUnaryUfunc(np.negative, self.v1, -1 * self.v1)
        # - (0 [m]) = 0 [m]
        self.assertUnaryUfunc(np.negative, self.v0, -1 * self.v0)

    def test_positive(self):
        """
        Numerical positive, element-wise.
        """
        # this was introduced only in numpy 1.13.0
        if version.parse(pint_version) >= version.parse("0.21.0"):
            self.assertUnaryUfunc(np.positive, self.v1, self.v1)
        # else should throw a not implemented error
        else:
            with self.assertRaises(NotImplementedError):
                self.assertUnaryUfunc(np.positive, self.v1, self.v1)

    def test_power(self):
        """
        First array elements raised to powers from second array, element-wise.

        Raise each base in `a` to the positionally-corresponding power in `b`.
        `a` and `b` must be broadcastable to the same shape. Note that an
        integer type raised to a negative integer power will raise a ValueError
        """
        ufunc = np.power
        # [0, 2, 4] * [2] = [0, 4, 16]
        # units change according to the power
        r = QV(np.array([0, 4, 16]), "m^2", timestamp=self.v2.timestamp)
        self.assertBinaryUfunc(ufunc, self.v2, 2, r)

        # [0, 2, 4] * [2, 3] => error
        # Exponent has no unit
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, self.v2, np.array([2, 3]),
                                   np.array([0, 4, 16]))
        with self.assertRaises(ValueError):
            self.assertBinaryUfunc(ufunc, self.one, -10, np.array([]))

        # TODO:
        # [[1, 2, 3], [1, 1, 1]] * [2] => [[1, 4, 9], [0, 1, 4]]
        # [[1, 2, 3], [0, 1, 2]] * [1, 2] => error
        # [[1, 2, 3], [0, 1, 2]] * [2, 1, 0] => [[1, 2, 1], [0, 1, 2]]

    def test_remainder(self):
        """
        Return element-wise remainder of division

        Computes the remainder complementary to the floor_divide function.
        It is equivalent to the Python modulus operator``x1 % x2`` and has the
        same sign as the divisor x2.

        This should not be confused with:

        Python 3.7’s math.remainder and C’s remainder, which computes the IEEE
        remainder, which are the complement to round(x1 / x2).

        Returns 0 when x2 is 0 and both x1 and x2 are (arrays of) integers.

        """
        ufunc = np.remainder

        # [0m, 2m, 4m] / [1, 2, 3] = [0m, 0m, 1m]
        numerator = self.v2
        denominator = QV([1, 2, 3], timestamp=self.ts2)
        res = QV([0, 0, 1], unit='m', timestamp=self.ts3)
        self.assertBinaryUfunc(ufunc, numerator, denominator, res)

        # FIXME: units of denominator are just ignored!
        # [0m, 2m, 4m] / [1s, 2s, 3s] = [0m, 0m, 1m]
        denominator = QV([1, 2, 3], unit='s', timestamp=self.ts2)
        self.assertBinaryUfunc(ufunc, numerator, denominator, res)

        # zero division:
        denominator = QV([0, 0, 1], timestamp=self.ts2)
        res = self.v0
        # arrays of ints: [0m, 2m, 4m] / [0, 0, 1] = [0m, 0m, 0m]
        with self.assertWarns(RuntimeWarning):
            self.assertBinaryUfunc(ufunc, numerator, denominator, res)

        # arrays of floats: [0m, 2m, 4m] / [0., 0., 1.] = [nan, nan, 0m]
        denominator = QV([0., 0., 1.], timestamp=self.ts2)
        res = QV([np.nan, np.nan, 0], unit='m')
        if _is_numpy_old():
            self.assertBinaryUfunc(ufunc, numerator, denominator, res)
        else:
            if system() == "Darwin":
                self.assertBinaryUfunc(ufunc, numerator, denominator, res)
            else:
                with self.assertWarns(RuntimeWarning):
                    self.assertBinaryUfunc(ufunc, numerator, denominator, res)

        # Negative values (differ from the result of `fmod`):
        # [0m, 2m, 4m] / [-1, 2, -3] = [0m, 0m, -1m]
        numerator = self.v2
        denominator = QV([-1, 2, -3], timestamp=self.ts2)
        res = QV([0, 0, -2], unit='m', timestamp=self.ts3)
        self.assertBinaryUfunc(ufunc, numerator, denominator, res)

    def test_mod(self):
        """
        `mod` is the same as `reminder`, already tested
        """
        assert np.mod is np.remainder

    def test_fmod(self):
        """
        Return the element-wise remainder of division.

        This is the NumPy implementation of the C library function `fmod`,
        the remainder has the same sign as the dividend `x1`.

        The result of the modulo operation for negative dividend and divisors
        is bound by conventions. For fmod, the sign of result is the sign of
        the dividend, while for remainder the sign of the result is the sign
        of the divisor.

        """
        ufunc = np.fmod

        # [0m, 2m, 4m] / [1, 2, 3] = [0m, 0m, 1m]
        numerator = self.v2
        denominator = QV([1, 2, 3], timestamp=self.ts2)
        res = QV([0, 0, 1], unit='m', timestamp=self.ts3)
        self.assertBinaryUfunc(ufunc, numerator, denominator, res)

        # zero division:
        denominator = QV([0, 0, 1], timestamp=self.ts2)
        res = self.v0
        # arrays of ints: [0m, 2m, 4m] / [0, 0, 1] = [0m, 0m, 0m]
        with self.assertWarns(RuntimeWarning):
            self.assertBinaryUfunc(ufunc, numerator, denominator, res)

        # arrays of floats: [0m, 2m, 4m] / [0., 0., 1.] = [nan, nan, 0s]
        denominator = QV([0., 0., 1.], timestamp=self.ts2)
        res = QV([np.nan, np.nan, 0], unit='m')
        if _is_numpy_old():
            self.assertBinaryUfunc(ufunc, numerator, denominator, res)
        else:
            if system() == "Darwin":
                self.assertBinaryUfunc(ufunc, numerator, denominator, res)
            else:
                with self.assertWarns(RuntimeWarning):
                    self.assertBinaryUfunc(ufunc, numerator, denominator, res)

        # units: [1m, 1m, 1m] / [1, 1, 1] != [1s, 1s, 1s]
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, self.v1, self.one, self.s1)

        # Negative values (differ from the result of `mod`):
        # [0m, 2m, 4m] / [-1, 2, -3] = [0m, 0m, -1m]
        numerator = self.v2
        denominator = QV([-1, 2, -3], timestamp=self.ts2)
        res = QV([0, 0, 1], unit='m', timestamp=self.ts3)
        self.assertBinaryUfunc(ufunc, numerator, denominator, res)

    def test_divmod(self):
        """
        Return element-wise quotient and remainder simultaneously.

        `np.divmod(x, y)` is equivalent to `(x // y, x % y)`,
        but faster because it avoids redundant work. It is used to implement
        the Python built-in function `divmod` on NumPy arrays.

        XXX: Returns a tuple of `numpy.array`s
        """
        ufunc = np.divmod
        # [0m, 2m, 4m] / [1, 2, 3] = ([0, 1, 1], [0, 0, 1])
        numerator = self.v2
        denominator = QV([1, 2, 3], timestamp=self.ts2)

        with self.assertRaises(NotImplementedError):
            ufunc(numerator, denominator)

    def test_absolute(self):
        """
        Calculate the absolute value element-wise.

        `np.abs` is a shorthand for this function.
        """
        ufunc = np.absolute
        # | 0, 0, 0 | = | 0, 0, 0 |
        self.assertUnaryUfunc(ufunc, self.v0, self.v0)
        # | 1, 1, 1 | = | 1, 1, 1 |
        self.assertUnaryUfunc(ufunc, self.v1, self.v1)
        # | -1, -1, -1 | = | 1, 1, 1 |
        self.assertUnaryUfunc(ufunc, -1 * self.v1, self.v1)
        # | -1, 0, 1 | = | 1, 0, 1 |
        a = QV([-1, 0, 1], unit='rad', timestamp=self.ts1)
        r = QV([1, 0, 1], unit='rad', timestamp=self.ts1)
        self.assertUnaryUfunc(ufunc, a, r)

        with self.assertRaises(AssertionError):
            # units are respected
            self.assertUnaryUfunc(ufunc, -1 * self.v1, self.v1_mm)
            # timestamps are respected
            self.assertUnaryUfunc(ufunc, self.v1_noTS, self.v1)

    def test_fabs(self):
        """
        Compute the absolute values element-wise.

        This function returns the absolute values (positive magnitude) of the
        data in x. Complex values are not handled, use `absolute` to find
        the absolute values of complex data.

        XXX: Returns numpy.array

        """
        ufunc = np.fabs
        # | 0, 0, 0 | = | 0, 0, 0 |
        self.assertUnaryUfunc(ufunc, self.v0, self.v0)
        # | 1, 1, 1 | = | 1, 1, 1 |
        self.assertUnaryUfunc(ufunc, self.v1, self.v1)
        # | -1, -1, -1 | = | 1, 1, 1 |
        self.assertUnaryUfunc(ufunc, -1 * self.v1, self.v1)
        # | -1, 0, 1 | = | 1, 0, 1 |
        a = QV([-1, 0, 1], unit='rad', timestamp=self.ts1)
        r = QV([1, 0, 1], unit='rad', timestamp=self.ts1)
        self.assertUnaryUfunc(ufunc, a, r)

    def test_rint(self):
        """Round elements of the array to the nearest integer."""
        ufunc = np.rint

        # from numpy documentation:
        a = np.array([-1.7, -1.5, -0.2, 0.2, 1.5, 1.7, 2.0])
        r = np.array([-2., -2., -0., 0., 2., 2., 2.])
        inp = QV(a, unit='m', timestamp=self.ts1)
        out = QV(r, unit='m', timestamp=self.ts1)
        self.assertUnaryUfunc(ufunc, inp, out)

        self.assertUnaryUfunc(ufunc, self.v0, self.v0)
        self.assertUnaryUfunc(ufunc, self.v1, self.v1)

        self.assertUnaryUfunc(ufunc, 1000 * self.v1_mm, self.v1)

    def test_sign(self):
        """
        XXX: Returns numpy.array
        """
        ufunc = np.sign
        self.assertUnaryUfunc(ufunc, self.v0, self.zero)
        self.assertUnaryUfunc(ufunc, self.v1, self.one)

    def test_heaviside(self):
        """
        Compute the Heaviside step function.
        The Heaviside step function is defined as:

                              0   if x1 < 0
        heaviside(x1, x2) =  x2   if x1 == 0
                              1   if x1 > 0

        where x2 is often taken to be 0.5, but 0 and 1 are also sometimes used.

        New in version 1.13.0.

        XXX: Returns numpy.array
        """
        ufunc = np.heaviside
        # θ([-1.5, 0, 2.0], 0.5) = [0., 0.5, 1.]
        a = QV([-1.5, 0, 2.0], unit='m', timestamp=self.ts1)
        r = np.array([0., 0.5, 1.])
        with self.assertRaises(NotImplementedError):
            self.assertBinaryUfunc(ufunc, a, 0.5, r)

    def test_exp(self):
        """
        XXX: Returns numpy.array
        """
        ufunc = np.exp
        # e⁰ = 1
        self.assertUnaryUfunc(ufunc, self.zero, self.one)
        # e¹ = e
        # Note: Before we were comparing with a simple ndarray.
        # However, a simple ndarray does not have units and timestamps. This
        # function itself works
        r = QV(np.array([e, e, e]), timestamp=self.one.timestamp)
        self.assertUnaryUfunc(ufunc, self.one, r)

        with self.assertRaises(DimensionalityError):
            # only accepts dimensionless argument
            self.assertUnaryUfunc(ufunc, self.v1, self.one)

    def test_exp2(self):
        """
        Calculate 2**p for all p in the input array.

        XXX: Returns numpy.array
        """
        ufunc = np.exp2
        # 2⁰ = 1
        self.assertUnaryUfunc(ufunc, self.zero, self.one)
        # 2¹ = 2
        r = QV(np.array([2, 2, 2]), timestamp=self.one.timestamp)
        self.assertUnaryUfunc(ufunc, self.one, r)

        with self.assertRaises(DimensionalityError):
            # only accepts dimensionless argument
            self.assertUnaryUfunc(ufunc, self.v1, self.one)

    def test_log(self):
        """
        Natural logarithm, element-wise.

        Logarithm is a multivalued function: for each `x` there is an infinite
        number of `z` such that `exp(z) = x`. The convention is to return the
        `z` whose imaginary part lies in `[-pi, pi]`.

        For real-valued input data types, `log` always returns real output. For
        each value that cannot be expressed as a real number or infinity, it
        yields `nan` and sets the `invalid` floating point error flag.

        For complex-valued input, `log` is a complex analytical function that
        has a branch cut `[-inf, 0]` and is continuous from above on it.
        `log` handles the floating-point negative zero as an infinitesimal
        negative number, conforming to the C99 standard.

        XXX: Returns numpy.array
        """
        ufunc = np.log
        # log([-1, 0, 1, e]) = [-Nan, -∞, 0, 1]
        a = QV([-1, 0, 1, e], timestamp=self.ts1)
        r = QV([np.nan, -np.inf, 0, 1], timestamp=self.ts1)
        with self.assertWarns(RuntimeWarning):
            self.assertUnaryUfunc(ufunc, a, r)

        with self.assertRaises(DimensionalityError):
            # only accepts dimensionless argument
            self.assertUnaryUfunc(ufunc, self.v1, self.one)

    def test_log2(self):
        """
        Base-2 logarithm of `x`.

        XXX: Returns numpy.array
        """
        ufunc = np.log2
        # log2([-1, 0, 1, 2]) = [-Nan, -∞, 0, 1]
        a = QV([-1, 0, 1, 2], timestamp=self.ts1)
        r = QV([np.nan, -np.inf, 0, 1], timestamp=self.ts1)
        with self.assertWarns(RuntimeWarning):
            self.assertUnaryUfunc(ufunc, a, r)

        with self.assertRaises(DimensionalityError):
            # only accepts dimensionless argument
            self.assertUnaryUfunc(ufunc, self.v1, self.one)

    def test_log10(self):
        """
        Base-10 logarithm of `x`.

        XXX: Returns numpy.array
        """
        ufunc = np.log10
        # log2([-1, 0, 1e-15, 100]) = [-Nan, -∞, -15, 2]
        a = QV([-1, 0, 1e-15, 100], timestamp=self.ts1)
        r = QV([np.nan, -np.inf, -15, 2], timestamp=self.ts1)
        with self.assertWarns(RuntimeWarning):
            self.assertUnaryUfunc(ufunc, a, r)

        with self.assertRaises(DimensionalityError):
            # only accepts dimensionless argument
            self.assertUnaryUfunc(ufunc, self.v1, self.one)

    def test_expm1(self):
        """
        Calculate `exp(x) - 1` for all elements in the array.

        XXX: Returns numpy.array
        """
        ufunc = np.expm1
        # e⁰ - 1 = 0
        self.assertUnaryUfunc(ufunc, self.zero, self.zero)

        with self.assertRaises(DimensionalityError):
            # only accepts dimensionless argument
            self.assertUnaryUfunc(ufunc, self.v1, self.one)

    def test_log1p(self):
        """
        Calculates `log(1 + x)`.

        XXX: Returns numpy.array
        """
        ufunc = np.log1p
        # log([-2, -1, 0, 1, e]) = [-Nan, -∞, 0, 1]
        a = QV([-2, -1, 0, 1e-99, e - 1], timestamp=self.ts1)
        r = QV([np.nan, -np.inf, 0, 1e-99, 1], timestamp=self.ts1)
        with self.assertWarns(RuntimeWarning):
            self.assertUnaryUfunc(ufunc, a, r)

        with self.assertRaises(DimensionalityError):
            # only accepts dimensionless argument
            self.assertUnaryUfunc(ufunc, self.v1, self.one)

    def test_sqrt(self):
        """
        Return the non-negative square-root of an array, element-wise
        """
        ufunc = np.sqrt
        # sqrt(1 m²) = 1 m
        self.assertUnaryUfunc(ufunc, self.v1 * self.v1, self.v1)

        x = QV([4, -1, np.inf], unit='m ** 2', timestamp=self.ts1)
        res = QV([2., np.nan, np.inf], unit='m', timestamp=self.ts1)
        with self.assertWarns(RuntimeWarning):
            self.assertUnaryUfunc(ufunc, x, res)

    def test_square(self):
        """Return the element-wise square of the input."""
        ufunc = np.square
        # ([0, 2, 4] m )² = [0, 4, 16] m²
        m_square = QV([0, 4, 16], unit='m ** 2', timestamp=self.ts3)
        self.assertUnaryUfunc(ufunc, self.v2, m_square)

    def test_cbrt(self):
        """
        Return the cube-root of an array, element-wise.

        XXX: Returns numpy.array

        """
        ufunc = np.cbrt

        # XXX: no ability to check dimensions
        self.assertUnaryUfunc(ufunc, self.v1 ** 3, self.v1)

        x = QV([1, 8, 27], timestamp=self.ts1)
        res = QV([1., 2., 3.], timestamp=self.ts1)
        self.assertUnaryUfunc(ufunc, x, res)

    def test_reciprocal(self):
        """Calculates `1/x`."""
        ufunc = np.reciprocal
        a = QV([1, 2, 1e-5], unit='m', timestamp=self.ts1)
        r = QV([1, 0.5, 100000], unit='1/m', timestamp=self.ts1)
        self.assertUnaryUfunc(ufunc, a, r)

        # It is possible to pass zero to this function (please don't!)
        # and the result will be unexpected (not `np.inf`)!
        # [0, 2, 4] → [???, 0.5, 0.25]
        r = QV([np.inf, 0.5, 0.25], unit='1/m', timestamp=self.ts3)
        with self.assertWarns(RuntimeWarning):
            with self.assertRaises(AssertionError):
                self.assertUnaryUfunc(ufunc, self.v2, r)

    def test_gcd(self):
        """Returns the greatest common divisor of `|x1|` and `|x2|`

        XXX: Returns numpy.array
        """
        try:
            ufunc = np.gcd
        except AttributeError:
            # some versions of numpy do not have this ufunc
            return

        # GCD(1, 1) = 1
        with self.assertRaises(NotImplementedError):
            self.assertBinaryUfunc(ufunc, self.v1, self.v1, self.v1)

    def test_lcm(self):
        """Returns the lowest common multiple of `|x1|` and `|x2|`

        XXX: Returns numpy.array
        """
        try:
            ufunc = np.lcm
        except AttributeError:
            # some versions of numpy do not have this ufunc
            return

        with self.assertRaises(NotImplementedError):
            self.assertBinaryUfunc(ufunc, self.v1, self.v1, self.v1)

    """
    ** Trigonometric functions **
    All trigonometric functions use radians when an angle is called for.
    The ratio of degrees to radians is 180°/π.
    """

    def test_sin(self):
        ufunc = np.sin
        self.assertUnaryUfunc(ufunc, self.zero_rad, self.zero)
        self.assertUnaryUfunc(ufunc, self.half_pi, self.one)

        # XXX: Should only accept radians as an argument, but it also accepts
        #      dimensionless ones
        self.assertUnaryUfunc(ufunc, self.zero, self.zero)
        # Though it doesn't accept meters or other definitely wrong stuff
        with self.assertRaises(DimensionalityError):
            self.assertUnaryUfunc(ufunc, self.v0, self.v0)

    def test_cos(self):
        ufunc = np.cos
        self.assertUnaryUfunc(ufunc, self.zero_rad, self.one)
        self.assertUnaryUfunc(ufunc, self.half_pi, self.zero)

        # XXX: Should only accept radians as an argument, but it also accepts
        #      dimensionless ones
        self.assertUnaryUfunc(ufunc, self.zero, self.one)
        # Though it doesn't accepts meters or other definitely wrong stuff
        with self.assertRaises(DimensionalityError):
            self.assertUnaryUfunc(ufunc, self.v0, self.v1)

    def test_tan(self):
        ufunc = np.tan
        # tan(0 rad) = 0
        self.assertUnaryUfunc(ufunc, self.zero_rad, self.zero)
        # tan(pi/4 rad) = 1
        self.assertUnaryUfunc(ufunc, self.half_pi / 2, self.one)

        self.assertUnaryUfunc(ufunc, self.zero_rad, self.zero)

        # XXX: Should only accept radians as an argument, but it also accepts
        #      dimensionless ones
        self.assertUnaryUfunc(ufunc, self.zero, self.zero)
        # Though it doesn't accept meters or other definitely wrong stuff
        with self.assertRaises(DimensionalityError):
            self.assertUnaryUfunc(ufunc, self.v0, self.v0)

    def test_arcsin(self):
        ufunc = np.arcsin
        # arcsin(0) = 0 rad
        self.assertUnaryUfunc(ufunc, self.zero, self.zero_rad)
        # arcsin(1) = pi/2 rad
        self.assertUnaryUfunc(ufunc, self.one, self.half_pi)

        # The function should only take dimensionless arguments
        with self.assertRaises(DimensionalityError):
            self.assertUnaryUfunc(ufunc, self.v1, self.half_pi)
            self.assertUnaryUfunc(ufunc, self.zero_rad, self.zero)

        # arcsin is defined only inside the range [-1, 1]
        with self.assertWarns(RuntimeWarning):
            nan_rad = QV(self.nan_vec.value, unit='rad', timestamp=self.ts1)
            self.assertUnaryUfunc(ufunc, self.one * 1.01, nan_rad)

    def test_arccos(self):
        ufunc = np.arccos
        # arccos(0) = pi/2 rad
        self.assertUnaryUfunc(ufunc, self.zero, self.half_pi)
        # arccos(1) = 0 rad
        self.assertUnaryUfunc(ufunc, self.one, self.zero_rad)

        # The function should only take dimensionless arguments
        with self.assertRaises(DimensionalityError):
            self.assertUnaryUfunc(ufunc, self.v1, self.zero_rad)
            self.assertUnaryUfunc(ufunc, self.zero_rad, self.half_pi)

        # arcsin is defined only inside the range [-1, 1]
        with self.assertWarns(RuntimeWarning):
            nan_rad = QV(self.nan_vec.value, unit='rad', timestamp=self.ts1)
            self.assertUnaryUfunc(ufunc, self.one * 1.01, nan_rad)

    def test_arctan(self):
        ufunc = np.arctan
        # arctan(0) = 0 rad
        self.assertUnaryUfunc(ufunc, self.zero, self.zero_rad)
        # arctan(1) = pi/4 rad
        self.assertUnaryUfunc(ufunc, self.one, self.half_pi / 2)

        # The function should only take dimensionless arguments
        with self.assertRaises(DimensionalityError):
            self.assertUnaryUfunc(ufunc, self.v1, self.half_pi / 2)
            self.assertUnaryUfunc(ufunc, self.zero_rad, self.zero)

    def test_arctan2(self):
        """
        Element-wise arc tangent of `x1/x2` choosing the quadrant correctly.

        The quadrant (i.e., branch) is chosen so that `arctan2(x1, x2)` is
        the signed angle in radians between the ray ending at the origin and
        passing through the point (1,0), and the ray ending at the origin and
        passing through the point (x2, x1).
        (Note the role reversal: the “y-coordinate” is the first function
        parameter, the “x-coordinate” is the second.) By IEEE convention, this
        function is defined for x2 = +/-0 and for either or both of x1 and
        x2 = +/-inf (see Notes for specific values).

        This function is not defined for complex-valued arguments;
        for the so-called argument of complex values, use angle.
        """
        ufunc = np.arctan2
        # arctan2(0,1) = arctan(0/1) = 0 rad
        self.assertBinaryUfunc(ufunc, self.zero, self.one, self.zero_rad)
        # arctan2(1,1) = arctan(1/1) = pi/4 rad
        self.assertBinaryUfunc(ufunc, self.one, self.one, self.half_pi / 2)
        #
        # The function should only take dimensionless arguments
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, self.v1, self.one, self.half_pi / 2)
            self.assertUnaryUfunc(ufunc, self.zero_rad, self.one, self.zero)

        # examples from the scipy reference:
        x = QV([-1, +1, +1, -1], unit='mm', timestamp=self.ts1)
        y = QV([-1, -1, +1, +1], unit='mm', timestamp=self.ts2)
        res = QV([-135., -45., 45., 135.], unit='rad', timestamp=self.ts2)
        self.assertBinaryUfunc(ufunc, y, x, res / 180 * np.pi)

        x = QV([1., -1.], unit='s', timestamp=self.ts1)
        y = QV([0., 0.], unit='s', timestamp=self.ts2)
        res = QV([np.pi / 2, -np.pi / 2], unit='rad', timestamp=self.ts2)
        self.assertBinaryUfunc(ufunc, x, y, res)

        x = QV([0., 0., np.inf], unit='m', timestamp=self.ts1)
        y = QV([+0., -0., np.inf], unit='m', timestamp=self.ts2)
        res = QV([0., np.pi, np.pi / 4], unit='rad', timestamp=self.ts2)
        self.assertBinaryUfunc(ufunc, x, y, res)

    def test_hypot(self):
        """
        Given the “legs” of a right triangle, return its hypotenuse.

        Equivalent to `sqrt(x1**2 + x2**2)`, element-wise. If `x1` or `x2` is
        scalar-like (i.e., unambiguously cast-able to a scalar type), it is
        broadcast for use with each element of the other argument.
        """
        ufunc = np.hypot
        # hypot(0, 0) = sqrt(0) = 0
        self.assertBinaryUfunc(ufunc, self.v0, self.v0, self.v0)
        # Egyptian triangle
        self.assertBinaryUfunc(ufunc, 3 * self.v1, 4 * self.v1, 5 * self.v1)

        # compatible units: hypot(3 m, 4000 mm) = 5 m
        self.assertBinaryUfunc(ufunc, 3 * self.v1, 4000 * self.v1_mm,
                               5 * self.v1)

        # incompatible units: hypot(3 m, 4000 mm) = 5 m
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, 3 * self.v1, 4 * self.s1,
                                   5 * self.v1)

    def test_sinh(self):
        """
        If `out` is provided, the function writes the result into it, and
        returns a reference to `out`.
        """
        ufunc = np.sinh
        # sinh(0) = 0
        self.assertUnaryUfunc(ufunc, self.zero_rad, self.zero)
        # sinh(-π/2) = -sinh(π/2)
        self.assertUnaryUfunc(ufunc, -1 * self.half_pi,
                              -1 * np.sinh(self.half_pi))

        # May take numbers instead of radians as an argument.
        self.assertUnaryUfunc(ufunc, self.zero, self.zero)
        # Though it doesn't accept meters or other definitely wrong stuff
        with self.assertRaises(DimensionalityError):
            self.assertUnaryUfunc(ufunc, self.v0, self.v0)

    def test_cosh(self):
        """
        If `out` is provided, the function writes the result into it, and
        returns a reference to `out`.
        """
        ufunc = np.cosh
        # cosh(0) = 1
        self.assertUnaryUfunc(ufunc, self.zero_rad, self.one)
        # cosh(-π/2) = cosh(π/2)
        self.assertUnaryUfunc(ufunc, -1 * self.half_pi, np.cosh(self.half_pi))

        # May take numbers instead of radians as an argument.
        self.assertUnaryUfunc(ufunc, self.zero, self.one)
        # Though it doesn't accept meters or other definitely wrong stuff
        with self.assertRaises(DimensionalityError):
            self.assertUnaryUfunc(ufunc, self.v0, self.v0)

    def test_tanh(self):
        """
        Equivalent to np.sinh(x)/np.cosh(x) or -1j * np.tan(1j*x).
        """
        ufunc = np.tanh
        # tanh(0) = 0
        self.assertUnaryUfunc(ufunc, self.zero_rad, self.zero)
        # tanh(-π/2) = -tanh(π/2)
        self.assertUnaryUfunc(ufunc, -1 * self.half_pi,
                              -1 * np.tanh(self.half_pi))

        # May take numbers instead of radians as an argument.
        self.assertUnaryUfunc(ufunc, self.zero, self.zero)
        # Though it doesn't accept meters or other definitely wrong stuff
        with self.assertRaises(DimensionalityError):
            self.assertUnaryUfunc(ufunc, self.v0, self.v0)

    def test_arcsinh(self):
        ufunc = np.arcsinh
        # arcsinh(0) = 0
        self.assertUnaryUfunc(ufunc, self.zero_rad, self.zero_rad)
        # arcsinh(-π/2) = -arcsinh(π/2)
        self.assertUnaryUfunc(ufunc, -1 * self.half_pi,
                              -1 * np.arcsinh(self.half_pi))

        # The function should only take radians as an argument
        with self.assertRaises(DimensionalityError):
            self.assertUnaryUfunc(ufunc, self.v1, np.arcsinh(self.one))

    def test_arccosh(self):
        ufunc = np.arccosh
        # arccosh(1) = 0
        self.assertUnaryUfunc(ufunc, self.one, self.zero_rad)
        # cosh(-π/4) = cosh(π/4)
        with self.assertWarns(RuntimeWarning):
            self.assertUnaryUfunc(ufunc, - 0.5 * self.half_pi,
                                  np.arccosh(self.half_pi / 2))

        # The function should only take radians as an argument
        with self.assertRaises(DimensionalityError):
            self.assertUnaryUfunc(ufunc, self.v1, self.zero)

    def test_arctanh(self):
        ufunc = np.arctanh
        # arctanh(0) = 0
        self.assertUnaryUfunc(ufunc, self.zero_rad, self.zero_rad)
        # arctanh(-π/2) = -arctanh(π/2)
        with self.assertWarns(RuntimeWarning):
            self.assertUnaryUfunc(ufunc, -1 * self.half_pi,
                                  -1 * np.arctanh(self.half_pi))

        # The function should only take radians as an argument
        with self.assertWarns(RuntimeWarning):
            with self.assertRaises(DimensionalityError):
                self.assertUnaryUfunc(ufunc, self.v1, np.arctanh(self.one))

    def test_deg2rad(self):
        """
        Convert angles from degrees to radians

        XXX: returns the wrong result!

        """
        ufunc = np.deg2rad
        # 0° = 0 rad
        self.assertUnaryUfunc(ufunc, self.zero, self.zero_rad)
        # 90° = π/2
        ninety = QV([90, 90, 90], unit='deg', timestamp=self.ts1)
        self.assertUnaryUfunc(ufunc, ninety, self.half_pi)
        # NB: dimensionless input produces the wrong answer!
        ninety_dimless = QV([90, 90, 90], timestamp=self.ts1)
        with self.assertRaises(AssertionError):
            self.assertUnaryUfunc(ufunc, ninety_dimless, self.half_pi)

        # The function should only arguments in degrees
        with self.assertRaises(DimensionalityError):
            self.assertUnaryUfunc(ufunc, self.v0, self.zero)

    def test_rad2deg(self):
        """
        Note, that the result MUST be in degrees!
        """
        ufunc = np.rad2deg
        # 0 rad = 0°
        zero_deg = QV([0, 0, 0], unit='deg', timestamp=self.ts1)

        self.assertUnaryUfunc(ufunc, self.zero_rad, zero_deg)
        # π/2 = 90°
        ninety = QV([90, 90, 90], unit='deg', timestamp=self.ts1)
        self.assertUnaryUfunc(ufunc, self.half_pi, ninety)

        # The function should only take radians as an argument
        with self.assertRaises(DimensionalityError):
            self.assertUnaryUfunc(ufunc, self.v0, self.zero)

    """
    *** Bit-twiddling functions **
    These function all require integer arguments and they manipulate
    the bit-pattern of those arguments.
    """

    def test_bitwise_and(self):
        """
        Compute the bit-wise AND of two arrays element-wise.

        Computes the bit-wise AND of the underlying binary representation of
        the integers in the input arrays. This ufunc implements the C/Python
        operator `&`.

        XXX: Returns numpy.array

        """
        ufunc = np.bitwise_and
        # XXX: units measurements are ignored!
        a = QV([11, 1], unit='m', timestamp=self.ts1)
        b = QV([4, 25], unit='s', timestamp=self.ts2)
        r = QV([0, 1], timestamp=self.ts2)
        with self.assertRaises(NotImplementedError):
            self.assertBinaryUfunc(ufunc, a, b, r)

    def test_bitwise_or(self):
        """
        Compute the bit-wise OR of two arrays element-wise.

        Computes the bit-wise OR of the underlying binary representation of
        the integers in the input arrays. This ufunc implements the C/Python
        operator `|`.

        XXX: Returns numpy.array
        """
        ufunc = np.bitwise_or
        # XXX: units measurements are ignored!
        a = QV([2, 5, 255], unit='m', timestamp=self.ts1)
        b = QV([4, 4, 4], unit='s', timestamp=self.ts2)
        r = QV([6, 5, 255], timestamp=self.ts2)
        with self.assertRaises(NotImplementedError):
            self.assertBinaryUfunc(ufunc, a, b, r)

    def test_bitwise_xor(self):
        """
        Compute the bit-wise XOR of two arrays element-wise.

        Computes the bit-wise XOR of the underlying binary representation of \
        the integers in the input arrays. This ufunc implements the C/Python
        operator `^`.

        XXX: Returns numpy.array
        """
        ufunc = np.bitwise_xor
        # XXX: units measurements are ignored!
        a = QV([31, 3], unit='m', timestamp=self.ts1)
        b = QV([5, 6], unit='s', timestamp=self.ts2)
        r = QV([26, 5], timestamp=self.ts2)
        with self.assertRaises(NotImplementedError):
            self.assertBinaryUfunc(ufunc, a, b, r)

    def test_invert(self):
        """
        Computes the bit-wise NOT of the underlying binary representation of
        the integers in the input arrays. This ufunc implements the C/Python
        operator `~`.

        """
        ufunc = np.invert
        a = QV(np.array([0, 2, 13], dtype=np.int8), unit='m',
               timestamp=self.ts1)
        r = QV(np.array([-1, -3, -14], dtype=np.int8), timestamp=self.ts2)
        with self.assertRaises(NotImplementedError):
            self.assertUnaryUfunc(ufunc, a, r)

    def test_left_shift(self):
        """
        Shift the bits of an integer to the left.

        Bits are shifted to the left by appending x2 0s at the right of x1.
        Since the internal representation of numbers is in binary format, this
        operation is equivalent to multiplying x1 by `2**x2`.

        XXX: Returns numpy.array
        """
        ufunc = np.left_shift
        # XXX: units measurements are ignored!
        a = QV([1, 2, 3], unit='m', timestamp=self.ts1)
        r = QV([2, 4, 8], timestamp=self.ts2)

        with self.assertRaises(NotImplementedError):
            self.assertBinaryUfunc(ufunc, self.v1, a, r)

    def test_right_shift(self):
        """
        Shift the bits of an integer to the right.

        Bits are shifted to the right x2. Because the internal representation
        of numbers is in binary format, this operation is equivalent to
        dividing x1 by `2**x2`.

        XXX: Returns numpy.array
        """
        ufunc = np.right_shift
        # XXX: units measurements are ignored!
        a = QV([10, 10, 10], unit='m', timestamp=self.ts1)
        b = QV([1, 2, 3], unit='s', timestamp=self.ts1)
        r = QV([5, 2, 1], timestamp=self.ts2)
        with self.assertRaises(NotImplementedError):
            self.assertBinaryUfunc(ufunc, a, b, r)

    """
    ** Comparison functions **
    """

    def test_greater(self):
        """
        XXX: Returns numpy.array
        """
        ufunc = np.greater
        # [1, 1, 1] > [0, 2, 4] = [True, False, False]
        r = QV([True, False, False], unit='s')
        # XXX: units must be comparable, but the result units might be chosen
        #      arbitrarily!
        self.assertBinaryUfunc(ufunc, self.v1, self.v2, r)
        # [1m, 1m, 1m] > [1mm, 1mm, 1mm] = [True, True, True]
        self.assertBinaryUfunc(ufunc, self.v1, self.v1_mm, self.true)

        # ints and floats are comparable if units are correct
        self.assertBinaryUfunc(ufunc, self.s1, self.f1, self.false)

        # Incompatible units
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, self.v1, self.s1, r)

    def test_greater_equal(self):
        """
        XXX: Returns numpy.array
        """
        ufunc = np.greater_equal
        # [1, 1, 1] >= [0, 2, 4] = [True, False, False]
        r = QV([True, False, False], unit='s')
        # XXX: units must be comparable, but the result units might be chosen
        #      arbitrarily!
        self.assertBinaryUfunc(ufunc, self.v1, self.v2, r)
        # [1m, 1m, 1m] > [1mm, 1mm, 1mm] = [True, True, True]
        self.assertBinaryUfunc(ufunc, self.v1, self.v1_mm, self.true)

        # ints and floats are comparable if units are correct
        # [1 s, 1 s, 1 s] >= [1. s, 2. s, 3. s] = [True, False, False]
        self.assertBinaryUfunc(ufunc, self.s1, self.f1, [True, False, False])

        # Incompatible units
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, self.v1, self.s1, r)

    def test_less(self):
        """
        XXX: Returns numpy.array
        """
        ufunc = np.less
        # [1, 1, 1] < [0, 2, 4] = [False, True, True]
        r = QV([False, True, True], unit='s')
        # XXX: units must be comparable, but the result units might be chosen
        #      arbitrarily!
        self.assertBinaryUfunc(ufunc, self.v1, self.v2, r)
        # [1m, 1m, 1m] < [1mm, 1mm, 1mm] = [False, False, False]
        self.assertBinaryUfunc(ufunc, self.v1, self.v1_mm, self.false)

        # ints and floats are comparable if units are correct
        # [1 s, 1 s, 1 s] < [1. s, 2. s, 3. s] = [True, False, False]
        self.assertBinaryUfunc(ufunc, self.s1, self.f1, [False, True, True])

        # Incompatible units
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, self.v1, self.s1, r)

    def test_less_equal(self):
        """
        XXX: Returns numpy.array
        """
        ufunc = np.less_equal
        # [1, 1, 1] <= [0, 2, 4] = [False, True, True]
        r = QV([False, True, True], unit='s')
        # XXX: units must be comparable, but the result units might be chosen
        #      arbitrarily!
        self.assertBinaryUfunc(ufunc, self.v1, self.v2, r)
        # [1m, 1m, 1m] <= [1mm, 1mm, 1mm] = [False, False, False]
        self.assertBinaryUfunc(ufunc, self.v1, self.v1_mm, self.false)

        # ints and floats are comparable if units are correct
        # [1 s, 1 s, 1 s] <= [1. s, 2. s, 3. s] = [True, False, False]
        self.assertBinaryUfunc(ufunc, self.s1, self.f1, self.true)

        # Incompatible units
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, self.v1, self.s1, r)

    def test_not_equal(self):
        """
        Returns `x1 != x2` element-wise.

        XXX: Returns numpy.array
        """
        ufunc = np.not_equal
        # [1, 1, 1] != [0, 2, 4] = [True, True, True]
        r = QV([True] * 3, unit='s')
        # XXX: units must be comparable, but the result units might be chosen
        #      arbitrarily!
        self.assertBinaryUfunc(ufunc, self.v1, self.v2, r)
        # [1m, 1m, 1m] != [1mm, 1mm, 1mm] = [True, True, True]
        self.assertBinaryUfunc(ufunc, self.v1, self.v1_mm, self.true)

        # ints and floats are comparable if units are correct
        # [1 s, 1 s, 1 s] != [1. s, 2. s, 3. s] = [False, True, True]
        self.assertBinaryUfunc(ufunc, self.s1, self.f1, [False, True, True])

        # Incompatible units
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, self.v1, self.s1, r)

        c1 = QV([-1j, 0, 1 - 1j], unit='m')
        c2 = QV([-1j, -1 - 1j, 2j], unit='m')
        self.assertBinaryUfunc(ufunc, c1, c2, [False, True, True])

    def test_equal(self):
        """
        Returns `x1 == x2` element-wise.

        XXX: Returns numpy.array
        """
        ufunc = np.equal
        # [1, 1, 1] == [0, 2, 4] = [False, False, False]
        r = QV([False] * 3, unit='s')
        # XXX: units must be comparable, but the result units might be chosen
        #      arbitrarily!
        self.assertBinaryUfunc(ufunc, self.v1, self.v2, r)
        # [1m, 1m, 1m] == [1mm, 1mm, 1mm] = [False, False, False]
        self.assertBinaryUfunc(ufunc, self.v1, self.v1_mm, self.false)

        # ints and floats are comparable if units are correct
        # [1 s, 1 s, 1 s] == [1. s, 2. s, 3. s] = [True, False, False]
        self.assertBinaryUfunc(ufunc, self.s1, self.f1, [True, False, False])

        # Incompatible units
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, self.v1, self.s1, r)

        c1 = QV([0 - 1j, 0, 1 - 1j], unit='m')
        c2 = QV([-1j, -1 - 1j, 2j], unit='m')
        self.assertBinaryUfunc(ufunc, c1, c2, [True, False, False])

    def test_logical_and(self):
        """
        Compute the truth value of x1 AND x2 element-wise.

        XXX: Returns numpy.array
        """
        ufunc = np.logical_and

        # XXX: units are ignored completely, the result units might be chosen
        #      arbitrarily!
        x1 = QV([True, False], unit='s')
        x2 = QV([False, False], unit='m')
        xr = QV([False, False], unit='mm')

        with self.assertRaises(NotImplementedError):
            self.assertBinaryUfunc(ufunc, x1, x2, xr)

    def test_logical_or(self):
        """
        Compute the truth value of x1 OR x2 element-wise.

        XXX: Returns numpy.array
        """
        ufunc = np.logical_or

        # XXX: units are ignored completely, the result units might be chosen
        #      arbitrarily!
        x1 = QV([True, False], unit='s')
        x2 = QV([False, False], unit='m')
        xr = QV([True, False], unit='mm')
        with self.assertRaises(NotImplementedError):
            self.assertBinaryUfunc(ufunc, x1, x2, xr)

    def test_logical_xor(self):
        """
        Compute the truth value of x1 XOR x2 element-wise.

        XXX: Returns numpy.array
        """
        ufunc = np.logical_xor

        # XXX: units are ignored completely, the result units might be chosen
        #      arbitrarily!
        x1 = QV([True, False], unit='s')
        x2 = QV([False, False], unit='m')
        xr = QV([True, False], unit='mm')

        with self.assertRaises(NotImplementedError):
            self.assertBinaryUfunc(ufunc, x1, x2, xr)

    def test_logical_not(self):
        """
        Compute the truth value of NOT x element-wise.

        XXX: Returns numpy.array
        """
        ufunc = np.logical_not

        # XXX: units are ignored completely, the result units might be chosen
        #      arbitrarily!
        x1 = QV([True, False], unit='s')
        xr = QV([False, True], unit='mm')
        with self.assertRaises(NotImplementedError):
            self.assertUnaryUfunc(ufunc, x1, xr)

    @skip
    def test_maximum(self):
        """
        Element-wise maximum of array elements.

        Compare two arrays and returns a new array containing the element-wise
        maxima. If one of the elements being compared is a NaN, then that
        element is returned. If both elements are NaNs then the first is
        returned. The latter distinction is important for complex NaNs, which
        are defined as at least one of the real or imaginary parts being a NaN.
        The net effect is that NaNs are propagated.

        XXX: Returns numpy.array
        """
        ufunc = np.maximum

        # XXX: units are ignored completely, the result units might be chosen
        #      arbitrarily!
        x1 = QV([2, 3, 4], unit='s')
        x2 = QV([1, 5, 2], unit='m')
        xr = QV([2, 5, 4], unit='mm')
        self.assertBinaryUfunc(ufunc, x1, x2, xr)

        # FIXME: the above mentioned ignorance leads to the weird results!
        # For instance, maximum of [1m, 1m, 1m]  and [2mm, 2mm, 2mm]
        # is [2, 2, 2]!
        res = QV([2, 2, 2], unit='mm')
        with self.assertRaises(AssertionError):
            self.assertBinaryUfunc(ufunc, self.v1, 2 * self.v1_mm, res)

        # Incompatible units
        # FIXME: this should raise, since we cannot compare meters and seconds
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, self.v1, self.s1, self.v1)

        # NaN propagation:
        x1, x2 = QV([np.nan, 0, np.nan]), QV([0, np.nan, np.nan])
        res = QV([np.nan, np.nan, np.nan])
        self.assertBinaryUfunc(ufunc, x1, x2, res)

    @skip
    def test_minimum(self):
        """
        Element-wise minimum of array elements.

        Compare two arrays and returns a new array containing the element-wise
        minima. If one of the elements being compared is a NaN, then that
        element is returned. If both elements are NaNs then the first is
        returned. The latter distinction is important for complex NaNs, which
        are defined as at least one of the real or imaginary parts being a NaN.
        The net effect is that NaNs are propagated.

        XXX: Returns numpy.array
        """
        ufunc = np.minimum

        # XXX: units are ignored completely, the result units might be chosen
        #      arbitrarily!
        x1 = QV([2, 3, 4], unit='s')
        x2 = QV([1, 5, 2], unit='m')
        xr = QV([1, 3, 2], unit='mm')
        self.assertBinaryUfunc(ufunc, x1, x2, xr)

        # FIXME: the above mentioned ignorance leads to the weird results!
        # For instance, minimum of [1m, 1m, 1m]  and [2mm, 2mm, 2mm]
        # is [1, 1, 1]!
        res = QV([1, 1, 1], unit='m')
        with self.assertRaises(AssertionError):
            self.assertBinaryUfunc(ufunc, self.v1, 2 * self.v1_mm, res)

        # Incompatible units
        # FIXME: this should raise, since we cannot compare meters and seconds
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, self.v1, self.s1, self.v1)

        # NaN propagation:
        x1, x2 = QV([np.nan, 0, np.nan]), QV([0, np.nan, np.nan])
        res = QV([np.nan, np.nan, np.nan])
        self.assertBinaryUfunc(ufunc, x1, x2, res)

    @skip
    def test_fmax(self):
        """
        Element-wise maximum of array elements.

        Compare two arrays and returns a new array containing the element-wise
        maxima. If one of the elements being compared is a NaN, then the
        non-nan element is returned. If both elements are NaNs then the first
        is returned. The latter distinction is important for complex NaNs,
        which are defined as at least one of the real or imaginary parts being
        a NaN. The net effect is that NaNs are ignored when possible.

        XXX: Returns numpy.array
        """
        ufunc = np.fmax

        # XXX: units are ignored completely, the result units might be chosen
        #      arbitrarily!
        x1 = QV([2, 3, 4], unit='s')
        x2 = QV([1, 5, 2], unit='m')
        xr = QV([2, 5, 4], unit='mm')
        self.assertBinaryUfunc(ufunc, x1, x2, xr)

        # FIXME: the above mentioned ignorance leads to the weird results!
        # For instance, maximum of [1m, 1m, 1m]  and [2mm, 2mm, 2mm]
        # is [2, 2, 2]!
        res = QV([2, 2, 2], unit='mm')
        with self.assertRaises(AssertionError):
            self.assertBinaryUfunc(ufunc, self.v1, 2 * self.v1_mm, res)

        # Incompatible units
        # FIXME: this should raise, since we cannot compare meters and seconds
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, self.v1, self.s1, self.v1)

        # NaN suppression (it's the only difference with `maximum`):
        x1, x2 = QV([np.nan, 0, np.nan]), QV([0, np.nan, np.nan])
        res = QV([0, 0, np.nan])
        self.assertBinaryUfunc(ufunc, x1, x2, res)

    @skip
    def test_fmin(self):
        """
        Element-wise minimum of array elements.

        Compare two arrays and returns a new array containing the element-wise
        minima. If one of the elements being compared is a NaN, then
        the non-nan element is returned. If both elements are NaNs then
        the first is returned. The latter distinction is important for complex
        NaNs, which are defined as at least one of the real or imaginary parts
        being a NaN. The net effect is that NaNs are ignored when possible.

        XXX: Returns numpy.array
        """
        ufunc = np.fmin

        # XXX: units are ignored completely, the result units might be chosen
        #      arbitrarily!
        x1 = QV([2, 3, 4], unit='s')
        x2 = QV([1, 5, 2], unit='m')
        xr = QV([1, 3, 2], unit='mm')
        self.assertBinaryUfunc(ufunc, x1, x2, xr)

        # FIXME: the above mentioned ignorance leads to the weird results!
        # For instance, minimum of [1m, 1m, 1m]  and [2mm, 2mm, 2mm]
        # is [1, 1, 1]!
        res = QV([1, 1, 1], unit='m')
        with self.assertRaises(AssertionError):
            self.assertBinaryUfunc(ufunc, self.v1, 2 * self.v1_mm, res)

        # Incompatible units
        # FIXME: this should raise, since we cannot compare meters and seconds
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, self.v1, self.s1, self.v1)

        # NaN suppression (it's the only difference with `minimum`):
        x1, x2 = QV([np.nan, 0, np.nan]), QV([0, np.nan, np.nan])
        res = QV([0, 0, np.nan])
        self.assertBinaryUfunc(ufunc, x1, x2, res)

    """
    ** Floating functions **
    """

    def test_isfinite(self):
        """
        Test element-wise for finiteness (not infinity or not Not a Number).
        The result is returned as a boolean array.

        XXX: Returns numpy.array
        """
        ufunc = np.isfinite

        self.assertUnaryUfunc(ufunc, self.v1, self.true)
        with self.assertWarns(RuntimeWarning):
            x1 = QV([np.log(-1.), 1, np.log(0)], unit='m', timestamp=self.ts1)
        self.assertUnaryUfunc(ufunc, x1, [False, True, False])

        x2 = QV([np.nan, np.inf, -np.inf], unit='m', timestamp=self.ts1)
        self.assertUnaryUfunc(ufunc, x2, self.false)

    def test_isinf(self):
        """
        Test element-wise for positive or negative infinity.

        Returns a boolean array of the same shape as `x`,
        `True` where `x == +/-inf`, otherwise `False`.

        XXX: Returns numpy.array
        """
        ufunc = np.isinf

        self.assertUnaryUfunc(ufunc, self.v1, self.false)

        x1 = QV([np.inf, -np.inf, 1.0, np.nan])
        self.assertUnaryUfunc(ufunc, x1, [True, True, False, False])

        x2 = QV([np.inf, np.nan, -np.inf], unit='m', timestamp=self.ts1)
        self.assertUnaryUfunc(ufunc, x2, [True, False, True])

    def test_isnan(self):
        """
        Test element-wise for NaN and return result as a boolean array.

        XXX: Returns numpy.array
        """
        ufunc = np.isnan

        self.assertUnaryUfunc(ufunc, self.v1, self.false)
        with self.assertWarns(RuntimeWarning):
            x1 = QV([np.log(-1.), 1, np.log(0)], unit='m', timestamp=self.ts1)
        self.assertUnaryUfunc(ufunc, x1, [True, False, False])

        x2 = QV([np.nan, np.inf, -np.inf], unit='m', timestamp=self.ts1)
        self.assertUnaryUfunc(ufunc, x2, [True, False, False])

    @skip
    def test_isnat(self):
        """
        Test element-wise for NaT (not a time) and return result as a boolean
        array. ufunc `isnat` is only defined for datetime and timedelta

        XXX: Returns numpy.array
        """
        ufunc = np.isnat
        time_arr = np.array(["NaT", "2016-01-01"], dtype="datetime64[s]")
        np.testing.assert_equal(ufunc(time_arr), [True, False])

        # XXX: Handling of arrays of this type is not implemented in Karabo
        x = QV(time_arr, unit='s', timestamp=self.ts1)
        self.assertUnaryUfunc(ufunc, x, [True, False])

    def test_signbit(self):
        """
        Returns element-wise True where signbit is set (less than zero).

        XXX: Returns numpy.array
        """
        ufunc = np.signbit
        # signs [1, 1, 1] are `+`
        self.assertUnaryUfunc(ufunc, self.v1, self.false)
        # signbit([-1, 0, 1]) = [1, 0, 0]
        x = QV(np.array([-1, 0, 1]), unit='m', timestamp=self.ts1)
        self.assertUnaryUfunc(ufunc, x, [True, False, False])

    def test_copysign(self):
        """
        Change the sign of `x1` to that of `x2`, element-wise.

        If `x2` is a scalar, its sign will be copied to all elements of `x1`.
        """
        ufunc = np.copysign
        # XXX: in `copysign(A, B)` arrays `A` and `B` must be of comparable
        #      units (why?).
        # copysign( X, [1,1,1]) = X
        self.assertBinaryUfunc(ufunc, self.v1, self.v1_mm, self.v1)
        minus = QV([-1, -2, -3], unit='m', timestamp=self.ts2)
        self.assertBinaryUfunc(ufunc, self.v1, minus, -1 * self.v1)
        minus = QV([-0.01, 0, 3], unit='m', timestamp=self.ts2)
        self.assertBinaryUfunc(ufunc, self.v1, minus, [-1, 1, 1] * self.v1)
        # broadcasting
        minus = QV([-1], unit='m', timestamp=self.ts2)
        self.assertBinaryUfunc(ufunc, self.v1, minus, -1 * self.v1)

    def test_nextafter(self):
        """
        Return the next floating-point value after x1 towards x2, element-wise.
        """
        ufunc = np.nextafter
        eps = QV([np.finfo(np.float64).eps], unit=self.v0.u)
        # next bigger number
        self.assertBinaryUfunc(ufunc, self.v0, self.v1, self.v0 + eps)
        # next smaller number
        self.assertBinaryUfunc(ufunc, self.v1, self.v1_mm, self.v1 - eps)

        # Incompatible units
        with self.assertRaises(DimensionalityError):
            self.assertBinaryUfunc(ufunc, self.v1, self.s1, self.v1 - eps)

    def test_spacing(self):
        """
        Return the distance between x and the nearest adjacent number.

        XXX: Returns numpy.array
        """
        ufunc = np.spacing
        eps = QV([np.finfo(np.float64).eps] * 3, unit=self.v1.u)
        with self.assertRaises(NotImplementedError):
            self.assertUnaryUfunc(ufunc, self.v1, eps)

    def test_modf(self):
        """
        Return the fractional and integral parts of an array, element-wise.

        The fractional and integral parts are negative if the given number is
        negative.

        Note: `divmod(x, 1)` is equivalent to `modf` with the return values
        switched, except it always has a positive remainder.
        Note: For integer input the return values are floats.
        """
        ufunc = np.modf
        x = QV([1.23, 3.45, 6.78e-3], unit='m', timestamp=self.ts1)
        res1, res2 = ufunc(x)
        expected1 = QV([0.23, 0.45, 0.00678], unit='m', timestamp=self.ts1)
        expected2 = QV([1., 3., 0.], unit='m', timestamp=self.ts1)
        self.assertArrayEqual(res1, expected1)
        self.assertArrayEqual(res2, expected2)

    def test_ldexp(self):
        """
        Returns `x1 * 2**x2`, element-wise.

        The mantissas `x1` and twos exponents `x2` are used to construct
        floating point numbers `x1 * 2**x2`.

        Complex dtypes are not supported, they will raise a TypeError.

        `ldexp` is useful as the inverse of `frexp`, if used by itself it is
        more clear to simply use the expression `x1 * 2**x2`.
        """
        ufunc = np.ldexp
        self.assertBinaryUfunc(ufunc, self.f1, self.one, 2 * self.f1)

    def test_frexp(self):
        """
        Decompose the elements of `x` into mantissa and `two`s exponent.

        Returns (mantissa, exponent), where `x = mantissa * 2**exponent`.
        The mantissa is lies in the open interval(-1, 1), while the twos
        exponent is a signed integer

        XXX: returns a pair: (Karabo array, numpy array)
        """
        ufunc = np.frexp
        # Since it returns _two_ arrays, we cannot check it so easily
        # TODO: check timestamps

        inp1 = self.f1
        res1, res2 = ufunc(inp1)
        expected1 = QV([0.5, 0.5, 0.75], unit='s')
        expected2 = np.array([1, 2, 2])
        self.assertArrayEqual(res1, expected1)
        np.testing.assert_equal(res2, expected2)

        desc_inp = self.descriptors_from_QV(inp1)
        res1, res2 = ufunc(desc_inp)
        self.assertArrayEqual(res1, expected1)
        np.testing.assert_equal(res2, expected2)

    def test_floor(self):
        """
        Return the floor of the input, element-wise.

        The floor of the scalar `x` is the largest integer `i`, such that
        `i <= x`.

        """
        ufunc = np.floor
        x = QV([-1.7, -1.5, -0.2, 0.2, 1.5, 1.7, 2.0], unit='m',
               timestamp=self.ts1)
        res = QV([-2., -2., -1., 0., 1., 1., 2.], unit='m',
                 timestamp=self.ts1)
        self.assertUnaryUfunc(ufunc, x, res)

        # Incompatible units
        wrong_res = QV([-2., -2., -1., 0., 1., 1., 2.], unit='s',
                       timestamp=self.ts1)
        with self.assertRaises(DimensionalityError):
            self.assertUnaryUfunc(ufunc, x, wrong_res)

    def test_ceil(self):
        """
        Return the ceiling of the input, element-wise.

        The ceil of the scalar `x` is the smallest integer `i`, such that
        `i >= x`.
        """
        ufunc = np.ceil
        x = QV([-1.7, -1.5, -0.2, 0.2, 1.5, 1.7, 2.0], unit='m',
               timestamp=self.ts1)
        res = QV([-1., -1., -0., 1., 2., 2., 2.], unit='m',
                 timestamp=self.ts1)
        self.assertUnaryUfunc(ufunc, x, res)

        # Incompatible units
        wrong_res = QV([-1., -1., -0., 1., 2., 2., 2.], unit='s',
                       timestamp=self.ts1)
        with self.assertRaises(DimensionalityError):
            self.assertUnaryUfunc(ufunc, x, wrong_res)

    def test_trunc(self):
        """
        Return the truncated value of the input, element-wise.

        The truncated value of the scalar `x` is the nearest integer `i` which
        is closer to zero than `x` is. In short, the fractional part of the
        signed number `x` is discarded.
        """
        ufunc = np.trunc
        x = QV([-1.7, -1.5, -0.2, 0.2, 1.5, 1.7, 2.0], unit='m',
               timestamp=self.ts1)
        res = QV([-1., -1., -0., 0., 1., 1., 2.], unit='m',
                 timestamp=self.ts1)
        self.assertUnaryUfunc(ufunc, x, res)

        # Incompatible units
        wrong_res = QV([-1., -1., -0., 0., 1., 1., 2.], unit='s',
                       timestamp=self.ts1)
        with self.assertRaises(DimensionalityError):
            self.assertUnaryUfunc(ufunc, x, wrong_res)

    def tearDown(self):
        for f in self.np_array_func:
            print(f'{f} returns numpy.array!')


if __name__ == "__main__":
    main()
