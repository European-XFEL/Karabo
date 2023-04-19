#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
from functools import partial

from qtpy.QtCore import QEvent, QSize, Qt, Slot
from qtpy.QtGui import QKeySequence, QPalette
from qtpy.QtWidgets import (
    QAction, QActionGroup, QApplication, QDialog, QMenu, QScrollArea)

from karabogui import icons
from karabogui.dialogs.api import ResizeSceneDialog
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.indicators import get_topic_color
from karabogui.sceneview.api import SceneView
from karabogui.sceneview.const import QT_CURSORS, SCENE_BORDER_WIDTH
from karabogui.sceneview.tools.api import (
    ArrowSceneTool, BoxHSceneAction, BoxVSceneAction, CreateToolAction,
    DeviceSceneLinkTool, GroupEntireSceneAction, GroupSceneAction,
    ImageRendererTool, InstanceStatusTool, LineSceneTool, RectangleSceneTool,
    SceneAlignAction, SceneBringToFrontAction, SceneCopyAction, SceneCutAction,
    SceneDeleteAction, SceneLinkTool, SceneMoveAction, ScenePasteAction,
    ScenePasteReplaceAction, SceneSelectAllAction, SceneSelectionTool,
    SceneSendToBackAction, StickerTool, TextSceneTool, UngroupSceneAction,
    WebLinkTool)
from karabogui.util import move_to_cursor
from karabogui.widgets.toolbar import ToolBar

from .base import BasePanelWidget

# NOTE: This is the amount of padding added by ScenePanel's QFrame parent
# We need to take it into account when undocking!!
QFRAME_PADDING = 4


class ScenePanel(BasePanelWidget):
    def __init__(self, model, connected_to_server):
        # cache a reference to the scene model
        self.model = model
        self.connected_to_server_at_init = connected_to_server
        super(ScenePanel, self).__init__(model.simple_name, allow_closing=True)

        # Add a handler for the window title. Don't forget to remove it later!
        self.model.on_trait_change(self.set_title, 'simple_name')

    # ----------------------------
    # BasePanelWidget Methods

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        self.scene_view = SceneView(model=self.model, parent=self)
        self.scroll_widget = ResizableScrollArea(self.scene_view, parent=self)
        self.scroll_widget.setWidget(self.scene_view)
        self.scroll_widget.setBackgroundRole(QPalette.Dark)
        self.scroll_widget.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        self.scroll_widget.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        self.scroll_widget.setContentsMargins(0, 0, 0, 0)
        return self.scroll_widget

    def dock(self):
        """Called before this panel is docked into the main window.
        """
        self.scroll_widget.setWidgetResizable(False)

    def undock(self, maximized=False):
        """Called before this panel is undocked from the main window.
        """
        width, height = self._compute_panel_size()
        screen_rect = QApplication.desktop().screenGeometry()
        scene_view = self.scene_view
        if (width < screen_rect.width() and height < screen_rect.height()):
            # Resize panel
            self.resize(width, height)
            # Enlarge the scene widget to its actual size
            with scene_view.ignore_resize_events():
                self.scroll_widget.setWidgetResizable(True)

    def toolbars(self):
        """This should create and return one or more `ToolBar` instances needed
        by this panel.
        """
        self.create_toolbar_actions(self.connected_to_server_at_init)

        always_visible_tb = ToolBar(parent=self)
        always_visible_tb.addAction(self.ac_design_mode)
        always_visible_tb.addSeparator()
        always_visible_tb.addAction(self.ac_apply_all)
        always_visible_tb.addAction(self.ac_decline_all)
        always_visible_tb.add_expander()

        tool_bar = ToolBar("Drawing", parent=self)
        tool_bar.setVisible(self.scene_view.design_mode)
        self.scene_view.setFocusProxy(tool_bar)
        tool_bar.setFocusPolicy(Qt.StrongFocus)

        tool_bar.addSeparator()
        for action in self.qactions:
            tool_bar.addAction(action)

        self.drawing_tool_bar = tool_bar
        # Add placeholder widget to toolbar
        self.drawing_tool_bar.add_expander()

        # Reset to selection tool when no current tool is selected
        self.scene_view.resetToSelectionTool.connect(
            self.ac_selection_tool.trigger)

        return [always_visible_tb, self.drawing_tool_bar]

    # ----------------------------
    # Qt Methods

    def closeEvent(self, event):
        super(ScenePanel, self).closeEvent(event)
        if event.isAccepted():
            # Tell the scene view and scroll widget to destruct
            if self.scene_view is not None:
                self.scene_view.setParent(None)
                self.scene_view.destroy()
                self.scene_view = None
            if self.scroll_widget is not None:
                self.scroll_widget.setParent(None)
                self.scroll_widget.destroy()
                self.scroll_widget = None
            # Remove the window title handler
            self.model.on_trait_change(self.set_title, 'simple_name',
                                       remove=True)
            # Tell the world we're closing
            broadcast_event(KaraboEvent.MiddlePanelClosed,
                            {'model': self.model})

    def hideEvent(self, event):
        if self.scene_view is not None:
            self.scene_view.set_tab_visible(False)

    def showEvent(self, event):
        if self.scene_view is not None:
            self.scene_view.set_tab_visible(True)

        # When undocked, make sure our panel has an appropriate size
        if not self.is_docked:
            width, height = self._compute_panel_size()
            self.resize(width, height)
            if self.scroll_widget is not None:
                self.scroll_widget.setWidgetResizable(True)

    def set_toolbar_style(self, karabo_topic):
        color = get_topic_color(karabo_topic)
        if color is not None:
            self.toolbar.setStyleSheet(
                """QToolBar {{ background-color: rgba{} }};""".format(color))

    # ----------------------------
    # Qt slots

    @Slot()
    def apply_editor_changes(self):
        self.scene_view.apply_editor_changes()

    @Slot()
    def decline_editor_changes(self):
        self.scene_view.decline_editor_changes()

    @Slot(bool)
    def toggle_snap_to_grid(self, is_checked):
        self.scene_view.snap_to_grid = is_checked

    @Slot()
    def resize_scene(self):
        dialog = ResizeSceneDialog(self.scene_view.size(), parent=self)
        move_to_cursor(dialog)
        if dialog.exec() != QDialog.Accepted:
            return

        self.scene_view.resize(dialog.scene_size)
        if not self.is_docked:
            width, height = self._compute_panel_size()
            self.resize(width, height)

    @Slot(bool)
    def design_mode_changed(self, is_checked):
        self.drawing_tool_bar.setVisible(is_checked)
        text = self.design_mode_text(is_checked)
        self.ac_design_mode.setToolTip(text)
        self.ac_design_mode.setStatusTip(text)
        self.ac_selection_tool.setChecked(is_checked)
        self.scene_view.design_mode = is_checked

    def design_mode_text(self, is_design_mode):
        if is_design_mode:
            return "Change to control mode"

        return "Change to design mode"

    # ----------------------------

    def setReadOnly(self, value):
        """This method is externally called for `AccessLevel` dependent view"""
        if not value and self.scene_view.design_mode:
            self.ac_design_mode.setChecked(False)
        self.ac_design_mode.setVisible(value)

    # ----------------------------
    # ToolBar and Action creators

    def create_toolbar_actions(self, connected_to_server):
        text = self.design_mode_text(self.scene_view.design_mode)
        self.ac_design_mode = QAction(icons.transform, text, self)
        self.ac_design_mode.setToolTip(text)
        self.ac_design_mode.setStatusTip(text)
        self.ac_design_mode.setCheckable(True)
        self.ac_design_mode.setChecked(self.scene_view.design_mode)
        self.ac_design_mode.setEnabled(connected_to_server)
        self.ac_design_mode.toggled.connect(self.design_mode_changed)

        self.ac_apply_all = QAction(icons.apply, 'Apply All Changes', self)
        self.ac_apply_all.triggered.connect(self.apply_editor_changes)
        self.ac_decline_all = QAction(icons.no, 'Decline All Changes', self)
        self.ac_decline_all.triggered.connect(self.decline_editor_changes)

        self.qactions = []
        mode_qactions = self.create_mode_qactions()
        tool_qactions = [self._build_qaction(a)
                         for a in self.create_tool_actions()]
        link_qactions = [self._build_qaction(a)
                         for a in self.create_link_actions()]

        self._build_qaction_group(
            mode_qactions + tool_qactions + link_qactions)
        self.qactions.extend(mode_qactions)
        self.qactions.append(self._build_separator())

        self.qactions.extend(self.create_scene_tool_actions())
        self.qactions.append(self._build_separator())

        self.qactions.extend(tool_qactions)
        self.qactions.append(self._build_separator())

        link_menu = QMenu()
        for q_action in link_qactions:
            link_menu.addAction(q_action)
        link_action = QAction(icons.scenelink, "Link", self)
        link_action.setMenu(link_menu)
        self.qactions.extend([link_action, self._build_separator()])

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

        menu = QMenu()
        move_actions = self.create_move_actions()
        for action in move_actions:
            q_action = self._build_qaction(action)
            menu.addAction(q_action)
        move_action = QAction(icons.move, "Move", self)
        move_action.setMenu(menu)
        self.qactions.extend([move_action, self._build_separator()])

        menu = QMenu()
        align_actions = self.create_align_actions()
        for action in align_actions:
            q_action = self._build_qaction(action)
            menu.addAction(q_action)
        align_action = QAction(icons.alignLeft, "Align", self)
        align_action.setMenu(menu)
        self.qactions.extend([align_action, self._build_separator()])

        self.qactions.extend(self._build_qaction(a)
                             for a in self.create_order_actions())

    def create_mode_qactions(self):
        """ Create mode QActions and return list of them"""
        selection = CreateToolAction(
            tool_factory=SceneSelectionTool,
            icon=icons.cursorArrow,
            checkable=True,
            text="Selection Mode",
            tooltip="Select objects in the scene"
        )
        q_actions = [self._build_qaction(a) for a in [selection]]
        # Save a reference to the SceneSelectionTool QAction
        self.ac_selection_tool = q_actions[0]
        return q_actions

    def create_scene_tool_actions(self):
        show_grid_action = QAction(icons.grid, 'Snap to Grid', self)
        show_grid_action.setCheckable(True)
        show_grid_action.setChecked(self.scene_view.snap_to_grid)
        show_grid_action.triggered.connect(self.toggle_snap_to_grid)

        resize_scene_action = QAction(icons.resize, 'Resize Scene', self)
        resize_scene_action.triggered.connect(self.resize_scene)

        return [show_grid_action, resize_scene_action]

    def create_link_actions(self):
        """ Create link actions and return list of them"""
        actions = []
        actions.append(CreateToolAction(tool_factory=SceneLinkTool,
                                        icon=icons.scenelink,
                                        text="Add scene link",
                                        tooltip="Add scene link to scene",
                                        checkable=True))
        actions.append(CreateToolAction(tool_factory=WebLinkTool,
                                        icon=icons.weblink,
                                        text="Add web link",
                                        tooltip="Add web link to scene",
                                        checkable=True))
        actions.append(CreateToolAction(tool_factory=DeviceSceneLinkTool,
                                        icon=icons.deviceInstance,
                                        text="Add device scene link",
                                        tooltip="Add device link to scene",
                                        checkable=True))
        return actions

    def create_tool_actions(self):
        """ Create tool actions and return list of them"""
        actions = []
        actions.append(CreateToolAction(tool_factory=TextSceneTool,
                                        icon=icons.text, text="Add text",
                                        tooltip="Add text to scene",
                                        checkable=True))
        actions.append(CreateToolAction(tool_factory=LineSceneTool,
                                        icon=icons.line, text="Add line",
                                        tooltip="Add line to scene",
                                        checkable=True))
        actions.append(CreateToolAction(tool_factory=ArrowSceneTool,
                                        icon=icons.arrow,
                                        text="Add arrow",
                                        tooltip="Add arrow to scene",
                                        checkable=True))
        actions.append(CreateToolAction(tool_factory=RectangleSceneTool,
                                        icon=icons.rect, text="Add rectangle",
                                        tooltip="Add rectangle to scene",
                                        checkable=True))
        actions.append(CreateToolAction(tool_factory=StickerTool,
                                        icon=icons.sticker,
                                        text="Add a sticker widget",
                                        tooltip="Add sticker to scene",
                                        checkable=True))
        actions.append(CreateToolAction(tool_factory=ImageRendererTool,
                                        icon=icons.image,
                                        text="Add an image to the scene",
                                        tooltip="Add image to scene",
                                        checkable=True))
        actions.append(CreateToolAction(tool_factory=InstanceStatusTool,
                                        icon=icons.statusOk,
                                        text="Add instance status",
                                        tooltip="Add instance status to scene",
                                        checkable=True))
        return actions

    def create_group_tool_actions(self):
        actions = []
        actions.append(GroupSceneAction(
            icon=icons.group,
            text="Group in Fixed Layout",
            shortcut=QKeySequence(Qt.CTRL + Qt.Key_G),
            tooltip="Group selected items"))
        actions.append(BoxVSceneAction(
            icon=icons.groupVertical,
            text="Group Vertically",
            tooltip="Group selected items in "
                    "vertical layout"))
        actions.append(BoxHSceneAction(
            icon=icons.groupHorizontal,
            text="Group Horizontally",
            tooltip="Group selected items in "
                    "horizontal layout"))
        actions.append(UngroupSceneAction(
            icon=icons.ungroup,
            shortcut=QKeySequence(Qt.CTRL + Qt.SHIFT + Qt.Key_G),
            text="Ungroup",
            tooltip="Ungroup selected items"))
        actions.append(GroupEntireSceneAction(
            icon=icons.entireWindow,
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

    def create_move_actions(self):
        actions = []

        actions.append(SceneMoveAction(
            icon=icons.arrowLeft,
            shortcut=QKeySequence.MoveToPreviousChar,
            text="Left",
            tooltip="Move item to the left"))
        actions.append(SceneMoveAction(
            icon=icons.arrowRight,
            shortcut=QKeySequence.MoveToNextChar,
            text="Right",
            tooltip="Move item to the right"))
        actions.append(SceneMoveAction(
            icon=icons.arrowUp,
            shortcut=QKeySequence.MoveToPreviousLine,
            text="Up",
            tooltip="Move item up"))
        actions.append(SceneMoveAction(
            icon=icons.arrowDown,
            shortcut=QKeySequence.MoveToNextLine,
            text="Down",
            tooltip="Move item down"))

        return actions

    def create_align_actions(self):
        actions = []

        actions.append(SceneAlignAction(
            icon=icons.alignLeft,
            text="Left",
            tooltip="Align items to left"))
        actions.append(SceneAlignAction(
            icon=icons.alignRight,
            text="Right",
            tooltip="Align items to right"))
        actions.append(SceneAlignAction(
            icon=icons.alignTop,
            text="Top",
            tooltip="Align items to top"))
        actions.append(SceneAlignAction(
            icon=icons.alignBottom,
            text="Bottom",
            tooltip="Align items to bottom"))

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
            if isinstance(sv_action.shortcut, QKeySequence):
                q_action.setShortcut(sv_action.shortcut)
            else:
                q_action.setShortcuts(
                    QKeySequence.keyBindings(sv_action.shortcut))
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

    def _compute_panel_size(self):
        tb_height = 0
        if self.toolbar.isVisible():
            tb_height = self.toolbar.height()
        frame_width = self.model.width + QFRAME_PADDING
        frame_height = self.model.height + tb_height + QFRAME_PADDING
        return (frame_width, frame_height)


class ResizableScrollArea(QScrollArea):
    """A QScrollArea which enables resizing of its child widget via mouse
    interaction
    """

    def __init__(self, child_widget, parent=None):
        super(ResizableScrollArea, self).__init__(parent=parent)

        self._resizing = False
        self._resize_type = ''
        self._child_widget = child_widget
        self._child_widget.installEventFilter(self)
        self.setMouseTracking(True)

    def eventFilter(self, obj, event):
        event_type = event.type()
        if event_type == QEvent.Enter:
            # Unset cursor when mouse is over child
            self.unsetCursor()
        elif event_type == QEvent.Leave:
            self._child_widget.mouse_left()
        return super(ResizableScrollArea, self).eventFilter(obj, event)

    def mouseMoveEvent(self, event):
        if not event.buttons():
            mouse_pos = event.pos()
            size = self._child_widget.size()
            dx = mouse_pos.x() - size.width()
            dy = mouse_pos.y() - size.height()
            right = 0 < dx < SCENE_BORDER_WIDTH
            bottom = 0 < dy < SCENE_BORDER_WIDTH
            if bottom and right:
                cursor = 'resize-diagonal-tlbr'
                self._resize_type = 'br'
            elif bottom and dx <= 0:
                cursor = 'resize-vertical'
                self._resize_type = 'b'
            elif right and dy <= 0:
                cursor = 'resize-horizontal'
                self._resize_type = 'r'
            else:
                cursor = 'arrow'
                self._resize_type = ''

            if not self._child_widget.design_mode:
                # Disallow unless the scene is in design mode!
                cursor = 'arrow'
                self._resize_type = ''

            self.setCursor(QT_CURSORS[cursor])
        elif self._resizing:
            mouse_pos = event.pos()
            size = QSize(self._child_widget.size())
            if "b" in self._resize_type:
                size.setHeight(mouse_pos.y())
            if "r" in self._resize_type:
                size.setWidth(mouse_pos.x())
            if (size.width() < SCENE_BORDER_WIDTH or
                    size.height() < SCENE_BORDER_WIDTH):
                return
            self._child_widget.resize(size)

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton and self._resize_type != '':
            self._resizing = True
            event.accept()

    def mouseReleaseEvent(self, event):
        if self._resizing:
            self._resizing = False
            self._resize_type = ''
            self.unsetCursor()
