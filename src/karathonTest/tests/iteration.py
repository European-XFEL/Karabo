import unittest
from hash import Hash


class  IterationTestCase(unittest.TestCase):
    def setUp(self):
        self.h = Hash("should", 1, "be", 2, "iterated", 3, "in", 4,
                      "correct", 5, "order", 6)
        #self.a = HashAttributes("should", 1, "be", 2, "iterated", 3, "in", 4,
        #                       "correct", 5, "order", 6)


    def test_iteration_1(self):
        insertionOrder = [ ]
        for k in self.h:
            insertionOrder.append(str(k))
        self.assertEqual(insertionOrder[0], "should");
        self.assertEqual(insertionOrder[1], "be");
        self.assertEqual(insertionOrder[2], "iterated");
        self.assertEqual(insertionOrder[3], "in");
        self.assertEqual(insertionOrder[4], "correct");
        self.assertEqual(insertionOrder[5], "order");
            

    def test_iteration_2(self):
        alphaNumericOrder = list()
        for k in self.h:
            alphaNumericOrder.append(k.getKey())
        alphaNumericOrder.sort()
        self.assertEqual(alphaNumericOrder[0], "be")
        self.assertEqual(alphaNumericOrder[1], "correct")
        self.assertEqual(alphaNumericOrder[2], "in")
        self.assertEqual(alphaNumericOrder[3], "iterated")
        self.assertEqual(alphaNumericOrder[4], "order")
        self.assertEqual(alphaNumericOrder[5], "should")

    def test_iteration_3(self):
        h = Hash(self.h)
        h.set("be", "2") # Has no effect on order

        insertionOrder = list()
        for k in h:
            insertionOrder.append(str(k.getKey()))

        self.assertEqual(insertionOrder[0], "should")
        self.assertEqual(insertionOrder[1], "be")
        self.assertEqual(insertionOrder[2], "iterated")
        self.assertEqual(insertionOrder[3], "in")
        self.assertEqual(insertionOrder[4], "correct")
        self.assertEqual(insertionOrder[5], "order")

    def test_iteration_3(self):
        alphaNumericOrder = list()
        for k in self.h:
            alphaNumericOrder.append(str(k.getKey()))
        alphaNumericOrder.sort()
        self.assertEqual(alphaNumericOrder[0], "be")
        self.assertEqual(alphaNumericOrder[1], "correct")
        self.assertEqual(alphaNumericOrder[2], "in")
        self.assertEqual(alphaNumericOrder[3], "iterated")
        self.assertEqual(alphaNumericOrder[4], "order")
        self.assertEqual(alphaNumericOrder[5], "should")


    def test_iteration_4(self):
        h = Hash(self.h)
        h.erase("be")  # Remove
        h.set("be", "2")   # Must be last element in sequence now

        insertionOrder = list()
        for k in h:
            insertionOrder.append(str(k.getKey()))

        self.assertEqual(insertionOrder[0], "should")
        self.assertEqual(insertionOrder[1], "iterated")
        self.assertEqual(insertionOrder[2], "in")
        self.assertEqual(insertionOrder[3], "correct")
        self.assertEqual(insertionOrder[4], "order")
        self.assertEqual(insertionOrder[5], "be")

    def test_iteration_5(self):
        alphaNumericOrder = list()
        for k in self.h:
            alphaNumericOrder.append(str(k.getKey()))
        alphaNumericOrder.sort()
        self.assertEqual(alphaNumericOrder[0], "be")
        self.assertEqual(alphaNumericOrder[1], "correct")
        self.assertEqual(alphaNumericOrder[2], "in")
        self.assertEqual(alphaNumericOrder[3], "iterated")
        self.assertEqual(alphaNumericOrder[4], "order")
        self.assertEqual(alphaNumericOrder[5], "should")

    def test_iteration_6(self):
        tmp = list()           # create empty set
        self.h.getKeys(tmp)    # fill set by keys
        tmp.sort()
        it = iter(tmp)
        self.assertEqual(str(it.next()), "be")
        self.assertEqual(str(it.next()), "correct")
        self.assertEqual(str(it.next()), "in")
        self.assertEqual(str(it.next()), "iterated")
        self.assertEqual(str(it.next()), "order")
        self.assertEqual(str(it.next()), "should")

    def test_iteration_7(self):
        tmp = list()           # create empty vector
        self.h.getKeys(tmp)    # fill vector by keys
        it = iter(tmp)
        self.assertEqual(str(it.next()), "should")
        self.assertEqual(str(it.next()), "be")
        self.assertEqual(str(it.next()), "iterated")
        self.assertEqual(str(it.next()), "in")
        self.assertEqual(str(it.next()), "correct")
        self.assertEqual(str(it.next()), "order")

    def test_iteration_8(self):
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

if __name__ == '__main__':
    unittest.main()

