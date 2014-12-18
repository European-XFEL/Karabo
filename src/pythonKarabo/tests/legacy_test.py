""" Test that legacy devices can still be imported when using the new API """
import sys
sys.karabo_api = 2

import numpy
import unittest
import numpy.testing

import configuration_test_classes as ctc
from karabo.configurator import Configurator
from karabo.hash import Hash
from karabo.schema import Schema

class LegacyTest(unittest.TestCase):
    def setUp(self):
        with open("legacy.xml", "r") as fin:
            self.prep = Hash.decode(fin.read(), "XML")
        self.addTypeEqualityFunc(numpy.ndarray,
                                 numpy.testing.assert_array_equal)

    def assertHashEqual(self, x, y):
        self.assertEqual(x.keys(), y.keys())
        for k, v, a in x.iterall():
            try:
                if isinstance(v, Hash):
                    self.assertHashEqual(v, y[k])
                else:
                    self.assertEqual(v, y[k])

                #self.assertDictEqual(a, y[k, ...])
                self.assertEqual(a.keys(), y[k, ...].keys())
                for kk, vv in a.items():
                    if isinstance(vv, numpy.ndarray) and vv.ndim > 0:
                        self.assertTrue((vv == y[k, kk]).all())
                    else:
                        self.assertEqual(vv, y[k, kk])
            except Exception as e:
                print("in ", k)
                raise

    def test_shape(self):
        s = Schema("bla", None)
        ctc.Shape.expectedParameters(s)
        h = s.hash
        self.assertEqual(h["shadowEnabled", "displayedName"], "Shadow")
        self.assertEqual(h["shadowEnabled", "valueType"], "BOOL")
        self.assertEqual(h["shadowEnabled", "assignment"], 0)

    def test_all(self):
        for k, v in self.prep.items():
            try:
                c = getattr(ctc, k)
                if hasattr(c, "__base_classid__"):
                    s = Configurator(c.__base_classid__).getSchema(c)
                else:
                    s = Schema("bla", None)
                    c.expectedParameters(s)
                self.assertHashEqual(v.hash, s.hash)
            except Exception:
                print("in class", k)
                raise

if __name__ == "__main__":
    unittest.main()
