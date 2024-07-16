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

from karabo.bound import (
    INT32_ELEMENT, KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS,
    STRING_ELEMENT, Configurator, Hash, Types)
from karabo.common.states import State


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("ExampleBase", "1.0")
class ExampleBaseClass:
    def __init__(self, configuration):
        self.configuration = configuration

    @staticmethod
    def expectedParameters(expected):
        e = STRING_ELEMENT(expected)
        e.key('firstWord').displayedName("First Word").description(
            "Input for first word")
        e.assignmentOptional().defaultValue("")
        e.reconfigurable().commit()

        e = STRING_ELEMENT(expected)
        e.key("secondWord").displayedName("Second Word").description(
            "Input for first word")
        e.assignmentOptional().defaultValue("")
        e.reconfigurable().commit()

        e = INT32_ELEMENT(expected)
        e.key("multiply").displayedName("Multiply").description(
            "multiplies word")
        e.assignmentOptional().defaultValue(1)
        e.allowedStates(State.ERROR).reconfigurable().commit()

        e = STRING_ELEMENT(expected)
        e.key("composedWord").displayedName("Composed word").description(
            "The composed word")
        e.assignmentOptional().noDefaultValue()
        e.readOnly().commit()


@KARABO_CLASSINFO("ExampleDerived", "1.0")
class ExampleDerivedClass(ExampleBaseClass):
    def __init__(self, configuration):
        super().__init__(configuration)
        self.configuration = configuration

    @staticmethod
    def expectedParameters(expected):
        e = STRING_ELEMENT(expected)
        e.key('hostname').displayedName("Hostname").description(
            "Input for host name")
        e.assignmentOptional().defaultValue("")
        e.reconfigurable().commit()

        e = INT32_ELEMENT(expected)
        e.key("port").displayedName("Port").description("Input for port")
        e.assignmentOptional().defaultValue(2999)
        e.reconfigurable().commit()


@KARABO_CLASSINFO("Example", "1.0")
class ExampleClass(ExampleBaseClass):
    def __init__(self, configuration):
        super().__init__(configuration)
        self.configuration = configuration

    @staticmethod
    def expectedParameters(expected):
        e = STRING_ELEMENT(expected)
        e.key('street').displayedName("Street").description("Street")
        e.assignmentOptional().defaultValue("Albert-Einstein-Ring")
        e.reconfigurable().commit()

        e = INT32_ELEMENT(expected)
        e.key("bld").displayedName("Bld").description("Building number")
        e.assignmentOptional().defaultValue(19)
        e.reconfigurable().commit()


# ------------------------------------------------
# Inherit from a class without Karabo

class NoKaraboClass:
    def __init__(self, configuration):
        super().__init__(configuration)

    def getKaraboVersion(self):
        return 2


@KARABO_CLASSINFO("ExampleNoKarabo", "1.0")
class ExampleNoKarabo(NoKaraboClass, ExampleDerivedClass):
    def __init__(self, configuration):
        super().__init__(configuration)
        self.configuration = configuration

    @staticmethod
    def expectedParameters(expected):
        e = STRING_ELEMENT(expected)
        e.key('hostname').displayedName("Hostname").description(
            "Input for host name")
        e.assignmentOptional().defaultValue("")
        e.reconfigurable().commit()

        e = INT32_ELEMENT(expected)
        e.key("port").displayedName("Port").description("Input for port")
        e.assignmentOptional().defaultValue(2999)
        e.reconfigurable().commit()


class Decorators_TestCase(unittest.TestCase):
    def test_decorators_(self):
        try:
            # call Configurator by classid
            schema = Configurator("ExampleBase").getSchema('Example')
            self.assertIsNotNone(schema)
            # call Configurator by class
            schema = Configurator(ExampleBaseClass).getSchema('Example')
            self.assertEqual(
                schema.getValueType("firstWord"), Types.STRING,
                "expectedParameters failed -- no 'firstWord' key found")
            self.assertEqual(
                schema.getValueType("secondWord"), Types.STRING,
                "expectedParameters failed -- no 'secondWord' key found")
            self.assertEqual(
                schema.getValueType("multiply"), Types.INT32,
                "expectedParameters failed -- no 'multiply' key found")
            self.assertEqual(
                schema.getValueType("composedWord"), Types.STRING,
                "expectedParameters failed -- no 'composedWord' key found")

            # Just try the other methods
            example = Configurator('ExampleBase').create('Example', Hash())
            self.assertIsNotNone(example)
            schema_derived = Configurator(ExampleBaseClass).getSchema(
                'Example')
            self.assertIsNotNone(schema_derived)
            schema_inherit = ExampleNoKarabo.create(
                'ExampleNoKarabo', Hash()).getSchema('ExampleNoKarabo')
            self.assertFalse(schema_inherit.empty())
            self.assertTrue(schema_inherit.has('hostname'))
            self.assertTrue(schema_inherit.has('port'))

        except Exception as e:
            self.fail("test_decorators group 1: " + str(e))


if __name__ == '__main__':
    unittest.main()
