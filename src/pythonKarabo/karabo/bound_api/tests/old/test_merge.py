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
