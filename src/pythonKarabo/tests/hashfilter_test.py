# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest

from karabo.karathon import *
from karabo.decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from karabo.configurator import Configurator
from configuration_example_classes import Base, P1, P2, P3, GraphicsRenderer2

        
class  HashfilterTestCase(unittest.TestCase):

    def test_hashfilter(self):
        try:
            schema = Configurator(GraphicsRenderer2).getSchema("GraphicsRenderer2")
            validator = Validator()
            config = validator.validate(schema, Hash())
            result = HashFilter.byTag(schema, config, "KW;KW,BH", ",;")

            self.assertFalse("antiAlias" in result)
            self.assertTrue("color" in result)
            self.assertFalse("bold" in result)
            self.assertTrue("shapes" in result)
            self.assertTrue("shapes.rectangle" in result)
            self.assertTrue("shapes.rectangle.b" in result)
            self.assertTrue("shapes.rectangle.c" in result)
            self.assertTrue("letter" in result)
            self.assertTrue("letter.a" in result)
            self.assertTrue("letter.b" in result)
            self.assertTrue("letter.c" in result)
            self.assertFalse("letter.d" in result)
            self.assertFalse("letter.e" in result)
            self.assertFalse("letter.f" in result)
            self.assertTrue("chars" in result)
            self.assertTrue("chars[0]" in result)
            self.assertTrue("chars[0].P2" in result)
            self.assertTrue("chars[0].P2.x" in result)
            self.assertFalse("chars[0].P2.y" in result)
            self.assertTrue("chars[0].P2.z" in result)
            self.assertTrue("chars[1]" in result)
            self.assertFalse("chars[1].P3" in result)
            self.assertFalse("chars[1].P3.k" in result)
            self.assertFalse("chars[1].P3.l" in result)
            self.assertFalse("chars[1].P3.m" in result)
            
        except Exception as e:
            self.fail("test_hashfilter exception group 1: " + str(e))

        try:
            result = HashFilter.byTag(schema, config, "JS", ",;")
            
            self.assertFalse("antiAlias" in result)
            self.assertFalse("color" in result)
            self.assertFalse("bold" in result)
            self.assertTrue("shapes" in result)
            self.assertTrue("shapes.rectangle" in result)
            self.assertTrue("shapes.rectangle.b" in result)
            self.assertTrue("shapes.rectangle.c" in result)
            self.assertTrue("letter" in result)
            self.assertTrue("letter.a" in result)
            self.assertFalse("letter.b" in result)
            self.assertFalse("letter.c" in result)
            self.assertTrue("letter.d" in result)
            self.assertFalse("letter.e" in result)
            self.assertFalse("letter.f" in result)
            self.assertFalse("chars" in result)
            self.assertFalse("chars[0]" in result)
            self.assertFalse("chars[0].P2" in result)
            self.assertFalse("chars[0].P2.x" in result)
            self.assertFalse("chars[0].P2.y" in result)
            self.assertFalse("chars[0].P2.z" in result)
            self.assertFalse("chars[1]" in result)
            self.assertFalse("chars[1].P3" in result)
            self.assertFalse("chars[1].P3.k" in result)
            self.assertFalse("chars[1].P3.l" in result)
            self.assertFalse("chars[1].P3.m" in result)
            
        except Exception as e:
            self.fail("test_hashfilter exception group 2: " + str(e))

        try:
            result = HashFilter.byTag(schema, config, "NC,LM", ",;")
            
            self.assertTrue("antiAlias" in result)
            self.assertFalse("color" in result)
            self.assertTrue("bold" in result)
            self.assertTrue("shapes" in result)
            self.assertTrue("shapes.rectangle" in result)
            self.assertFalse("shapes.rectangle.b" in result)
            self.assertTrue("shapes.rectangle.c" in result)
            self.assertTrue("letter" in result)
            self.assertTrue("letter.a" in result)
            self.assertFalse("letter.b" in result)
            self.assertFalse("letter.c" in result)
            self.assertFalse("letter.d" in result)
            self.assertTrue("letter.e" in result)
            self.assertTrue("letter.f" in result)
            self.assertTrue("chars" in result)
            self.assertTrue("chars[0]" in result)
            self.assertTrue("chars[0].P2" in result)
            self.assertTrue("chars[0].P2.x" in result)
            self.assertTrue("chars[0].P2.y" in result)
            self.assertTrue("chars[0].P2.z" in result)
            self.assertTrue("chars[1]" in result)
            self.assertTrue("chars[1].P3" in result)
            self.assertTrue("chars[1].P3.k" in result)
            self.assertTrue("chars[1].P3.l" in result)
            self.assertTrue("chars[1].P3.m" in result)
            
        except Exception as e:
            self.fail("test_hashfilter exception group 3: " + str(e))

        try:
            result = HashFilter.byTag(schema, config, "CY", ",;")
            
            self.assertFalse("antiAlias" in result)
            self.assertFalse("color" in result)
            self.assertFalse("bold" in result)
            self.assertTrue("shapes" in result)
            self.assertTrue("shapes.rectangle" in result)
            self.assertTrue("shapes.rectangle.b" in result)
            self.assertTrue("shapes.rectangle.c" in result)
            self.assertTrue("letter" in result)
            self.assertTrue("letter.a" in result)
            self.assertTrue("letter.b" in result)
            self.assertFalse("letter.c" in result)
            self.assertTrue("letter.d" in result)
            self.assertTrue("letter.e" in result)
            self.assertFalse("letter.f" in result)
            self.assertTrue("chars" in result)
            self.assertTrue("chars[0]" in result)
            self.assertTrue("chars[0].P2" in result)
            self.assertFalse("chars[0].P2.x" in result)
            self.assertTrue("chars[0].P2.y" in result)
            self.assertTrue("chars[0].P2.z" in result)
            self.assertTrue("chars[1]" in result)
            self.assertTrue("chars[1].P3" in result)
            self.assertFalse("chars[1].P3.k" in result)
            self.assertTrue("chars[1].P3.l" in result)
            self.assertTrue("chars[1].P3.m" in result)
            
        except Exception as e:
            self.fail("test_hashfilter exception group 4: " + str(e))
        
        try:
            result = HashFilter.byTag(schema, config, "BF", ",;")
            
            self.assertFalse("antiAlias" in result)
            self.assertFalse("color" in result)
            self.assertFalse("bold" in result)
            self.assertFalse("shapes" in result)
            self.assertFalse("shapes.rectangle" in result)
            self.assertFalse("shapes.rectangle.b" in result)
            self.assertFalse("shapes.rectangle.c" in result)
            self.assertFalse("letter" in result)
            self.assertFalse("letter.a" in result)
            self.assertFalse("letter.b" in result)
            self.assertFalse("letter.c" in result)
            self.assertFalse("letter.d" in result)
            self.assertFalse("letter.e" in result)
            self.assertFalse("letter.f" in result)
            self.assertTrue("chars" in result)
            self.assertTrue("chars[0]" in result)
            self.assertFalse("chars[0].P2" in result)
            self.assertFalse("chars[0].P2.x" in result)
            self.assertFalse("chars[0].P2.y" in result)
            self.assertFalse("chars[0].P2.z" in result)
            self.assertTrue("chars[1]" in result)
            self.assertTrue("chars[1].P3" in result)
            self.assertFalse("chars[1].P3.k" in result)
            self.assertFalse("chars[1].P3.l" in result)
            self.assertTrue("chars[1].P3.m" in result)
            
        except Exception as e:
            self.fail("test_hashfilter exception group 5: " + str(e))
        
        try:
            result = HashFilter.byTag(schema, config, "WP76", ",;")
            
            self.assertFalse("antiAlias" in result)
            self.assertFalse("color" in result)
            self.assertFalse("bold" in result)
            self.assertFalse("shapes" in result)
            self.assertFalse("shapes.rectangle" in result)
            self.assertFalse("shapes.rectangle.b" in result)
            self.assertFalse("shapes.rectangle.c" in result)
            self.assertFalse("letter" in result)
            self.assertFalse("letter.a" in result)
            self.assertFalse("letter.b" in result)
            self.assertFalse("letter.c" in result)
            self.assertFalse("letter.d" in result)
            self.assertFalse("letter.e" in result)
            self.assertFalse("letter.f" in result)
            self.assertFalse("chars" in result)
            self.assertFalse("chars[0]" in result)
            self.assertFalse("chars[0].P2" in result)
            self.assertFalse("chars[0].P2.x" in result)
            self.assertFalse("chars[0].P2.y" in result)
            self.assertFalse("chars[0].P2.z" in result)
            self.assertFalse("chars[1]" in result)
            self.assertFalse("chars[1].P3" in result)
            self.assertFalse("chars[1].P3.k" in result)
            self.assertFalse("chars[1].P3.l" in result)
            self.assertFalse("chars[1].P3.m" in result)
            
        except Exception as e:
            self.fail("test_hashfilter exception group 6: " + str(e))
        
            
if __name__ == '__main__':
    unittest.main()

