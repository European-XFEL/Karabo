import numpy as np

from karabo.native import Hash, HashList, has_changes


def test_array_equal():
    assert not has_changes(np.array([1, 2, 3]), np.array([1, 2, 3]))
    assert has_changes(np.array([1, 2, 3]), np.array([1, 2, 4]))


def test_has_changes():
    assert has_changes(None, 1)

    assert has_changes("", "a")
    assert not has_changes("", "")
    assert not has_changes("a", "a")
    assert not has_changes("a|", "a|")

    assert has_changes(True, False)
    assert not has_changes(True, True)

    assert not has_changes(2, 2)
    assert has_changes(3, 1)

    assert not has_changes(1.0, 1. + 5e-8)
    assert not has_changes(2.0e10, (2 + 1.0e-7) * 1e10)
    assert has_changes(2.0, 2.0 + 3e-7)
    assert has_changes(2.0, 2.0 + 3e-7)
    assert not has_changes(2.0, 2.0 + 3e-8)
    assert has_changes(2.0, 2.7)

    assert has_changes([1, 2, 3], [1, 2])
    assert has_changes([1, 2, 3], [1, 2, 4])
    assert not has_changes([1, 2, 3], [1, 2, 3])

    assert has_changes(np.array([1, 2, 3]), np.array([1, 2]))
    assert has_changes(np.array([1, 2, 7]), np.array([1, 2, 3]))
    assert not has_changes(np.array([1, 2, 3]), np.array([1, 2, 3]))

    # List of Hashes
    assert has_changes([Hash("float", 2.0)], [Hash("float", 2.0 + 3e-7)])
    assert not has_changes([Hash("float", 2.0)], [Hash("float", 2.0 + 3e-8)])

    fh = Hash("stringProperty", "foo",
              "uintProperty", 1,
              "boolProperty", True)
    sh = Hash("stringProperty", "bar",  # changed
              "uintProperty", 1,
              "boolProperty", True)
    assert has_changes(HashList([fh]), HashList([sh]))

    fh = Hash("stringProperty", "foo",
              "uintProperty", 1,
              "vectorProperty", np.array([1.2, 3.2]),  # changed
              "boolProperty", False)
    sh = Hash("stringProperty", "foo",
              "vectorProperty", np.array([1.2, 3.2221222]),
              "uintProperty", 1,
              "boolProperty", False)
    assert has_changes(HashList([fh]), HashList([sh]))

    fh = Hash("stringProperty", "foo",
              "uintProperty", 1,
              "boolProperty", True)
    sh = Hash("stringProperty", "foo",
              "uintProperty", 1,
              "boolProperty", True)
    assert not has_changes(HashList([fh]), HashList([sh]))

    fh = Hash("stringProperty", "foo",
              "uintProperty", 1,
              "boolProperty", False)
    sh = Hash("stringProperty", "foo",
              "uintProperty", Hash(),  # change faulty!
              "boolProperty", False)
    assert has_changes(HashList([fh]), HashList([sh]))

    fh = Hash("stringProperty", "foo",
              "uintProperty", 1,
              "boolProperty", False)
    sh = Hash("stringProperty", "foo",
              "uintProperty", 1,
              "boolProperty", True)  # changed
    assert has_changes(HashList([fh]), HashList([sh]))

    fh = Hash("stringProperty", "foo",
              "uintProperty", 1,
              "boolProperty", 2.0)  # changed faulty!
    sh = Hash("stringProperty", "foo",
              "uintProperty", 1,
              "boolProperty", True)
    assert has_changes(HashList([fh]), HashList([sh]))

    fh = Hash("stringProperty", "foo",
              "uintProperty", 1,
              "boolProperty", True)
    sh = Hash("stringProperty", "%%%$$$",  # changed
              "uintProperty", 1,
              "boolProperty", True)
    assert has_changes(HashList([fh]), HashList([sh]))