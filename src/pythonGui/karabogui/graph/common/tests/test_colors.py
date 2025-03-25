from karabogui.graph.common.api import get_available_colors, rgba_to_hex


def test_get_available_colors():
    expected = [("a", (255, 191, 0, 255)),
                ("b", (51, 153, 255, 255)),
                ("r", (255, 51, 51, 255)),
                ("g", (51, 160, 44, 255)),
                ("c", (166, 206, 227, 255)),
                ("p", (106, 61, 154, 255)),
                ("y", (255, 255, 0, 255)),
                ("n", (251, 154, 153, 255)),
                ("w", (177, 89, 40, 255)),
                ("o", (255, 127, 0, 255)),
                ("s", (178, 223, 138, 255)),
                ("d", (202, 178, 214, 255)),
                ("k", (0, 0, 0, 255))]

    assert list(get_available_colors()) == expected


def test_rgba_to_hex():
    assert rgba_to_hex((255, 191, 0, 255)) == "#ffbf00"
    assert rgba_to_hex((51, 153, 255, 255)) == "#3399ff"
    assert rgba_to_hex((106, 61, 154, 255)) == "#6a3d9a"
    assert rgba_to_hex((0, 0, 0, 255)) == "#000000"
