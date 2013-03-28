import unittest
from libkarathon import *

class  ConstructorsTestCase(unittest.TestCase):

    def test_constructors(self):
        
        # Check properties of empty Hash
        try:
            h = Hash()
            self.assertEqual(len(h), 0)
            self.assertEqual(h.empty(), True)
        except Exception, e:
            self.fail("test_constructors exception group 1: " + str(e))

        # Check Hash with one property
        try:
            h = Hash('a', 1)
            self.assertEqual(len(h), 1)
            self.assertEqual(h.empty(), False)
            self.assertEqual(h.get('a'), 1)
            self.assertEqual(h['a'], 1)
        except Exception,e:
            self.fail("test_constructors exception group 2: " + str(e))
            
        # Check Hash with 2 properties
        try:
            h = Hash('a', 1, 'b', 2.0)
            self.assertEqual(h.empty(), False)
            self.assertEqual(len(h), 2)
            self.assertEqual(h['a'], 1)
            self.assertEqual(h['b'], 2.0)
        except Exception,e:
            self.fail("test_constructors exception group 3: " + str(e))
        
        # Check Hash with 6 properties of different types
        try:
            h = Hash("a.b.c", 1, "b.c", 2.0, "c", 3.7, "d.e", "4",
                    "e.f.g.h", [5,5,5,5,5], "F.f.f.f.f", Hash("x.y.z", 99))
            self.assertEqual(h.empty(), False)
            self.assertEqual(len(h), 6)
            self.assertEqual(h['a.b.c'], 1)
            self.assertEqual(h['b.c'], 2.0)
            self.assertEqual(h['c'], 3.7)
            self.assertEqual(h['d.e'], "4")
            self.assertEqual(h['e.f.g.h'][0], 5)
            self.assertEqual(len(h['e.f.g.h']), 5)
            self.assertEqual(h['F.f.f.f.f']['x.y.z'], 99)
            self.assertEqual(h['F.f.f.f.f.x.y.z'], 99)
            # Make Hash flat
            flat = Hash()
            Hash.flatten(h, flat)
            self.assertEqual(flat.empty(), False)
            self.assertEqual(len(flat), 6)
            self.assertEqual(flat.get('a.b.c', ' '), 1)
            self.assertEqual(flat.get('b.c', ' '), 2.0)
            self.assertEqual(flat.get('c', ' '), 3.7)
            self.assertEqual(flat.get('d.e', ' '), "4")
            self.assertEqual(flat.get('e.f.g.h', ' ')[0], 5)
            self.assertEqual(len(flat.get('e.f.g.h', ' ')), 5)
            self.assertEqual(flat.get('F.f.f.f.f.x.y.z', ' '), 99)
            
            # Make flat Hash unflatten again
            tree = Hash()
            flat.unflatten(tree)
            self.assertEqual(tree.empty(), False)
            self.assertEqual(len(tree), 6)
            self.assertEqual(tree['a.b.c'], 1)
            self.assertEqual(tree['b.c'], 2.0)
            self.assertEqual(tree['c'], 3.7)
            self.assertEqual(tree['d.e'], "4")
            self.assertEqual(tree['e.f.g.h'][0], 5)
            self.assertEqual(len(tree['e.f.g.h']), 5)
            self.assertEqual(tree['F.f.f.f.f']['x.y.z'], 99)
            self.assertEqual(tree['F.f.f.f.f.x.y.z'], 99)
        except Exception,e:
            self.fail("test_constructors exception group 4: " + str(e))
        

if __name__ == '__main__':
    unittest.main()

