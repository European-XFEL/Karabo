import math
from weakref import proxy

from PyQt4.QtCore import QPoint, Qt
from PyQt4.QtGui import QBrush, QPainter, QPainterPath, QPen, QWidget

from karabo_gui.sceneview.utils import save_painter_state
from .const import (CHANNEL_DIAMETER, CHANNEL_LENGTH, CHANNEL_INPUT,
                    CHANNEL_OUTPUT, DATA_DIST_SHARED)


class WorkflowOverlay(QWidget):
    """ An overlay for the main scene where workflow channels and connections
    are drawn.
    """
    def __init__(self, scene_view, parent=None):
        super(WorkflowOverlay, self).__init__(parent)
        self.scene_view = proxy(scene_view)

        self.setAttribute(Qt.WA_TransparentForMouseEvents, True)

    def paintEvent(self, event):
        workflow_model = self.scene_view.workflow_model
        with QPainter(self) as painter:
            for channel in workflow_model.channels:
                with save_painter_state(painter):
                    _draw_channel(painter, channel)
            for connection in workflow_model.connections:
                with save_painter_state(painter):
                    _draw_connection(painter, connection)


def _draw_channel(painter, channel):
    """ Draw an input or output channel
    """
    start_pos = channel.position
    end_pos = QPoint(0, 0)
    if channel.kind == CHANNEL_INPUT:
        end_pos = QPoint(start_pos.x() - CHANNEL_LENGTH, start_pos.y())
    elif channel.kind == CHANNEL_OUTPUT:
        end_pos = QPoint(start_pos.x() + CHANNEL_LENGTH, start_pos.y())

    painter.setBrush(QBrush(Qt.white))
    painter.drawLine(start_pos, end_pos)
    painter.drawEllipse(end_pos, CHANNEL_DIAMETER, CHANNEL_DIAMETER)


def _draw_connection(painter, connection):
    """ Draw a curve connecting two workflow channels.
    """
    start_pos = connection.output_pos
    end_pos = connection.input_pos
    width = abs(end_pos.x() - start_pos.x())
    height = abs(end_pos.y() - start_pos.y())
    length = math.sqrt(width**2 + height**2)
    delta = length / 3

    c1 = QPoint(start_pos.x() + delta, start_pos.y())
    c2 = QPoint(end_pos.x() - delta, end_pos.y())

    if connection.data_distribution == DATA_DIST_SHARED:
        painter.setPen(QPen(Qt.DashLine))

    curve = QPainterPath(start_pos)
    curve.cubicTo(c1, c2, end_pos)
    painter.drawPath(curve)
