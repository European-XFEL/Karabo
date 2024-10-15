from ..utils import get_arrowhead_points


def test_get_arrowhead_points():
    # Test case 1: Horizontal line
    x1, y1, x2, y2 = 0, 0, 100, 0
    left, right = get_arrowhead_points(x1, y1, x2, y2)

    assert left.x() < x2 and right.x() < x2, (
        "Arrowhead points should be behind the endpoint")
    assert left.x() == right.x(), (
        "Arrowhead points should have the same x-coordinate")
    assert left.y() == - right.y(), "Left y should be negative of right y "

    # Test case 2: Vertical line
    x1, y1, x2, y2 = 0, 0, 0, 100
    left, right = get_arrowhead_points(x1, y1, x2, y2)
    assert left.y() < y2 and right.y() < y2, (
        "Arrowhead points should be above the endpoint")
    assert round(left.x(), 9) == round(-right.x(), 9), (
        "left x should be negative right x")
    assert left.y() == right.y()

    # Test case 3: Diagonal line
    x1, y1, x2, y2 = 0, 0, 100, 100
    left, right = get_arrowhead_points(x1, y1, x2, y2)

    assert left != right, "Arrowhead points should be distinct"
    assert left.x() < x2 and right.x() < x2, (
        "Arrowhead points should be behind the endpoint")
    assert left.y() < y2 and right.y() < y2, (
        "Arrowhead points should be below the endpoint")

    # Test case 4: Negative coordinates
    x1, y1, x2, y2 = -100, -100, -200, -200
    left, right = get_arrowhead_points(x1, y1, x2, y2)

    assert left != right, "Arrowhead points should be distinct"
    assert round(left.x(), 9) == round(right.y(), 9)
    assert round(left.y(), 9) == round(right.x(), 9)
