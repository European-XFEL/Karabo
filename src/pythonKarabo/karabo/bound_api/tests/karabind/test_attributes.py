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
            # BOOL
            ha.set('attr1', False)
            self.assertEqual(ha.getNode('attr1').getType(), Types.BOOL)
            self.assertFalse(ha.get('attr1'))
            # INT32
            ha.set('attr2', 12)
            self.assertEqual(ha.getNode('attr2').getType(), Types.INT32)
            self.assertEqual(ha.get('attr2'), 12)
            # INT64
            ha.set('attr3', 333333333333)
            self.assertEqual(ha.getNode('attr3').getType(), Types.INT64)
            self.assertEqual(ha.get('attr3'), 333333333333)
            # DOUBLE
            ha.set('attr4', 3.14159265359)
            self.assertEqual(ha.getNode('attr4').getType(), Types.DOUBLE)
            self.assertEqual(ha.get('attr4'), 3.14159265359)
            # COMPLEX_DOUBLE
            ha.set('attr5', complex(12., 42.))
            self.assertEqual(
                ha.getNode('attr5').getType(), Types.COMPLEX_DOUBLE)
            self.assertEqual(ha.get('attr5'), complex(12., 42.))
            # STRING
            ha.set('attr6', 'abrakadabra')
            self.assertEqual(ha.getNode('attr6').getType(), Types.STRING)
            self.assertEqual(ha.get('attr6'), 'abrakadabra')

            # BOOL
            ha['attr7'] = True
            self.assertEqual(ha.getNode('attr7').getType(), Types.BOOL)
            self.assertTrue(ha['attr7'])
            # INT32
            ha['attr8'] = 42
            self.assertEqual(ha.getNode('attr8').getType(), Types.INT32)
            self.assertEqual(ha['attr8'], 42)
            # INT64
            ha['attr9'] = -123456789012
            self.assertEqual(ha.getNode('attr9').getType(), Types.INT64)
            self.assertEqual(ha['attr9'], -123456789012)
            # DOUBLE
            ha['attr10'] = 2.718281828
            self.assertEqual(ha.getNode('attr10').getType(), Types.DOUBLE)
            self.assertEqual(ha['attr10'], 2.718281828)
            # COMPLEX_DOUBLE
            ha['attr11'] = complex(9999.12, -65432.03)
            self.assertEqual(
                ha.getNode('attr11').getType(), Types.COMPLEX_DOUBLE)
            # STRING
            ha['attr12'] = 'some string'
            self.assertEqual(ha.getNode('attr12').getType(), Types.STRING)
            # VECTOR_BOOL
            ha['attr13'] = [True, False, False, True, True]
            self.assertEqual(
                ha.getNode('attr13').getType(), Types.VECTOR_BOOL)
            # VECTOR_INT32
            ha['attr14'] = [1, 2, 3, 4, 5]
            self.assertEqual(
                ha.getNode('attr14').getType(), Types.VECTOR_INT32)
            # VECTOR_INT64
            ha['attr15'] = [12, 13, 1444444444444, -2, 0]
            self.assertEqual(
                ha.getNode('attr15').getType(), Types.VECTOR_INT64)
            # VECTOR_DOUBLE
            ha['attr16'] = [1.11111, 2.222222, 3.333333]
            self.assertEqual(ha.getNode('attr16').getType(),
                             Types.VECTOR_DOUBLE)
            # VECTOR_COMPLEX_DOUBLE
            if Attributes is karabind.HashAttributes:
                ha['attr17'] = [complex(1.0, -5), complex(-2., 188)]
                self.assertEqual(ha.getNode('attr17').getType(),
                                 Types.VECTOR_COMPLEX_DOUBLE)
                self.assertEqual(ha.getType('attr17'),
                                 Types.VECTOR_COMPLEX_DOUBLE)
                self.assertTrue(
                    ha.isType('attr17', Types.VECTOR_COMPLEX_DOUBLE))
                self.assertTrue(ha.isType('attr17', "VECTOR_COMPLEX_DOUBLE"))
                ha.erase('attr17')
                ha['attr17'] = [complex(1.0, -5), complex(-2., 188)]
                del(ha['attr17'])

            # VECTOR_STRING
            ha['attr18'] = ['aaaa', 'bbb', 'cccccccc', 'www']
            self.assertEqual(
                ha.getNode('attr18').getType(), Types.VECTOR_STRING)
            self.assertEqual(len(ha), 17)
            self.assertTrue('attr18' in ha)
            self.assertFalse('attr17' in ha)
            self.assertFalse(ha.empty())
            # Get reference to node
            node6 = ha.getNode('attr6')
            self.assertIsInstance(node6, Node)
            self.assertEqual(node6.getKey(), 'attr6')
            self.assertEqual(node6.getType(), Types.STRING)
            self.assertEqual(node6.getValue(), 'abrakadabra')
            # Implemented only in karabind: check attribute type
            if Attributes is karabind.HashAttributes:
                self.assertEqual(ha.getType('attr1'), Types.BOOL)
                self.assertTrue(ha.isType('attr1', Types.BOOL))
                self.assertTrue(ha.isType('attr1', "BOOL"))
                self.assertEqual(ha.getType('attr2'), Types.INT32)
                self.assertTrue(ha.isType('attr2', Types.INT32))
                self.assertTrue(ha.isType('attr2', "INT32"))
                self.assertEqual(ha.getType('attr3'), Types.INT64)
                self.assertTrue(ha.isType('attr3', Types.INT64))
                self.assertTrue(ha.isType('attr3', "INT64"))
                self.assertEqual(ha.getType('attr4'), Types.DOUBLE)
                self.assertTrue(ha.isType('attr4', Types.DOUBLE))
                self.assertTrue(ha.isType('attr4', "DOUBLE"))
                self.assertEqual(ha.getType('attr5'), Types.COMPLEX_DOUBLE)
                self.assertTrue(ha.isType('attr5', Types.COMPLEX_DOUBLE))
                self.assertTrue(ha.isType('attr5', "COMPLEX_DOUBLE"))
                self.assertEqual(ha.getType('attr6'), Types.STRING)
                self.assertTrue(ha.isType('attr6', Types.STRING))
                self.assertTrue(ha.isType('attr6', "STRING"))
                self.assertEqual(ha.getType('attr13'), Types.VECTOR_BOOL)
                self.assertTrue(ha.isType('attr13', Types.VECTOR_BOOL))
                self.assertTrue(ha.isType('attr13', "VECTOR_BOOL"))
                self.assertEqual(ha.getType('attr14'), Types.VECTOR_INT32)
                self.assertTrue(ha.isType('attr14', Types.VECTOR_INT32))
                self.assertTrue(ha.isType('attr14', "VECTOR_INT32"))
                self.assertEqual(ha.getType('attr15'), Types.VECTOR_INT64)
                self.assertTrue(ha.isType('attr15', Types.VECTOR_INT64))
                self.assertTrue(ha.isType('attr15', "VECTOR_INT64"))
                self.assertEqual(ha.getType('attr16'), Types.VECTOR_DOUBLE)
                self.assertTrue(ha.isType('attr16', Types.VECTOR_DOUBLE))
                self.assertTrue(ha.isType('attr16', "VECTOR_DOUBLE"))
                self.assertEqual(ha.getType('attr18'), Types.VECTOR_STRING)
                self.assertTrue(ha.isType('attr18', Types.VECTOR_STRING))
                self.assertTrue(ha.isType('attr18', "VECTOR_STRING"))
                self.assertFalse(ha.isType('attr15', "INT64"))

            if Attributes is karathon.HashAttributes:
                self.assertEqual(ha.getNode('attr1').getType(), Types.BOOL)
                self.assertEqual(ha.getNode('attr2').getType(), Types.INT32)
                self.assertEqual(ha.getNode('attr3').getType(), Types.INT64)
                self.assertEqual(ha.getNode('attr4').getType(), Types.DOUBLE)
                self.assertEqual(ha.getNode('attr5').getType(),
                                 Types.COMPLEX_DOUBLE)
                self.assertEqual(ha.getNode('attr6').getType(), Types.STRING)

                self.assertEqual(ha.getNode('attr13').getType(),
                                 Types.VECTOR_BOOL)
                self.assertEqual(ha.getNode('attr14').getType(),
                                 Types.VECTOR_INT32)
                self.assertEqual(ha.getNode('attr15').getType(),
                                 Types.VECTOR_INT64)
                self.assertEqual(ha.getNode('attr16').getType(),
                                 Types.VECTOR_DOUBLE)
                self.assertEqual(ha.getNode('attr18').getType(),
                                 Types.VECTOR_STRING)

            ha.clear()
            self.assertTrue(ha.empty())

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

    def test_changetype(self):
        def inner(Attributes, Node, Types):
            ha = Attributes()
            ha['a'] = 65

            def current_type_is(reftype):
                if Attributes is karabind.HashAttributes:
                    self.assertEqual(ha.getType('a'), reftype)
                if Attributes is karathon.HashAttributes:
                    self.assertEqual(ha.getNode('a').getType(), reftype)

            current_type_is(Types.INT32)
            # Get as INT64
            self.assertEqual(ha.getAs('a', "INT64"), 65)
            # Get as INT16
            self.assertEqual(ha.getAs('a', "INT16"), 65)
            # Get as INT8
            self.assertEqual(ha.getAs('a', "INT8"), 65)
            # Get as DOUBLE
            self.assertEqual(ha.getAs('a', "DOUBLE"), 65.0)
            # Attempt to get as BOOL: Cast Exception
            with self.assertRaises(RuntimeError):
                self.assertTrue(ha.getAs('a', "BOOL"))
            # Get as COMPLEX_DOUBLE
            if Attributes is karabind.HashAttributes:
                self.assertEqual(ha.getAs('a', "COMPLEX_DOUBLE"),
                                 complex(65, 0))
            # Get as STRING
            self.assertEqual(ha.getAs('a', "STRING"), '65')

            # Change the type ...
            current_type_is(Types.INT32)
            self.assertEqual(ha['a'], 65)
            # INT64
            ha.getNode('a').setType("INT64")
            current_type_is(Types.INT64)
            self.assertEqual(ha['a'], 65)
            # INT16
            ha.getNode('a').setType("INT16")
            current_type_is(Types.INT16)
            self.assertEqual(ha['a'], 65)
            # INT8
            ha.getNode('a').setType("INT8")
            current_type_is(Types.INT8)
            self.assertEqual(ha['a'], 65)
            # DOUBLE
            ha.getNode('a').setType("DOUBLE")
            current_type_is(Types.DOUBLE)
            self.assertEqual(ha['a'], 65.0)
            # COMPLEX_DOUBLE
            ha.getNode('a').setType("COMPLEX_DOUBLE")
            current_type_is(Types.COMPLEX_DOUBLE)
            self.assertEqual(ha['a'], complex(65, 0))  # (65+0j)
            # STRING
            ha.getNode('a').setType("STRING")
            current_type_is(Types.STRING)
            self.assertEqual(ha['a'], '(65,0)')

        # Run test in karabind version
        inner(karabind.HashAttributes, karabind.HashAttributesNode,
              karabind.Types)
        # Run test in karathon version
        inner(karathon.HashAttributes, karathon.HashAttributesNode,
              karathon.Types)
