# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.bound import Hash, similar


def test_merge():
    h1 = Hash("a", 1,
              "b", 2,
              "c.b[0].g", 3,
              "c.c[0].d", 4,
              "c.c[1]", Hash("a.b.c", 6),
              "d.e", 7)

    h2 = Hash("a", 21,
              "b.c", 22,
              "c.b[0]", Hash("key", "value"),
              "c.b[1].d", 24,
              "e", 27)

    h1 += h2
    assert h1.has("a")
    assert h1.get("a") == 21
    assert h1["a"] == 21
    assert h1.has("b")
    assert not h1.has("c.b.d")
    assert h1.has("c.b[0]")
    assert h1.has("c.b[1]")
    assert not h1.has("c.b[2]")
    assert h1.get("c.b[1].d") == 24
    assert h1.has("c.c[0].d")
    assert h1.has("c.c[1].a.b.c")
    assert h1.has("d.e")
    assert h1.has("e")

    h3 = h1
    assert similar(h1, h3)
