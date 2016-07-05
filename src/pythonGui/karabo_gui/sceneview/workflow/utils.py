import math

from PyQt4.QtCore import QPoint


def get_curve_points(start, end):
    """ Compute the control points for a simple cubic bezier.
    """
    width = abs(end.x() - start.x())
    height = abs(end.y() - start.y())
    length = math.sqrt(width**2 + height**2)
    delta = length / 3

    c1 = QPoint(start.x() + delta, start.y())
    c2 = QPoint(end.x() - delta, end.y())
    return [start, c1, c2, end]
