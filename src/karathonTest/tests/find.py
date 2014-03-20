import unittest
from hash import Hash


class  FindTestCase(unittest.TestCase):

    def test_find_1(self):
        h = Hash("a.b.c1.d", 1)
        node = h.find("a.b.c1.d")
        if node is not None:
            self.assertEqual(node.getKey(), "d")
            self.assertEqual(node.getValue(), 1)
        else:
            self.assertEqual(True, False)
        node = h.find("a.b.c1.f")
        if node is not None:
            self.assertEqual(True, False)
        else:
            self.assertEqual(True, True)

    def test_find_2(self):
        h = Hash("a.b.c", "1")
        node = h.find("a.b.c")
        if node is not None:
            node.setValue(2)
            self.assertEqual(h.get("a.b.c"), 2);
        node = h.find("a.b.c", '/')
        if node is not None:
            self.assertEqual(True, False)
        else:
            self.assertEqual(True, True)

if __name__ == '__main__':
    unittest.main()

