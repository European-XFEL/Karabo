import unittest
from hash import Hash


class  GetSetTestCase(unittest.TestCase):

    def test_getSet_1(self):
        h = Hash()
        h.set("a.b.c1.d", 1)
        self.assertEqual(h.get("a").has("b"), True)
        self.assertEqual(h.get("a.b").has("c1"), True)
        self.assertEqual(h.get("a.b.c1").has("d"), True)
        self.assertEqual(h.get("a.b.c1.d"), 1)
        self.assertEqual(h.has("a.b.c1.d"), True)
        if "a.b.c1.d" not in h:
            self.fail("test_getSet group 1: h __contains__ \"a.b.c1.d\" failed")
        self.assertEqual(h.get("a").has("b.c1"), True)
        if "b.c1" not in h["a"]:
            self.fail("test_getSet group 1: h['a'] __contains__ \"b.c1\" failed")

        h.set("a.b.c2.d", 2.0)
        self.assertEqual(h.get("a.b").has("c2.d"), True)
        self.assertEqual(h.get("a.b").has("c1.d"), True)
        self.assertEqual(h.get("a.b.c1.d"), 1)
        self.assertEqual(h.get("a.b.c2.d"), 2.0)
        
        h.set("a.b[0]", Hash("a", 1))
        self.assertEqual(h.get("a").has("b"), True)
        self.assertEqual(h["a"].has("b"), True)
        if "b" not in h["a"]:
            self.fail("test_getSet group 1: h['a'] __contains__ \"b\" failed")
        self.assertEqual(len(h.get("a")), 1)
        self.assertEqual(len(h["a"]), 1)
        self.assertEqual(len(h.get("a.b")), 1)
        self.assertEqual(len(h["a.b"]), 1)
        self.assertEqual(len(h.get("a.b")[0]), 1)
        self.assertEqual(len(h["a.b"][0]), 1)
        self.assertEqual(h["a.b"][0]["a"], 1)
        self.assertEqual(h["a.b[0].a"], 1)
        self.assertEqual(h.get("a.b")[0].get("a"), 1)
        self.assertEqual(h.get("a.b[0].a"), 1)
        
        h.set("a.b[2]", Hash("a", 1))
        self.assertEqual(h.get("a").has("b"), True)
        self.assertEqual(len(h["a"]), 1)
        self.assertEqual(len(h["a.b"]), 3)   # 0, 1, 2
        self.assertEqual(h["a.b[0].a"], 1)
        self.assertEqual(h["a.b[2].a"], 1)
        self.assertEqual(h["a.b"][0]["a"], 1)
        self.assertEqual(h["a.b"][2]["a"], 1)
        self.assertEqual(h["a.b"][1].empty(), True)

    def test_getSet_2(self):
        h = Hash()
        h["a.b.c"] = 1    # statement is equivalent to h.set("a.b.c", 1)
        h["a.b.c"] = 2
        self.assertEqual(h["a.b.c"], 2)
        self.assertEqual(h.has("a.b"), True)
        self.assertEqual(h.has("a.b.c.d"), False)
            
    def test_getSet_3(self):
        h = Hash("a[0]", Hash("a", 1), "a[1]", Hash("a", 1))
        self.assertEqual(h["a[0].a"], 1)
        self.assertEqual(h["a[1].a"], 1)
        self.assertEqual(h["a"][0]["a"], 1)
        self.assertEqual(h["a"][1]["a"], 1)
            
    def test_getSet_4(self):
        h = Hash()
        h["x[0].y[0]"] = Hash("a", 4.2, "b", "red", "c", True)
        h["x[1].y[0]"] = Hash("a", 4.0, "b", "green", "c", False)
        self.assertEqual(h["x[0].y[0].c"], True)
        self.assertEqual(h["x[1].y[0].c"], False)
        self.assertEqual(h["x[0].y[0].b"], "red")
        self.assertEqual(h["x[1].y[0].b"], "green")
            
    def test_getSet_4(self):
        h1 = Hash("a[0].b[0]", Hash("a", 1))
        h2 = Hash("a[0].b[0]", Hash("a", 2))
        self.assertEqual(h1["a[0].b[0].a"], 1)
        h1["a[0]"] = h2
        self.assertEqual(h1["a[0].a[0].b[0].a"], 2)
        h1["a"] = h2
        self.assertEqual(h1["a.a[0].b[0].a"], 2)
            
    def test_getSet_5(self):
        h = Hash()
        b = True
        h["a"] = b
        self.assertEqual(str(h.getType("a")), "BOOL")
        self.assertEqual(h.getType("a"), Types.BOOL)
            
    def test_getSet_5(self):
        h = Hash('a.b.c', [1, 2, 3, 4, 5, 6, 7], 'b.c.d', [False, False, True, True, True, False, True])
        self.assertEqual(isStdVectorDefaultConversion(Types.PYTHON), True)
        self.assertEqual(h.isType('a.b.c', Types.VECTOR_INT32), True)
        self.assertEqual(str(type(h['a.b.c'])), "<type 'list'>")
        try:
            setStdVectorDefaultConversion(Types.VECTOR_INT32)
        except RuntimeError,e:
            pass
        self.assertEqual(isStdVectorDefaultConversion(Types.PYTHON), True)
        setStdVectorDefaultConversion(Types.NUMPY)
        self.assertEqual(isStdVectorDefaultConversion(Types.PYTHON), False)
        self.assertEqual(isStdVectorDefaultConversion(Types.NUMPY), True)
        self.assertEqual(str(type(h['a.b.c'])), "<type 'numpy.ndarray'>")
        self.assertEqual(str(type(h['b.c.d'])), "<type 'numpy.ndarray'>")
        setStdVectorDefaultConversion(Types.PYTHON)
        self.assertEqual(isStdVectorDefaultConversion(Types.PYTHON), True)
            
            
if __name__ == '__main__':
    unittest.main()

