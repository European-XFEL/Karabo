import unittest
from libkarathon import *


class  FindTestCase(unittest.TestCase):

    def test_find(self):
        try:
            h = Hash("a.b.c1.d", 1)
            node = h.find("a.b.c1.d")
            if node is not None:
                self.assertEqual(node.getKey(), "d", 'Bad key returned by "getKey" method')
                self.assertEqual(node.getValue(), 1, 'Should return 1');
            else:
                self.assertEqual(True, False)
            node = h.find("a.b.c1.f")
            if node is not None:
                self.assertEqual(True, False)
            else:
                self.assertEqual(True, True)
        except Exception,e:
            self.fail("test_find exception group 1: " + str(e))
    

        try:
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
        except Exception,e:
            self.fail("test_find exception group 2: " + str(e))
        

if __name__ == '__main__':
    unittest.main()

