import unittest
from hash import Hash

class  ConstructorsTestCase(unittest.TestCase):

    def test_empty(self):
        """ Check properties of empty Hash """
        h = Hash()
        self.assertEqual(len(h), 0)
        self.assertEqual(h.empty(), True)

    def test_one_property(self):
        """ Check Hash with one property """
        h = Hash('a', 1)
        self.assertEqual(len(h), 1)
        self.assertEqual(h.empty(), False)
        self.assertEqual(h.get('a'), 1)
        self.assertEqual(h['a'], 1)
            
    def test_two_properties(self):
        """ Check Hash with 2 properties """
        h = Hash('a', 1, 'b', 2.0)
        self.assertEqual(h.empty(), False)
        self.assertEqual(len(h), 2)
        self.assertEqual(h['a'], 1)
        self.assertEqual(h['b'], 2.0)
        
    def test_six_properties(self):
        """ Check Hash with 6 properties of different types """
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
        
if __name__ == '__main__':
    unittest.main()

