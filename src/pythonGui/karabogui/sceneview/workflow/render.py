from weakref import proxy

from PyQt4.QtCore import QPoint, Qt
from PyQt4.QtGui import QBrush, QPainter, QPainterPath, QPen, QWidget

from karabo.common.api import ProxyStatus
from karabogui.indicators import get_device_status_pixmap
from karabogui.sceneview.utils import save_painter_state
from .const import (CHANNEL_DIAMETER, CHANNEL_WIDTH, CHANNEL_INPUT,
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
            for connection in workflow_model.connections:
                with save_painter_state(painter):
                    _draw_connection(painter, connection)
            for channel in workflow_model.channels:
                with save_painter_state(painter):
                    _draw_channel(painter, channel)
            for device in workflow_model.devices:
                with save_painter_state(painter):
                    _draw_device_state(painter, device)


def _draw_channel(painter, channel):
    """ Draw an input or output channel
    """
    start_pos = QPoint(0, 0)
    end_pos = channel.position
    if channel.kind == CHANNEL_INPUT:
        start_pos = QPoint(end_pos.x() + CHANNEL_WIDTH, end_pos.y())
    elif channel.kind == CHANNEL_OUTPUT:
        start_pos = QPoint(end_pos.x() - CHANNEL_WIDTH, end_pos.y())

    painter.setBrush(QBrush(Qt.white))
    painter.drawLine(start_pos, end_pos)
    painter.drawEllipse(end_pos, CHANNEL_DIAMETER, CHANNEL_DIAMETER)


def _draw_connection(painter, connection):
    """ Draw a curve connecting two workflow channels.
    """
    points = connection.curve_points
    if connection.data_distribution == DATA_DIST_SHARED:
        painter.setPen(QPen(Qt.DashLine))

    curve = QPainterPath(points[0])
    curve.cubicTo(*points[1:4])
    painter.setRenderHint(QPainter.Antialiasing)
    painter.drawPath(curve)


def _draw_device_state(painter, device_model):
    """ Draw a pixmap which reflects the state of the connected device model

    The Device might be missing in the project due to concurrence. Then the
    device status is missing.
    """
    dev = device_model.device
    status = dev.proxy.status if dev is not None else ProxyStatus.MISSING
    error = (status is ProxyStatus.ERROR)
    pixmap = get_device_status_pixmap(status, error)
    if pixmap is not None:
        painter.drawPixmap(device_model.position, pixmap)
