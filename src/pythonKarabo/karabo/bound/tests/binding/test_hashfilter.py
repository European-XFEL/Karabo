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

from karabo.bound import (
    BOOL_ELEMENT, FLOAT_ELEMENT, INT32_ELEMENT, INT64_ELEMENT, NODE_ELEMENT,
    STRING_ELEMENT, UINT32_ELEMENT, AssemblyRules, Hash, HashFilter,
    MetricPrefix, Unit, Validator)
from karabo.bound.decorators import (
    KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS)
from karabo.common.states import State


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("BaseY", "1.0")
class BaseY:
    def __init__(self, configuration):
        super().__init__()


@KARABO_CLASSINFO("P1Y", "1.0")
class P1Y(BaseY):
    def __init__(self, configuration):
        super().__init__(configuration)

    @staticmethod
    def expectedParameters(expected):
        (
            STRING_ELEMENT(expected).key("a")
            .description("a").displayedName("a")
            .assignmentOptional().defaultValue("a value")
            .tags("CY,CY,NC,JS,KW,NC")
            .commit(),

            STRING_ELEMENT(expected).key("b")
            .tags("BH,CY")
            .displayedName("Example key 1")
            .description("Example key 1 description")
            .options("Radio,Air Condition,Navigation", ",")
            .assignmentOptional().defaultValue("Air Condition")
            .reconfigurable()
            .commit(),

            INT32_ELEMENT(expected).key("c").alias(10)
            .tags("BH")
            .displayedName("Example key 2")
            .description("Example key 2 description")
            .options("5, 25, 10")
            .minInc(5).maxInc(25).unit(Unit.METER).metricPrefix(
                MetricPrefix.MILLI)
            .assignmentOptional().defaultValue(5)
            .init()
            .commit(),

            UINT32_ELEMENT(expected).key("d").alias(5.5)
            .tags("CY,JS")
            .displayedName("Example key 3")
            .description("Example key 3 description")
            .allowedStates(State.STARTED, State.STOPPED,
                           State.NORMAL)  # TODO check
            .minExc(10).maxExc(20).assignmentOptional().defaultValue(11)
            .reconfigurable()
            .commit(),

            FLOAT_ELEMENT(expected).key("e").alias("exampleAlias4")
            .tags("DB,NC,CY")
            .displayedName("Example key 4")
            .description("Example key 4 description")
            .options("1.1100000, -2.22, 5.55")
            .assignmentOptional()
            .defaultValue(1.11)
            .commit(),

            INT64_ELEMENT(expected).key("f").alias("exampleAlias5")
            .tags("LM,DB")
            .displayedName("Example key 5")
            .description("Example key 5 description")
            .assignmentOptional()
            .defaultValue(5)
            .commit(),
        )


@KARABO_CLASSINFO("P2Y", "1.0")
class P2Y(BaseY):
    def __init__(self, configuration):
        super().__init__(configuration)

    @staticmethod
    def expectedParameters(expected):
        (
            STRING_ELEMENT(expected).key("x")
            .description("x")
            .displayedName("x")
            .assignmentOptional().defaultValue("a value")
            .tags("LM,BH")
            .commit(),

            STRING_ELEMENT(expected).key("y")
            .tags("CY")
            .displayedName("Example key 1")
            .description("Example key 1 description")
            .options("Radio,Air Condition,Navigation", ",")
            .assignmentOptional().defaultValue("Radio")
            .reconfigurable()
            .commit(),

            INT32_ELEMENT(expected).key("z").alias(10)
            .tags("CY,LM,KW")
            .displayedName("Example key 2")
            .description("Example key 2 description")
            .options("5, 25, 10")
            .minInc(5).maxInc(25).unit(Unit.AMPERE).metricPrefix(
                MetricPrefix.MILLI)
            .assignmentOptional().defaultValue(10)
            .init()
            .commit(),
        )


@KARABO_CLASSINFO("P3Y", "1.0")
class P3Y(BaseY):
    def __init__(self, configuration):
        super().__init__(configuration)

    @staticmethod
    def expectedParameters(expected):
        (
            STRING_ELEMENT(expected).key("k")
            .description("k").displayedName("k")
            .assignmentOptional().defaultValue("k value")
            .tags("LM")
            .commit(),

            STRING_ELEMENT(expected).key("l")
            .tags("CY")
            .displayedName("l").description("l")
            .options("Radio,Air Condition,Navigation", ",")
            .assignmentOptional().defaultValue("Navigation")
            .reconfigurable()
            .commit(),

            INT32_ELEMENT(expected).key("m").alias(10)
            .tags("CY,DB,JE,BP,MK,PG,BF")
            .displayedName("Example key 2")
            .description("Example key 2 description")
            .options("5, 25, 10")
            .minInc(5).maxInc(25).unit(Unit.METER).metricPrefix(
                MetricPrefix.MILLI)
            .assignmentOptional().defaultValue(25)
            .init()
            .commit(),
        )


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("GraphicsRenderer2Y", "1.0")
class GraphicsRenderer2Y:
    def __init__(self, configuration):
        super().__init__()

    @staticmethod
    def expectedParameters(expected):
        (
            BOOL_ELEMENT(expected).key("antiAlias")
            .tags("NC")
            .displayedName("Use Anti-Aliasing")
            .description("You may switch of for speed")
            .assignmentOptional().defaultValue(True)
            .init().expertAccess()
            .commit(),

            STRING_ELEMENT(expected).key("color")
            .tags("KW")
            .displayedName("Color")
            .description("The default color for any shape")
            .assignmentOptional().defaultValue("red")
            .reconfigurable()
            .commit(),

            BOOL_ELEMENT(expected).key("bold")
            .tags("LM")
            .displayedName("Bold").description("Toggles bold painting")
            .assignmentOptional().defaultValue(False)
            .reconfigurable()
            .commit(),

            STRING_ELEMENT(expected).key("shapes")
            .tags("DB")
            .assignmentOptional().defaultValue("rectangle")
            .commit(),

            NODE_ELEMENT(expected).key("circle")
            .tags("JS")
            .displayedName("Circle").description("A circle")
            .commit(),

            FLOAT_ELEMENT(expected).key("circle.radius")
            .description("The radius of the circle")
            .displayedName("Radius")
            .tags("NC,KW")
            .minExc(0).maxExc(100).unit(Unit.METER).metricPrefix(
                MetricPrefix.MILLI)
            .assignmentOptional().defaultValue(10)
            .init().commit(),

            NODE_ELEMENT(expected).key("rectangle")
            .tags("BH, KW , CY")
            .displayedName("Rectangle").description("A rectangle")
            .commit(),

            FLOAT_ELEMENT(expected).key("rectangle.b")
            .tags("JS")
            .description("Rectangle side - b").displayedName("Side B")
            .assignmentOptional().defaultValue(10)
            .init().commit(),

            FLOAT_ELEMENT(expected).key("rectangle.c")
            .tags("LM,JS")
            .description("Rectangle side - c").displayedName("Side C")
            .assignmentOptional().defaultValue(10)
            .init()
            .commit(),

            NODE_ELEMENT(expected).key("triangle")
            .displayedName("triangle")
            .description("A triangle (Node element containing no "
                         "other elements)")
            .commit(),

            NODE_ELEMENT(expected).key("letter")
            .displayedName("Letter").description("Letter")
            .appendParametersOf(P1Y)
            .commit(),

        )


def test_hash_filter():
    schema = GraphicsRenderer2Y.getSchema(
        "GraphicsRenderer2Y", rules=AssemblyRules())
    validator = Validator()
    _, _, config = validator.validate(schema, Hash())

    result = HashFilter.byTag(schema, config, "KW;KW,BH", ",;")

    assert ("antiAlias" in result) is False
    assert ("color" in result) is True
    assert ("bold" in result) is False
    assert ("shapes" in result) is False
    assert ("rectangle" in result) is True
    assert ("rectangle.b" in result) is True
    assert ("rectangle.c" in result) is True
    assert ("letter" in result) is True
    assert ("letter.a" in result) is True
    assert ("letter.b" in result) is True
    assert ("letter.c" in result) is True
    assert ("letter.d" in result) is False
    assert ("letter.e" in result) is False
    assert ("letter.f" in result) is False

    result = HashFilter.byTag(schema, config, "JS", ",;")

    assert ("antiAlias" in result) is False
    assert ("bold" in result) is False
    assert ("shapes" in result) is False
    assert ("rectangle" in result) is True
    assert ("rectangle.b" in result) is True
    assert ("rectangle.c" in result) is True
    assert ("letter" in result) is True
    assert ("letter.a" in result) is True
    assert ("letter.b" in result) is False
    assert ("letter.c" in result) is False
    assert ("letter.d" in result) is True
    assert ("letter.e" in result) is False
    assert ("letter.f" in result) is False

    result = HashFilter.byTag(schema, config, "NC,LM", ",;")

    assert ("antiAlias" in result) is True
    assert ("color" in result) is False
    assert ("bold" in result) is True
    assert ("shapes" in result) is False
    assert ("rectangle" in result) is True
    assert ("rectangle.b" in result) is False
    assert ("rectangle.c" in result) is True
    assert ("letter" in result) is True
    assert ("letter.a" in result) is True
    assert ("letter.b" in result) is False
    assert ("letter.c" in result) is False
    assert ("letter.d" in result) is False
    assert ("letter.e" in result) is True
    assert ("letter.f" in result) is True

    result = HashFilter.byTag(schema, config, "CY", ",;")

    assert ("antiAlias" in result) is False
    assert ("color" in result) is False
    assert ("bold" in result) is False
    assert ("shapes" in result) is False
    assert ("rectangle" in result) is True
    assert ("rectangle.b" in result) is True
    assert ("rectangle.c" in result) is True
    assert ("letter" in result) is True
    assert ("letter.a" in result) is True
    assert ("letter.b" in result) is True
    assert ("letter.c" in result) is False
    assert ("letter.d" in result) is True
    assert ("letter.e" in result) is True
    assert ("letter.f" in result) is False

    result = HashFilter.byTag(schema, config, "BF", ",;")

    assert ("antiAlias" in result) is False
    assert ("color" in result) is False
    assert ("bold" in result) is False
    assert ("shapes" in result) is False
    assert ("rectangle" in result) is False
    assert ("rectangle.b" in result) is False
    assert ("rectangle.c" in result) is False
    assert ("letter" in result) is False
    assert ("letter.a" in result) is False
    assert ("letter.b" in result) is False
    assert ("letter.c" in result) is False
    assert ("letter.d" in result) is False
    assert ("letter.e" in result) is False
    assert ("letter.f" in result) is False

    # no "WP76" tag
    result = HashFilter.byTag(schema, config, "WP76", ",;")

    assert ("antiAlias" in result) is False
    assert ("color" in result) is False
    assert ("bold" in result) is False
    assert ("shapes" in result) is False
    assert ("rectangle" in result) is False
    assert ("rectangle.b" in result) is False
    assert ("rectangle.c" in result) is False
    assert ("letter" in result) is False
    assert ("letter.a" in result) is False
    assert ("letter.b" in result) is False
    assert ("letter.c" in result) is False
    assert ("letter.d" in result) is False
    assert ("letter.e" in result) is False
    assert ("letter.f" in result) is False
