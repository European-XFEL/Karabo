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

import pytest

from karabo.bound import (
    FLOAT_ELEMENT, INT32_ELEMENT, NODE_ELEMENT, Schema, Types)


class ShapePosition:
    @staticmethod
    def expectedParameters(expected):
        (
            FLOAT_ELEMENT(expected).key("shapeX")
            .displayedName("Shape X")
            .description("X coordinate of shape")
            .assignmentOptional().defaultValue(0.0)
            .reconfigurable()
            .commit(),

            FLOAT_ELEMENT(expected).key("shapeY")
            .displayedName("Shape Y")
            .description("Y coordinate of shape")
            .assignmentOptional().defaultValue(0.0)
            .reconfigurable()
            .commit(),
        )


class Circle:
    @staticmethod
    def expectedParameters(expected):
        (
            NODE_ELEMENT(expected).key("center")
            .displayedName("Center")
            .description("Center of circle")
            .appendParametersOf(ShapePosition)
            .commit(),

            FLOAT_ELEMENT(expected).key("radius")
            .displayedName("Radius")
            .description("Radius of circle")
            .assignmentOptional().defaultValue(1.0)
            .reconfigurable()
            .commit(),
        )


class Rectangle:
    @staticmethod
    def expectedParameters(expected):
        (
            NODE_ELEMENT(expected).key("topLeftCorner")
            .displayedName("Top-left corner")
            .description("Top-left corner of rectangle")
            .appendParametersOf(ShapePosition)
            .commit(),

            FLOAT_ELEMENT(expected).key("width")
            .displayedName("Width")
            .description("Width of rectangle")
            .assignmentOptional().defaultValue(1.0)
            .reconfigurable()
            .commit(),

            FLOAT_ELEMENT(expected).key("height")
            .displayedName("Height")
            .description("Height of rectangle")
            .assignmentOptional().defaultValue(1.0)
            .reconfigurable()
            .commit(),
        )


class A:
    @staticmethod
    def expectedParameters(expected):
        (
            INT32_ELEMENT(expected)
            .key("a")
            .assignmentOptional().defaultValue(10)
            .reconfigurable()
            .commit(),
        )


def test_appendSchema():
    dataschema = Schema()
    A.expectedParameters(dataschema)

    class B:
        @staticmethod
        def expectedParameters(expected):
            (
                NODE_ELEMENT(expected)
                .key("node")
                .appendSchema(dataschema)
                .commit(),
            )

    schema = Schema()
    B.expectedParameters(schema)
    assert schema.has("node.a") is True
    assert schema.getValueType("node.a") == Types.INT32
    assert schema.getDefaultValue("node.a") == 10


def test_appendParametersOf():
    schema = Schema("circle")
    Circle.expectedParameters(schema)
    assert schema.getRootName() == "circle"
    assert schema.isNode("center") is True
    assert schema.isLeaf("center.shapeX") is True
    assert schema.getValueType("center.shapeX") == Types.FLOAT
    assert schema.getDisplayedName("center.shapeX") == "Shape X"
    assert schema.getDisplayedName("center.shapeY") == "Shape Y"
    assert schema.getDefaultValue("center.shapeX") == 0.0
    assert schema.getDefaultValue("center.shapeY") == 0.0
    assert schema.getDisplayedName("radius") == "Radius"
    assert schema.getValueType("radius") == Types.FLOAT
    assert schema.getDefaultValue("radius") == 1.0
    schema = Schema("rect")
    Rectangle.expectedParameters(schema)
    assert schema.getRootName() == "rect"
    assert schema.isNode("topLeftCorner") is True
    assert schema.getDisplayedName("topLeftCorner.shapeX") == "Shape X"
    assert schema.getDisplayedName("topLeftCorner.shapeY") == "Shape Y"
    assert schema.getDefaultValue("topLeftCorner.shapeX") == 0.0
    assert schema.getDefaultValue("topLeftCorner.shapeY") == 0.0
    assert schema.getDisplayedName("width") == "Width"
    assert schema.getDisplayedName("height") == "Height"
    assert schema.getDefaultValue("width") == 1.0
    assert schema.getValueType("height") == Types.FLOAT
    assert schema.getDefaultValue("height") == 1.0


def test_keyValidation():
    s = Schema()
    NODE_ELEMENT(s).key("allowed").commit()
    NODE_ELEMENT(s).key("allowed", True).commit()
    with pytest.raises(RuntimeError, match="illegal character at position 3"):
        NODE_ELEMENT(s).key("Bad@Inside", True).commit()

    # We can tolerate the @, but not the $
    NODE_ELEMENT(s).key("Bad@Inside", False).commit()
    with pytest.raises(RuntimeError,
                       match="not tolerated character at position 3"):
        NODE_ELEMENT(s).key("Bad$Inside", False).commit()
