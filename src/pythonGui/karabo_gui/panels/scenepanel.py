#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtCore import pyqtSignal, pyqtSlot, Qt
from PyQt4.QtGui import (QAction, QPalette, QScrollArea, QSizePolicy, QWidget)

from karabo_gui.docktabwindow import Dockable
import karabo_gui.icons as icons
from karabo_gui.sceneview.tools import (CreateToolAction, LineSceneTool,
                                        TextSceneTool, RectangleSceneTool,
                                        SceneLinkTool)
from karabo_gui.toolbar import ToolBar


class ScenePanel(Dockable, QScrollArea):
    signalClosed = pyqtSignal()

    def __init__(self, scene_view, connected_to_server):
        super(ScenePanel, self).__init__()

        # Reference to underlying scene view
        self.scene_view = scene_view
        self.setWidget(self.scene_view)

        self.setupActions(connected_to_server)

        self.setBackgroundRole(QPalette.Dark)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        self.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)

    def design_mode_text(self, is_design_mode):
        if is_design_mode:
            return "Change to control mode"

        return "Change to design mode"

    def setupActions(self, connected_to_server):
        self.create_design_mode_action(connected_to_server)
        self.create_text_action()
        self.create_line_action()
        self.create_rect_action()
        self.create_scene_link_action()
        self.create_group_actions()
        self.create_select_all_action()
        self.create_cut_action()
        self.create_copy_action()
        self.create_paste_action()
        self.create_replace_action()
        self.create_delete_action()
        self.create_bring_to_front_action()
        self.create_send_to_back_action()

    def setupToolBars(self, standardToolBar, parent):
        standardToolBar.addAction(self.ac_design_mode)

        tool_bar = ToolBar("Drawing")
        tool_bar.setVisible(self.scene_view.design_mode)
        parent.addToolBar(tool_bar)
        self.scene_view.setFocusProxy(tool_bar)
        tool_bar.setFocusPolicy(Qt.StrongFocus)

        tool_bar.addSeparator()
        tool_bar.addAction(self.ac_text)
        tool_bar.addAction(self.ac_line)
        tool_bar.addAction(self.ac_rect)
        tool_bar.addAction(self.ac_scene_link)

        self.drawing_tool_bar = tool_bar

        # Add placeholder widget to toolbar
        widget = QWidget()
        widget.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        self.drawing_tool_bar.addWidget(widget)

    @pyqtSlot(bool)
    def design_mode_changed(self, is_checked):
        self.drawing_tool_bar.setVisible(is_checked)
        text = self.design_mode_text(is_checked)
        self.ac_design_mode.setToolTip(text)
        self.ac_design_mode.setStatusTip(text)
        self.scene_view.design_mode = is_checked

    def create_design_mode_action(self, connected_to_server):
        """ Switch for design and control mode """
        text = self.design_mode_text(self.scene_view.design_mode)
        self.ac_design_mode = QAction(icons.transform, text, self)
        self.ac_design_mode.setToolTip(text)
        self.ac_design_mode.setStatusTip(text)
        self.ac_design_mode.setCheckable(True)
        self.ac_design_mode.setChecked(self.scene_view.design_mode)
        self.ac_design_mode.setEnabled(connected_to_server)
        self.ac_design_mode.toggled.connect(self.design_mode_changed)

    def create_text_action(self):
        """ Add text"""
        action = CreateToolAction(tool_factory=TextSceneTool,
                                  icon=icons.text, text="Add text",
                                  tooltip="Add text to scene")
        self.ac_text = QAction(action.icon, action.text, self)
        self.ac_text.setToolTip(action.text)
        self.ac_text.setStatusTip(action.tooltip)
        self.ac_text.triggered.connect(partial(action.perform, self.scene_view))

    def create_line_action(self):
        """ Add line"""
        action = CreateToolAction(tool_factory=LineSceneTool,
                                  icon=icons.line, text="Add line",
                                  tooltip="Add line to scene")
        self.ac_line = QAction(action.icon, action.text, self)
        self.ac_line.setToolTip(action.text)
        self.ac_line.setStatusTip(action.tooltip)
        self.ac_line.triggered.connect(action.perform)

    def create_rect_action(self):
        """ Add rectangle"""
        action = CreateToolAction(tool_factory=RectangleSceneTool,
                                  icon=icons.rect, text="Add rectangle",
                                  tooltip="Add rectangle to scene")
        self.ac_rect = QAction(action.icon, action.text, self)
        self.ac_rect.setToolTip(action.text)
        self.ac_rect.setStatusTip(action.tooltip)
        self.ac_rect.triggered.connect(action.perform)

    def create_scene_link_action(self):
        """ Add scene link"""
        action = CreateToolAction(tool_factory=SceneLinkTool,
                                  icon=icons.scenelink, text="Add scene link",
                                  tooltip="Add scene link to scene")
        self.ac_scene_link = QAction(action.icon, action.text, self)
        self.ac_scene_link.setToolTip(action.text)
        self.ac_scene_link.setStatusTip(action.tooltip)
        self.ac_scene_link.triggered.connect(action.perform)

    def create_group_actions(self):
        """ Grouping"""

    def create_select_all_action(self):
        """ Select all"""

    def create_cut_action(self):
        """ Cut"""

    def create_copy_action(self):
        """ Copy"""

    def create_paste_action(self):
        """ Paste"""

    def create_replace_action(self):
        """ Replace"""

    def create_delete_action(self):
        """ Delete"""

    def create_bring_to_front_action(self):
        """ Bring to front"""

    def create_send_to_back_action(self):
        """ Send to back"""
