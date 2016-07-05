import numpy as np
from PyQt4.QtCore import QPoint, Qt
from PyQt4.QtGui import QMessageBox, QPainterPath, QPen
from traits.api import Dict, Instance

from karabo_gui.sceneview.bases import BaseSceneTool, BaseSceneAction
from karabo_gui.sceneview.workflow.api import (
    SceneWorkflowModel, WorkflowChannelModel, WorkflowConnectionModel,
    get_curve_points, CHANNEL_OUTPUT, CHANNEL_DIAMETER)

MIN_CLICK_DIST = 10


class CreateWorkflowConnectionToolAction(BaseSceneAction):
    """ A BaseSceneAction which starts up the WorkflowConnectionTool
    """
    def perform(self, scene_view):
        tool = WorkflowConnectionTool(workflow_model=scene_view.workflow_model)
        scene_view.set_tool(tool)
        scene_view.set_cursor('cross')


class WorkflowConnectionTool(BaseSceneTool):
    """ A tool for connecting workflow items
    """
    # We need to draw
    visible = True
    # We want to watch the scene view's workflow model
    workflow_model = Instance(SceneWorkflowModel)

    # Some private state variables for the tool
    _connection_cache = Dict
    _path = Instance(QPainterPath)
    _start_pos = Instance(QPoint)
    _start_channel = Instance(WorkflowChannelModel)
    _hover_channel = Instance(WorkflowChannelModel)
    _hover_connection = Instance(WorkflowConnectionModel)

    def draw(self, painter):
        """ Draw the line
        """
        if self._path is not None:
            painter.setPen(QPen())
            painter.drawPath(self._path)
        if self._hover_channel is not None:
            chan_pos = self._hover_channel.position
            pen = QPen(Qt.green)
            pen.setWidth(MIN_CLICK_DIST)
            painter.setPen(pen)
            painter.drawEllipse(chan_pos, CHANNEL_DIAMETER, CHANNEL_DIAMETER)
        if self._hover_connection is not None:
            pen = QPen(Qt.green)
            pen.setWidth(MIN_CLICK_DIST/2)
            painter.setPen(pen)
            path, _ = self._connection_cache[self._hover_connection]
            painter.drawPath(path)

    def mouse_down(self, scene_view, event):
        """ A callback which is fired whenever the user clicks in the
        SceneView.
        """
        pos = event.pos()
        if self._hover_channel is not None:
            self._start_channel = self._hover_channel
            self._start_pos = pos
            self._path = QPainterPath(pos)
            event.accept()
        elif (self._hover_connection is not None
                and event.button() == Qt.LeftButton):
            result = QMessageBox.question(
                scene_view, 'Remove connection?',
                'Do you really want to remove the connection?',
                QMessageBox.Yes | QMessageBox.No)
            if result == QMessageBox.Yes:
                # Disconnect!
                connection = self._hover_connection
                self._hover_connection = None
                disconnect_channels(connection.output, connection.input)
            event.accept()

    def mouse_move(self, scene_view, event):
        """ A callback which is fired whenever the user moves the mouse
        in the SceneView.
        """
        pos = event.pos()
        ch = self._get_channel_at_pos(pos, scene_view)
        if ch is not None:
            self._hover_channel = ch
            self._hover_connection = None
        else:
            self._hover_channel = None

        if event.buttons() and self._path is not None:
            start_end = (self._start_pos, pos)
            if self._start_channel.kind != CHANNEL_OUTPUT:
                start_end = (pos, self._start_pos)
            points = get_curve_points(*start_end)
            self._path = QPainterPath(points[0])
            self._path.cubicTo(*points[1:4])
        elif self._hover_channel is None:
            self._hover_connection = self._get_connection_at_pos(pos)

    def mouse_up(self, scene_view, event):
        """ A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
        ch = self._get_channel_at_pos(event.pos(), scene_view)
        if ch is not None:
            start_ch = ch if ch.kind == CHANNEL_OUTPUT else self._start_channel
            end_ch = ch if ch is not start_ch else self._start_channel
            connect_channels(start_ch, end_ch)

        if self._start_channel is not None:
            self._start_channel = None
            self._path = None

    def _build_cache(self, event):
        """ Build a cache of connection curves for high-speed intersection
        checking.
        """
        def path_from_points(points):
            path = QPainterPath(points[0])
            path.cubicTo(*points[1:4])
            return path

        def path_to_array(path):
            pts = [path.pointAtPercent(t) for t in np.linspace(0, 1, 100)]
            return np.array([[pt.x(), pt.y()] for pt in pts])

        for conn in self.workflow_model.connections:
            if conn not in self._connection_cache:
                path = path_from_points(conn.curve_points)
                points = path_to_array(path)
                self._connection_cache[conn] = (path, points)

        for conn in list(self._connection_cache.keys()):
            if conn not in self.workflow_model.connections:
                del self._connection_cache[conn]

    def _get_channel_at_pos(self, pos, scene_view):
        workflow_model = scene_view.workflow_model
        for ch in workflow_model.channels:
            dist = (ch.position - pos).manhattanLength()
            if dist < MIN_CLICK_DIST:
                return ch

    def _get_connection_at_pos(self, pos):
        for conn, (path, points) in self._connection_cache.items():
            if not path.controlPointRect().contains(pos.x(), pos.y()):
                continue
            # compute the distance from the position to the curve
            offsets = points - np.array([pos.x(), pos.y()])
            dists = np.sqrt(offsets[:, 0]**2 + offsets[:, 1]**2)
            if np.min(dists) < MIN_CLICK_DIST:
                return conn

    def _workflow_model_changed(self):
        self.workflow_model.on_trait_change(self._build_cache, 'updated')
        self._build_cache(None)


def connect_channels(start_channel, end_channel):
    """ Establish a connection between two channels.
    """
    path = ".".join(start_channel.box.path)
    if not end_channel.box.boxvalue.connectedOutputChannels.hasValue():
        end_channel.box.value.connectedOutputChannels = []

    paths = ["{}:{}".format(id, path) for id in start_channel.device_ids]
    old_connections = end_channel.box.value.connectedOutputChannels
    new_connections = [c for c in paths if c not in old_connections]
    end_channel.box.value.connectedOutputChannels.extend(new_connections)
    end_channel.box.boxvalue.connectedOutputChannels.update()


def disconnect_channels(start_channel, end_channel):
    """ Break the connection between two channels.
    """
    if start_channel is None or end_channel is None:
        return

    path = ".".join(start_channel.box.path)
    paths = {"{}:{}".format(did, path) for did in start_channel.device_ids}
    old_connections = end_channel.box.value.connectedOutputChannels
    new_connections = [c for c in old_connections if c not in paths]
    end_channel.box.value.connectedOutputChannels = new_connections
