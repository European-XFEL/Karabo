# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.bound import Hash, Types


def test_getAs():
    h = Hash("a", True)
    assert h.getAs("a", Types.STRING) == "1"
    assert h.getAs("a", Types.INT32) == 1
    assert h.getAs("a", Types.INT64) == 1
    assert h.getAs("a", Types.FLOAT) == 1.0
    assert h.getAs("a", Types.DOUBLE) == 1.0

    h = Hash("a", True)
    h.setAttribute("a", "a", True)
    assert h.getAttributeAs("a", "a", Types.STRING) == "1"
    assert h.getAttributeAs("a", "a", Types.INT32) == 1
    assert h.getAttributeAs("a", "a", Types.DOUBLE) == 1.0
    h.setAttribute("a", "b", 12)
    h.setAttribute("a", "c", 1.23)
    attrs = h.getAttributes("a")
    g = Hash("Z.a.b.c", "value")
    g.setAttributes("Z.a.b.c", attrs)
    assert g.getAttributeAs("Z.a.b.c", "a", Types.STRING) == "1"
    assert g.getAttributeAs("Z.a.b.c", "a", Types.INT32) == 1
    assert g.getAttributeAs("Z.a.b.c", "a", Types.DOUBLE) == 1.0

    # value is python list of boolean -> std::vector<bool>
    h = Hash("a", [False, False, False, False])
    assert h.getAs("a", Types.STRING) == "0,0,0,0"
    assert h.getAs("a", Types.VECTOR_INT32)[3] == 0

    # value is python list -> std::vector<char>
    h = Hash("a", ['1', '2', '3', '4'])
    assert h.getAs("a", Types.STRING) == "1,2,3,4"
    assert h.getAs("a", Types.VECTOR_INT32)[3] == 4

    h = Hash("a", [13, 13, 13, 13])
    assert h.getAs("a", Types.STRING) == "13,13,13,13"

    h = Hash("a", [-42])
    assert h.getAs("a", Types.STRING) == "-42"

    h = Hash("a", -2147483647)
    assert h.getAs("a", Types.STRING) == "-2147483647"

    h = Hash("a", 1234567890123456789)
    assert h.getAs("a", Types.STRING) == "1234567890123456789"
    assert h.getType("a") == Types.INT64
    assert str(h.getType("a")) == "INT64"

    h = Hash("a", 0.123456789123456)
    assert h.getAs("a", Types.STRING) == "0.123456789123456"
    assert h.getType("a") == Types.DOUBLE
