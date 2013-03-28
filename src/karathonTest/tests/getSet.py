import unittest
from libkarathon import *


class  GetSetTestCase(unittest.TestCase):

    def test_getSet(self):
        
        try:
            h = Hash()
            h.set("a.b.c1.d", 1)
            self.assertEqual(h.get("a").has("b"), True, '"b" not found')
            self.assertEqual(h.get("a.b").has("c1"), True, '"c1" not found')
            self.assertEqual(h.get("a.b.c1").has("d"), True, '"d" not found')
            self.assertEqual(h.get("a.b.c1.d"), 1, '"get" should return 1')
            self.assertEqual(h.has("a.b.c1.d"), True, '"a.b.c1.d" key not found')
            self.assertEqual(h.get("a").has("b.c1"), True, '"b.c1" key not found')
        
            h.set("a.b.c2.d", 2.0)
            self.assertEqual(h.get("a.b").has("c2.d"), True, '"c2.d" not found')
            self.assertEqual(h.get("a.b").has("c1.d"), True, '"c1.d" not found')
            self.assertEqual(h.get("a.b.c1.d"), 1, '"get" should return 1')
            self.assertEqual(h.get("a.b.c2.d"), 2.0, '"get" should return 2.0')
            
            h.set("a.b[0]", Hash("a", 1))
            self.assertEqual(h.get("a").has("b"), True, "'b' not found")
            self.assertEqual(h["a"].has("b"), True, "'b' not found")
            self.assertEqual(len(h.get("a")), 1, "'len' should give 1")
            self.assertEqual(len(h["a"]), 1, "'len' should give 1")
            self.assertEqual(len(h.get("a.b")), 1, "'len' should give 1")
            self.assertEqual(len(h["a.b"]), 1, "'len' should give 1")
            self.assertEqual(len(h.get("a.b")[0]), 1, "'len' should give 1")
            self.assertEqual(len(h["a.b"][0]), 1, "'len' should give 1")
            self.assertEqual(h["a.b"][0]["a"], 1, '"get" should return 1')
            self.assertEqual(h["a.b[0].a"], 1, '"get" should return 1')
            self.assertEqual(h.get("a.b")[0].get("a"), 1, '"get" should return 1')
            self.assertEqual(h.get("a.b[0].a"), 1, '"get" should return 1')
            
            h.set("a.b[2]", Hash("a", 1))
            self.assertEqual(h.get("a").has("b"), True, "'b' not found")
            self.assertEqual(len(h["a"]), 1, "'len' should give 1")
            self.assertEqual(len(h["a.b"]), 3, "'len' should give 3")   # 0, 1, 2
            self.assertEqual(h["a.b[0].a"], 1, '"get" should return 1')
            self.assertEqual(h["a.b[2].a"], 1, '"get" should return 1')
            self.assertEqual(h["a.b"][0]["a"], 1, '"get" should return 1')
            self.assertEqual(h["a.b"][2]["a"], 1, '"get" should return 1')
            self.assertEqual(h["a.b"][1].empty(), True, 'h["a.b"][1] should be empty Hash')
            
        except Exception,e:
            self.fail("test_getSet exception group 1: " + str(e))
            
        try:
            h = Hash()
            h["a.b.c"] = 1    # statement is equivalent to h.set("a.b.c", 1)
            h["a.b.c"] = 2
            self.assertEqual(h["a.b.c"], 2, "Value should be overwritten by 2")
            self.assertEqual(h.has("a.b"), True, "Key 'a.b' not found")
            self.assertEqual(h.has("a.b.c.d"), False, "Key 'a.b.c.d' should not be found")
        except Exception,e:
            self.fail("test_getSet exception group 2: " + str(e))
            
        try:
            h = Hash("a[0]", Hash("a", 1), "a[1]", Hash("a", 1))
            self.assertEqual(h["a[0].a"], 1, "Value should be 1")
            self.assertEqual(h["a[1].a"], 1, "Value should be 1")
            self.assertEqual(h["a"][0]["a"], 1, "Value should be 1")
            self.assertEqual(h["a"][1]["a"], 1, "Value should be 1")
        except Exception,e:
            self.fail("test_getSet exception group 3: " + str(e))
            
        try:
            h = Hash()
            h["x[0].y[0]"] = Hash("a", 4.2, "b", "red", "c", True)
            h["x[1].y[0]"] = Hash("a", 4.0, "b", "green", "c", False)
            self.assertEqual(h["x[0].y[0].c"], True, "Failure in array case")
            self.assertEqual(h["x[1].y[0].c"], False, "Failure in array case")
            self.assertEqual(h["x[0].y[0].b"], "red", "Failure in array case")
            self.assertEqual(h["x[1].y[0].b"], "green", "Failure in array case")
        except Exception,e:
            self.fail("test_getSet exception group 4: " + str(e))
            
        try:
            h1 = Hash("a[0].b[0]", Hash("a", 1))
            h2 = Hash("a[0].b[0]", Hash("a", 2))
            self.assertEqual(h1["a[0].b[0].a"], 1, "Value should be equal 1")
            h1["a[0]"] = h2
            self.assertEqual(h1["a[0].a[0].b[0].a"], 2, "Value should be equal 2")
            h1["a"] = h2
            self.assertEqual(h1["a.a[0].b[0].a"], 2, "Value should be equal 2")
        except Exception,e:
            self.fail("test_getSet exception group 5: " + str(e))
            
        try:
            h = Hash()
            b = True
            h["a"] = b
            self.assertEqual(h.getType("a"), "BOOL", 'The type should be "BOOL"')
            self.assertEqual(h.getTypeAsId("a"), 0, 'The type ID for "BOOL" should be 0')
        except Exception,e:
            self.fail("test_getSet exception group 6: " + str(e))
            
            
if __name__ == '__main__':
    unittest.main()

