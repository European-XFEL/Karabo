# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest

from karabo.bound import Configurator, Hash, HashFilter, Validator

from .configuration_example_classes import GraphicsRenderer2


class HashfilterTestCase(unittest.TestCase):
    def test_hashfilter(self):
        try:
            schema = Configurator(GraphicsRenderer2).getSchema(
                "GraphicsRenderer2")
            validator = Validator()
            _, _, config = validator.validate(schema, Hash())
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

        except Exception as e:
            self.fail("test_hashfilter exception group 6: " + str(e))


if __name__ == '__main__':
    unittest.main()
