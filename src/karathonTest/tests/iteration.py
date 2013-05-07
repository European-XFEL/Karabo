import unittest
from libkarathon import *


class  IterationTestCase(unittest.TestCase):
    #def setUp(self):
    #    self.foo = Iteration()
    #

    #def tearDown(self):
    #    self.foo.dispose()
    #    self.foo = None

    def test_iteration(self):
        h = Hash("should", 1, "be", 2, "iterated", 3, "in", 4, "correct", 5, "order", 6)
        a = HashAttributes("should", 1, "be", 2, "iterated", 3, "in", 4, "correct", 5, "order", 6)
        try:
            insertionOrder = list()
            for k in h:
                insertionOrder.append(str(k))
            self.assertEqual(insertionOrder[0], "should");
            self.assertEqual(insertionOrder[1], "be");
            self.assertEqual(insertionOrder[2], "iterated");
            self.assertEqual(insertionOrder[3], "in");
            self.assertEqual(insertionOrder[4], "correct");
            self.assertEqual(insertionOrder[5], "order");
            
        except Exception,e:
            self.fail("test_iteration exception group 1: " + str(e))

        try:
            alphaNumericOrder = list()
            for k in h:
                alphaNumericOrder.append(k.getKey())
            alphaNumericOrder.sort()
            self.assertEqual(alphaNumericOrder[0], "be")
            self.assertEqual(alphaNumericOrder[1], "correct")
            self.assertEqual(alphaNumericOrder[2], "in")
            self.assertEqual(alphaNumericOrder[3], "iterated")
            self.assertEqual(alphaNumericOrder[4], "order")
            self.assertEqual(alphaNumericOrder[5], "should")
        except Exception,e:
            self.fail("test_iteration exception group 2: " + str(e))

        h.set("be", "2") # Has no effect on order

        try:
            insertionOrder = list()
            for k in h:
                insertionOrder.append(str(k.getKey()))
        
            self.assertEqual(insertionOrder[0], "should")
            self.assertEqual(insertionOrder[1], "be")
            self.assertEqual(insertionOrder[2], "iterated")
            self.assertEqual(insertionOrder[3], "in")
            self.assertEqual(insertionOrder[4], "correct")
            self.assertEqual(insertionOrder[5], "order")
        except Exception,e:
            self.fail("test_iteration exception group 3: " + str(e))

        try:
            alphaNumericOrder = list()
            for k in h:
                alphaNumericOrder.append(str(k.getKey()))
            alphaNumericOrder.sort()
            self.assertEqual(alphaNumericOrder[0], "be")
            self.assertEqual(alphaNumericOrder[1], "correct")
            self.assertEqual(alphaNumericOrder[2], "in")
            self.assertEqual(alphaNumericOrder[3], "iterated")
            self.assertEqual(alphaNumericOrder[4], "order")
            self.assertEqual(alphaNumericOrder[5], "should")
        except Exception,e:
            self.fail("test_iteration exception group 4: " + str(e))

        h.erase("be")  # Remove
        h.set("be", "2")   # Must be last element in sequence now

        try:
            insertionOrder = list()
            for k in h:
                insertionOrder.append(str(k.getKey()))
        
            self.assertEqual(insertionOrder[0], "should")
            self.assertEqual(insertionOrder[1], "iterated")
            self.assertEqual(insertionOrder[2], "in")
            self.assertEqual(insertionOrder[3], "correct")
            self.assertEqual(insertionOrder[4], "order")
            self.assertEqual(insertionOrder[5], "be")
        except Exception,e:
            self.fail("test_iteration exception group 5: " + str(e))

        try:
            alphaNumericOrder = list()
            for k in h:
                alphaNumericOrder.append(str(k.getKey()))
            alphaNumericOrder.sort()
            self.assertEqual(alphaNumericOrder[0], "be")
            self.assertEqual(alphaNumericOrder[1], "correct")
            self.assertEqual(alphaNumericOrder[2], "in")
            self.assertEqual(alphaNumericOrder[3], "iterated")
            self.assertEqual(alphaNumericOrder[4], "order")
            self.assertEqual(alphaNumericOrder[5], "should")
        except Exception,e:
            self.fail("test_iteration exception group 6: " + str(e))

        #  getKeys(...) to ...
        #         "list" and sort it  ... like C++ set
        try:
            tmp = list()           # create empty set
            h.getKeys(tmp)         # fill set by keys
            tmp.sort()
            it = iter(tmp)
            self.assertEqual(str(it.next()), "be")
            self.assertEqual(str(it.next()), "correct")
            self.assertEqual(str(it.next()), "in")
            self.assertEqual(str(it.next()), "iterated")
            self.assertEqual(str(it.next()), "order")
            self.assertEqual(str(it.next()), "should")
        except Exception,e:
            self.fail("test_iteration exception group 7: " + str(e))

        #        "list" ... like C++ vector
        try:
            tmp = list()           # create empty vector
            h.getKeys(tmp)         # fill vector by keys
            it = iter(tmp)
            self.assertEqual(str(it.next()), "should")
            self.assertEqual(str(it.next()), "iterated")
            self.assertEqual(str(it.next()), "in")
            self.assertEqual(str(it.next()), "correct")
            self.assertEqual(str(it.next()), "order")
            self.assertEqual(str(it.next()), "be")
        except Exception,e:
            self.fail("test_iteration exception group 8: " + str(e))

        try:
            h = Hash("b", "bla-la-la", "a.b.c", 1, "abc.2", 2.2222222, "a.b.c1.d", "abc1d", "abc.1", 1.11111111)
            
            l = [];
            h.getKeys(l)    # use top level keys
            self.assertEqual(len(l), 3)
            i = iter(l)                         # "canonical" order: on every level insertion order
            self.assertEqual(str(i.next()), "b")
            self.assertEqual(str(i.next()), "a")
            self.assertEqual(str(i.next()), "abc")
            
            l = [];
            h.getPaths(l)   # use full keys 
            self.assertEqual(len(l), 5)
            i = iter(l)
            self.assertEqual(str(i.next()), "b")
            self.assertEqual(str(i.next()), "a.b.c")
            self.assertEqual(str(i.next()), "a.b.c1.d")
            self.assertEqual(str(i.next()), "abc.2")
            self.assertEqual(str(i.next()), "abc.1")
        except Exception,e:
            self.fail("test_iteration exception group 9: " + str(e))

if __name__ == '__main__':
    unittest.main()

