import unittest
from hash import Hash


class  MergeTestCase(unittest.TestCase):

    def test_merge(self):
        h1 = Hash("a", 1,
                  "b", 2,
                  "c.b[0].g", 3,
                  "c.c[0].d", 4,
                  "c.c[1]", Hash("a.b.c", 6),
                  "d.e", 7
                 )

        h2 = Hash("a", 21,
                  "b.c", 22,
                  "c.b[0]", Hash("key", "value"),
                  "c.b[1].d", 24,
                  "e", 27
                 )

        h1 += h2

        self.assertEqual(h1.has("a"), True)
        self.assertEqual(h1.get("a"), 21)
        self.assertEqual(h1["a"], 21)
        self.assertEqual(h1.has("b"), True)
        self.assertEqual(h1.has("c.b.d"), False)
        self.assertEqual(h1.has("c.b[0]"), True)
        self.assertEqual(h1.has("c.b[1]"), True)
        self.assertEqual(h1.has("c.b[2]"), True)
        self.assertEqual(h1.get("c.b[2].d"), 24)
        self.assertEqual(h1.has("c.c[0].d"), True)
        self.assertEqual(h1.has("c.c[1].a.b.c"), True)
        self.assertEqual(h1.has("d.e"), True)
        self.assertEqual(h1.has("e"), True)

        h3 = h1

        self.assertEqual(similar(h1, h3), True)

if __name__ == '__main__':
    unittest.main()

