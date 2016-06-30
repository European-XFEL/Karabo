from PyQt4.QtCore import QLine, QPoint
from PyQt4.QtGui import QPen
from traits.api import Instance

from karabo_gui.sceneview.bases import BaseSceneTool
from karabo_gui.sceneview.workflow.api import (WorkflowChannelModel,
                                               CHANNEL_OUTPUT)

MIN_CLICK_DIST = 10


class WorkflowConnectionTool(BaseSceneTool):
    """ A tool for connecting workflow items
    """
    visible = True
    line = Instance(QLine)
    start_pos = Instance(QPoint)
    start_channel = Instance(WorkflowChannelModel)

    def draw(self, painter):
        """ Draw the line
        """
        if self.line is not None:
            painter.setPen(QPen())
            painter.drawLine(self.line)

    def mouse_down(self, scene_view, event):
        """ A callback which is fired whenever the user clicks in the
        SceneView.
        """
        pos = event.pos()
        channel = self._get_channel_at_pos(pos, scene_view)
        if channel is not None:
            self.start_channel = channel
            self.start_pos = pos
            self.line = QLine(pos, pos)

    def mouse_move(self, scene_view, event):
        """ A callback which is fired whenever the user moves the mouse
        in the SceneView.
        """
        if self._get_channel_at_pos(event.pos(), scene_view) is not None:
            scene_view.set_cursor('cross')
        else:
            scene_view.set_cursor('none')

        if event.buttons() and self.line is not None:
            self.line.setPoints(self.start_pos, event.pos())

    def mouse_up(self, scene_view, event):
        """ A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
        ch = self._get_channel_at_pos(event.pos(), scene_view)
        if ch is not None:
            start_ch = ch if ch.kind == CHANNEL_OUTPUT else self.start_channel
            end_ch = ch if ch is not start_ch else self.start_channel
            connect_channels(start_ch, end_ch)

        if self.start_channel is not None:
            self.start_channel = None
            self.line = None

    def _get_channel_at_pos(self, pos, scene_view):
        workflow_model = scene_view.workflow_model
        for ch in workflow_model.channels:
            dist = (ch.position - pos).manhattanLength()
            if dist < MIN_CLICK_DIST:
                return ch


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
