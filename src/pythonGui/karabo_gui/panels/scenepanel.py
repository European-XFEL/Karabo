#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtCore import pyqtSignal, pyqtSlot, Qt
from PyQt4.QtGui import (QAction, QKeySequence, QMenu, QPalette, QScrollArea,
                         QSizePolicy, QWidget)

from karabo_gui.docktabwindow import Dockable
import karabo_gui.icons as icons
from karabo_gui.sceneview.tools.api import (
    BoxVSceneAction, BoxHSceneAction, CreateToolAction, GroupEntireSceneAction,
    GridSceneAction, GroupSceneAction, UngroupSceneAction, LineSceneTool,
    TextSceneTool, RectangleSceneTool, SceneBringToFrontAction,
    SceneCopyAction, SceneCutAction, SceneDeleteAction, SceneLinkTool,
    ScenePasteAction, SceneSelectAllAction, SceneSendToBackAction)
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

    def closeEvent(self, event):
        event.accept()
        self.scene_view.destroy()
        self.signalClosed.emit()

    def design_mode_text(self, is_design_mode):
        if is_design_mode:
            return "Change to control mode"

        return "Change to design mode"

    def setupActions(self, connected_to_server):
        self.create_design_mode_action(connected_to_server)

        self.qactions = []
        tool_actions = self.create_tool_actions()
        tool_qactions = [self._build_qaction(a) for a in tool_actions]
        self.qactions.extend(tool_qactions)

        self.qactions.append(self._build_separator())

        menu = QMenu()
        group_actions = self.create_group_tool_actions()
        for action in group_actions:
            q_action = self._build_qaction(action)
            menu.addAction(q_action)
        group_action = QAction(icons.group, "Group", self)
        group_action.setMenu(menu)
        self.qactions.append(group_action)

        self.qactions.append(self._build_separator())

        clipboard_actions = self.create_clipboard_actions()
        clipboard_qactions = [self._build_qaction(a)
                              for a in clipboard_actions]
        self.qactions.extend(clipboard_qactions)

        self.qactions.append(self._build_separator())
        order_actions = self.create_order_actions()
        order_qactions = [self._build_qaction(a) for a in order_actions]
        self.qactions.extend(order_qactions)

    def setupToolBars(self, standardToolBar, parent):
        standardToolBar.addAction(self.ac_design_mode)

        tool_bar = ToolBar("Drawing")
        tool_bar.setVisible(self.scene_view.design_mode)
        parent.addToolBar(tool_bar)
        self.scene_view.setFocusProxy(tool_bar)
        tool_bar.setFocusPolicy(Qt.StrongFocus)

        tool_bar.addSeparator()
        for action in self.qactions:
            tool_bar.addAction(action)

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

    def create_tool_actions(self):
        """ Create tool actions and return list of them"""
        actions = []
        actions.append(CreateToolAction(tool_factory=TextSceneTool,
                                        icon=icons.text, text="Add text",
                                        tooltip="Add text to scene"))
        actions.append(CreateToolAction(tool_factory=LineSceneTool,
                                        icon=icons.line, text="Add line",
                                        tooltip="Add line to scene"))
        actions.append(CreateToolAction(tool_factory=RectangleSceneTool,
                                        icon=icons.rect, text="Add rectangle",
                                        tooltip="Add rectangle to scene"))
        actions.append(CreateToolAction(tool_factory=SceneLinkTool,
                                        icon=icons.scenelink,
                                        text="Add scene link",
                                        tooltip="Add scene link to scene"))
        return actions

    def create_group_tool_actions(self):
        actions = []
        actions.append(GroupSceneAction(icon=icons.group,
                                        text="Group without layout",
                                        tooltip="Group without layout"))
        actions.append(BoxVSceneAction(icon=icons.groupVertical,
                                       text="Group Vertically",
                                       tooltip="Group Vertically"))
        actions.append(BoxHSceneAction(icon=icons.groupHorizontal,
                                       text="Group Horizontally",
                                       tooltip="Group Horizontally"))
        actions.append(GridSceneAction(icon=icons.groupGrid,
                                       text="Group in a Grid",
                                       tooltip="Group in a Grid"))
        actions.append(UngroupSceneAction(icon=icons.ungroup,
                                          text="Ungroup",
                                          tooltip="Ungroup"))
        actions.append(GroupEntireSceneAction(icon=icons.entireWindow,
                                              text="Group Entire Window",
                                              tooltip="Group Entire Window"))
        return actions

    def create_clipboard_actions(self):
        actions = []
        actions.append(SceneSelectAllAction(icon=icons.selectAll,
                                            shortcut=QKeySequence.SelectAll,
                                            text="Select All",
                                            tooltip="Select All"))
        actions.append(SceneCutAction(icon=icons.editCut,
                                      shortcut=QKeySequence.Cut,
                                      text="Cut", tooltip="Cut"))
        actions.append(SceneCopyAction(icon=icons.editCopy,
                                       shortcut=QKeySequence.Copy,
                                       text="Copy", tooltip="Copy"))
        actions.append(ScenePasteAction(icon=icons.editPaste,
                                        shortcut=QKeySequence.Paste,
                                        text="Paste", tooltip="Paste"))
        actions.append(SceneDeleteAction(icon=icons.delete,
                                         shortcut=QKeySequence.Delete,
                                         text="Delete", tooltip="Delete"))
        return actions

    def create_order_actions(self):
        actions = []
        actions.append(SceneBringToFrontAction(icon=icons.bringToFront,
                                               text="Bring to Front",
                                               tooltip="Bring to Front"))
        actions.append(SceneSendToBackAction(icon=icons.sendToBack,
                                             text="Send to Back",
                                             tooltip="Send to Back"))
        return actions

    def _build_qaction(self, sv_action):
        q_action = QAction(sv_action.icon, sv_action.text, self)
        q_action.setToolTip(sv_action.text)
        q_action.setStatusTip(sv_action.tooltip)
        q_action.triggered.connect(partial(sv_action.perform, self.scene_view))
        if sv_action.shortcut != QKeySequence.UnknownKey:
            q_action.setShortcut(QKeySequence(sv_action.shortcut))
        return q_action

    def _build_separator(self):
        q_action = QAction(self)
        q_action.setSeparator(True)
        return q_action
