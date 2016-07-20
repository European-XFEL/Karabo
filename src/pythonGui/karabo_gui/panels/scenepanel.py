#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtCore import pyqtSignal, pyqtSlot, Qt
from PyQt4.QtGui import (QAction, QActionGroup, QApplication, QKeySequence,
                         QMenu, QPalette, QScrollArea, QSizePolicy, QWidget)

from karabo_gui.docktabwindow import Dockable
import karabo_gui.icons as icons
from karabo_gui.sceneview.tools.api import (
    BoxVSceneAction, BoxHSceneAction, CreateToolAction,
    CreateWorkflowConnectionToolAction, GroupEntireSceneAction,
    GridSceneAction, GroupSceneAction, UngroupSceneAction, LineSceneTool,
    TextSceneTool, RectangleSceneTool, SceneBringToFrontAction,
    SceneCopyAction, SceneCutAction, SceneDeleteAction, SceneLinkTool,
    ScenePasteAction, ScenePasteReplaceAction, SceneSelectAllAction,
    SceneSendToBackAction, SceneSelectionTool)
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
        mode_qactions = self.create_mode_qactions()
        self._build_qaction_group(mode_qactions)
        self.qactions.extend(mode_qactions)
        self.qactions.append(self._build_separator())

        self.qactions.extend(self._build_qaction(a)
                             for a in self.create_tool_actions())
        self.qactions.append(self._build_separator())

        menu = QMenu()
        group_actions = self.create_group_tool_actions()
        for action in group_actions:
            q_action = self._build_qaction(action)
            menu.addAction(q_action)
        group_action = QAction(icons.group, "Group", self)
        group_action.setMenu(menu)
        self.qactions.extend([group_action, self._build_separator()])

        self.qactions.extend(self._build_qaction(a)
                             for a in self.create_clipboard_actions())
        self.qactions.append(self._build_separator())

        self.qactions.extend(self._build_qaction(a)
                             for a in self.create_order_actions())

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
        self.ac_selection_tool.setChecked(is_checked)
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

    def create_mode_qactions(self):
        """ Create mode QActions and return list of them"""
        selection = CreateToolAction(
            tool_factory=SceneSelectionTool,
            icon=icons.cursorArrow,
            checkable=True,
            text="Selection Mode",
            tooltip="Select objects in the scene"
        )
        workflow = CreateWorkflowConnectionToolAction(
            icon=icons.link,
            checkable=True,
            text="Connection Mode",
            tooltip="Connect workflow items"
        )
        q_actions = [self._build_qaction(a) for a in [selection, workflow]]
        # Save a reference to the SceneSelectionTool QAction
        self.ac_selection_tool = q_actions[0]
        return q_actions

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
                                        tooltip="Group selected items"))
        actions.append(BoxVSceneAction(icon=icons.groupVertical,
                                       text="Group Vertically",
                                       tooltip="Group selected items in "
                                               "vertical layout"))
        actions.append(BoxHSceneAction(icon=icons.groupHorizontal,
                                       text="Group Horizontally",
                                       tooltip="Group selected items in "
                                               "horizontal layout"))
        actions.append(GridSceneAction(icon=icons.groupGrid,
                                       text="Group in a Grid",
                                       tooltip="Group selected items in "
                                               "in a grid layout"))
        actions.append(UngroupSceneAction(icon=icons.ungroup,
                                          text="Ungroup",
                                          tooltip="Ungroup selected items"))
        actions.append(GroupEntireSceneAction(icon=icons.entireWindow,
                                              text="Group Entire Window",
                                              tooltip="Group all items in "
                                                      "entire window"))
        return actions

    def create_clipboard_actions(self):
        actions = []
        actions.append(SceneSelectAllAction(icon=icons.selectAll,
                                            shortcut=QKeySequence.SelectAll,
                                            text="Select All",
                                            tooltip="Select all items of the "
                                                    "scene"))
        actions.append(SceneCutAction(icon=icons.editCut,
                                      shortcut=QKeySequence.Cut,
                                      text="Cut",
                                      tooltip="Cut selected items"))
        actions.append(SceneCopyAction(icon=icons.editCopy,
                                       shortcut=QKeySequence.Copy,
                                       text="Copy",
                                       tooltip="Copy selected items"))
        actions.append(ScenePasteAction(icon=icons.editPaste,
                                        shortcut=QKeySequence.Paste,
                                        text="Paste",
                                        tooltip="Paste selected items"))
        actions.append(ScenePasteReplaceAction(icon=icons.editPasteReplace,
                                               shortcut=QKeySequence.Replace,
                                               text="Paste and Replace",
                                               tooltip="Paste and replace the "
                                                       "associated device ID "
                                                       "of selected items"))
        actions.append(SceneDeleteAction(icon=icons.delete,
                                         shortcut=QKeySequence.Delete,
                                         text="Delete",
                                         tooltip="Delete selected items"))
        return actions

    def create_order_actions(self):
        actions = []
        actions.append(SceneBringToFrontAction(icon=icons.bringToFront,
                                               text="Bring to Front",
                                               tooltip="Bring selected items "
                                                       "to front"))
        actions.append(SceneSendToBackAction(icon=icons.sendToBack,
                                             text="Send to Back",
                                             tooltip="Send selected items "
                                                     "to Back"))
        return actions

    def _build_qaction(self, sv_action):
        q_action = QAction(sv_action.icon, sv_action.text, self)
        q_action.setToolTip(sv_action.tooltip)
        q_action.setStatusTip(sv_action.tooltip)
        q_action.setCheckable(sv_action.checkable)
        q_action.triggered.connect(partial(sv_action.perform, self.scene_view))
        if sv_action.shortcut != QKeySequence.UnknownKey:
            q_action.setShortcut(QKeySequence(sv_action.shortcut))
        return q_action

    def _build_qaction_group(self, actions):
        group = QActionGroup(self)
        for ac in actions:
            group.addAction(ac)
        actions[0].setChecked(True)  # Mark the first action

    def _build_separator(self):
        q_action = QAction(self)
        q_action.setSeparator(True)
        return q_action

    def notifyTabVisible(self, visible):
        self.scene_view.set_tab_visible(visible)

    def onUndock(self):
        self.scene_view.set_tab_visible(True)
        osize = self.scene_view.size()
        screen_rect = QApplication.desktop().screenGeometry()
        if (osize.width() < screen_rect.width() and
                osize.height() < screen_rect.height()):
            # Enlarge the scene widget to its actual size
            self.setWidgetResizable(True)
            # Resize parent
            self.parent().resize(osize - self.scene_view.size() +
                                 self.parent().size())

    def onDock(self):
        self.setWidgetResizable(False)
