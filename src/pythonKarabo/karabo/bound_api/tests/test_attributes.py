import unittest

import karabind

import karathon


class Tests(unittest.TestCase):

    def test_constructor(self):
        def inner(Attributes, Node):
            # Empty hash attributes
            ha = Attributes()
            self.assertEqual(len(ha), 0)
            self.assertTrue(ha.empty())
            # Only default HashAttributes constructor allowed in karabind
            with self.assertRaises(TypeError):
                ha = karabind.HashAttributes('a', 1)
            # Contruction of HashAttributesNode is not possible in python
            with self.assertRaises(TypeError):
                ha = karabind.HashAttributesNode()
            with self.assertRaises(RuntimeError):
                ha = karathon.HashAttributesNode()

        # Run test in karabind version
        inner(karabind.HashAttributes, karabind.HashAttributesNode)
        # Run test in karathon version
        inner(karathon.HashAttributes, karathon.HashAttributesNode)

    def test_set(self):
        def inner(Attributes, Node, Types):
            # Set attribute
            ha = Attributes()
            ha.set('attr1', 12)
            ha.set('attr2', 'abrakadabra')
            ha.set('attr3', 3.14159265359)

            ha['attr4'] = 2.718281828
            ha['attr5'] = 42
            ha['attr6'] = 'some string'
            ha['attr7'] = True
            ha['attr8'] = [1, 2, 3, 4, 5]

            self.assertEqual(len(ha), 8)
            self.assertFalse(ha.empty())
            self.assertEqual(ha.get('attr1'), 12)
            self.assertEqual(ha['attr2'], 'abrakadabra')
            self.assertEqual(ha['attr4'], 2.718281828)

            # Get reference to node
            node = ha.getNode('attr6')
            self.assertIsInstance(node, Node)
            self.assertEqual(node.getKey(), 'attr6')
            self.assertEqual(node.getType(), Types.STRING)
            self.assertEqual(node.getValue(), 'some string')

            # Implemented only in karabind
            if Attributes is karabind.HashAttributes:
                self.assertTrue(ha.isType('attr4', Types.DOUBLE))
                self.assertTrue(ha.isType('attr4', "DOUBLE"))
                self.assertEqual(ha.getType('attr4'), Types.DOUBLE)

                self.assertTrue(ha.isType('attr8', Types.VECTOR_INT32))
                self.assertEqual(ha.getType('attr8'), Types.VECTOR_INT32)

        # Run test in karabind version
        inner(karabind.HashAttributes, karabind.HashAttributesNode,
              karabind.Types)
        # Run test in karathon version
        inner(karathon.HashAttributes, karathon.HashAttributesNode,
              karathon.Types)

    def test_iteration(self):
        def inner(Attributes, Node, Types):
            ha = Attributes()
            ha['a'] = 8100000000000000
            ha['b'] = True
            ha['c'] = 2.333333333
            ha['s'] = 'Hello'

            it = iter(ha)
            n = next(it)
            self.assertIsInstance(n, Node)
            self.assertEqual(n.getKey(), 'a')
            self.assertEqual(n.getType(), Types.INT64)
            self.assertEqual(n.getValue(), 8100000000000000)
            self.assertEqual(n.getValueAs(Types.STRING), '8100000000000000')

            n = next(it)
            self.assertIsInstance(n, Node)
            self.assertEqual(n.getKey(), 'b')
            self.assertEqual(n.getType(), Types.BOOL)
            self.assertEqual(n.getValue(), True)

            n = next(it)
            self.assertEqual(n.getKey(), 'c')
            self.assertEqual(n.getType(), Types.DOUBLE)

            n = next(it)
            self.assertEqual(n.getKey(), 's')
            self.assertEqual(n.getType(), Types.STRING)
            self.assertEqual(n.getValue(), 'Hello')

            with self.assertRaises(StopIteration):
                n = next(it)

        # Run test in karabind version
        inner(karabind.HashAttributes, karabind.HashAttributesNode,
              karabind.Types)
        # Run test in karathon version
        inner(karathon.HashAttributes, karathon.HashAttributesNode,
              karathon.Types)
