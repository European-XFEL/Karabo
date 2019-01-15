#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from contextlib import contextmanager
import time

from PyQt4.QtCore import pyqtSignal, pyqtSlot, QEvent, QSize, Qt, QTimer
from PyQt4.QtGui import (
    QPalette, QPainter, QPen, QSizePolicy, QStackedLayout, QWidget)

from karabo.common.scenemodel.api import (
    FixedLayoutModel, WorkflowItemModel, SCENE_MIN_WIDTH,
    SCENE_MIN_HEIGHT, SceneTargetWindow)
from karabogui import globals as krb_globals
from karabogui.events import (
    broadcast_event, KaraboEventSender, register_for_broadcasts,
    unregister_from_broadcasts)
from karabogui.generic_scenes import get_generic_scene
from karabogui.request import send_property_changes
from .bases import BaseSceneTool
from .builder import (
    bring_object_to_front, create_object_from_model, fill_root_layout,
    find_top_level_model, is_widget, iter_widgets_and_models,
    remove_object_from_layout, replace_model_in_top_level_model,
    send_object_to_back)
from .const import QT_CURSORS
from .layout.api import GroupLayout
from .selection_model import SceneSelectionModel
from .tools.api import (
    ConfigurationDropHandler, NavigationDropHandler, ProxySelectionTool,
    ProjectDropHandler, SceneControllerHandler, SceneSelectionTool,
    SceneToolHandler)
from .utils import save_painter_state
from .widget.api import ControllerContainer, WorkflowItemWidget
from .workflow.api import SceneWorkflowModel, WorkflowOverlay

# The scene widgets handler for mutations and action of the controllers and
# layout items on our scene
SCENE_WIDGET_HANDLER = (SceneControllerHandler, SceneToolHandler)

_WIDGET_REMOVAL_DELAY = 5000


def _get_time_milli():
    return int(round(time.time() * 1000))


class SceneView(QWidget):
    """An object representing the view for a Karabo GUI scene.
    """

    resetToSelectionTool = pyqtSignal()

    def __init__(self, model=None, design_mode=False, parent=None):
        super(SceneView, self).__init__(parent)

        layout_model = FixedLayoutModel(x=0, y=0, width=SCENE_MIN_WIDTH,
                                        height=SCENE_MIN_HEIGHT)
        # XXX: This is an interesting hack.
        # Basically, we want to put all of the scene's children into a widget
        # which is parented to this widget and completely covers it.
        # Then, the `design_mode` property toggles the mouse handling for this
        # child widget. In design mode, we want the main scene view and its
        # tools to get the mouse events. Out of design mode, we re-enable mouse
        # handling for all of the scene's children.
        self.layout = GroupLayout(layout_model, parent=self)
        self.setContentsMargins(0, 0, 0, 0)

        self.inner = QWidget(self)
        self.inner.setLayout(self.layout)
        self.inner.setFocusPolicy(Qt.StrongFocus)
        # Also create an overlay for the workflow connections
        self.overlay = WorkflowOverlay(self, parent=self)

        layout = QStackedLayout(self)
        layout.setStackingMode(QStackedLayout.StackAll)
        layout.addWidget(self.overlay)
        layout.addWidget(self.inner)

        self.scene_model = None
        self._ignore_resize_events = False
        self.selection_model = SceneSelectionModel()
        self.workflow_model = SceneWorkflowModel()

        # List of scene drag n drop handlers
        self.scene_handler_list = [ConfigurationDropHandler(),
                                   NavigationDropHandler(),
                                   ProjectDropHandler()]
        self.current_scene_handler = None

        self.current_tool = None
        # Use an internal flag to explicitly mark the design_mode, so when
        # other tools need the parent widget to handle mouse event won't
        # trigger the design_mode behavior
        self._design_mode = False
        self.design_mode = design_mode
        self.tab_visible = False
        self._scene_obj_cache = {}

        # Widget cleanup
        self._widget_removal_queue = []
        self._widget_removal_timer = QTimer(self)
        self._widget_removal_timer.setInterval(1000)
        self._widget_removal_timer.timeout.connect(self._clean_removed_widgets)

        # Redraw when the workflow model changes
        self.workflow_model.on_trait_change(lambda *args: self.update(),
                                            'updated')

        self.setFocusPolicy(Qt.StrongFocus)
        self.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        self.setAcceptDrops(True)
        self.setAttribute(Qt.WA_MouseTracking)
        self.setBackgroundRole(QPalette.Window)
        self.resize(SCENE_MIN_WIDTH, SCENE_MIN_HEIGHT)

        self.update_model(model)

        register_for_broadcasts(self)

    @property
    def design_mode(self):
        return self._design_mode

    @design_mode.setter
    def design_mode(self, value):
        self._design_mode = value
        # Toggle mouse handling in the inner view
        self.enable_mouse_event_handling(value)
        if not value:
            self.selection_model.clear_selection()
            self.set_tool(None)

    def enable_mouse_event_handling(self, enable):
        """Set whether delivery of mouse events to the widget and its children
        is either enabled or disabled.
        """
        self.inner.setAttribute(Qt.WA_TransparentForMouseEvents, enable)

    @contextmanager
    def ignore_resize_events(self):
        """Provide a context for explicitly disabling resize propagation to
        the data model.

        This is necessary because Qt will first set us to a size which it
        chooses when undocking, then give us a chance to set the size we
        actually want. In the meantime, it would be nice to disable updates
        so that our scene doesn't get modified.
        """
        try:
            self._ignore_resize_events = True
            yield
        finally:
            self._ignore_resize_events = False

    # ----------------------------
    # Qt Methods

    def mouseMoveEvent(self, event):
        if self.design_mode:
            self.current_tool.mouse_move(self, event)
            if event.isAccepted():
                self.update()
            else:
                super(SceneView, self).mouseMoveEvent(event)

    def mousePressEvent(self, event):
        proxy_selecting = isinstance(self.current_tool, ProxySelectionTool)
        if self.design_mode or proxy_selecting:
            self.current_tool.mouse_down(self, event)
            if event.isAccepted():
                self.update()
            else:
                super(SceneView, self).mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        if self.design_mode:
            self.current_tool.mouse_up(self, event)
            if event.isAccepted():
                self.update()
            else:
                super(SceneView, self).mouseReleaseEvent(event)

    def mouseDoubleClickEvent(self, event):
        if self.design_mode:
            item = self.item_at_position(event.pos())
            if item is not None and hasattr(item, 'edit'):
                item.edit(self)
                self.update()
            event.accept()
        else:
            item = self.controller_at_position(event.pos())
            if item is not None and not item.is_editable:
                proxy = item.widget_controller.proxy
                model = get_generic_scene(proxy)
                if model is not None:
                    window = SceneTargetWindow.Dialog
                    broadcast_event(KaraboEventSender.ShowUnattachedSceneView,
                                    {'model': model, 'target_window': window})

        super(SceneView, self).mouseDoubleClickEvent(event)

    def keyPressEvent(self, event):
        if event.key() == Qt.Key_Alt and not self.design_mode:
            if not self.inner.hasFocus():
                self.inner.setFocus()
            self.set_cursor('pointing-hand')
            self.set_tool(ProxySelectionTool())
            self.enable_mouse_event_handling(True)

    def keyReleaseEvent(self, event):
        if event.key() == Qt.Key_Alt and not self.design_mode:
            self.set_cursor('none')
            self.enable_mouse_event_handling(False)
            self.set_tool(None)

    def dragEnterEvent(self, event):
        if not self.design_mode:
            event.ignore()
            return

        for scene_handler in self.scene_handler_list:
            if scene_handler.can_handle(event):
                self.current_scene_handler = scene_handler
                event.accept()
                return
        event.ignore()

    def dropEvent(self, event):
        if self.current_scene_handler is None:
            event.ignore()
            return

        self.current_scene_handler.handle(self, event)
        self.current_scene_handler = None
        event.accept()

    def paintEvent(self, event):
        """Show view content.
        """
        with QPainter(self) as painter:
            self.layout.draw(painter)
            self._draw_selection(painter)
            if self.current_tool.visible:
                self.current_tool.draw(painter)

    def sizeHint(self):
        model = self.scene_model
        if model is None:
            return QSize()

        return QSize(model.width, model.height)

    def resizeEvent(self, event):
        model = self.scene_model
        if model is None:
            return

        if self._ignore_resize_events or not self.design_mode:
            # Often, we want to avoid modifying the model
            return

        size = event.size()
        model.width = size.width()
        model.height = size.height()

    def event(self, event):
        """This needs to be reimplemented to show tooltips not only in control
        mode but also in design mode.

        If the design mode is active the widget attribute
        ``WA_TransparentForMouseEvents`` is set to ``False`` which prevents the
        forwarding of the events to the actual widgets.
        """
        if event.type() == QEvent.ToolTip:
            widget = self.widget_at_position(event.pos())
            if widget is not None:
                return widget.event(event)
        return super(SceneView, self).event(event)

    def karaboBroadcastEvent(self, event):
        if event.sender in (KaraboEventSender.AlarmServiceInit,
                            KaraboEventSender.AlarmServiceUpdate):
            self._notify_alarm()
        elif event.sender is KaraboEventSender.AccessLevelChanged:
            self._update_widget_states()
        return False

    def contextMenuEvent(self, event):
        """Show scene view specific context menu. """
        if self.design_mode:
            widget = self.widget_at_position(event.pos())
            if widget is None:
                event.ignore()
                return
            # NOTE: An eventual handler blocks here until an action is selected
            for handler in SCENE_WIDGET_HANDLER:
                widget_handler = handler(widget=widget)
                if widget_handler.can_handle():
                    widget_handler.handle(self, event)
                    event.accept()
                    return

        event.ignore()

    # ----------------------------
    # Public methods

    def apply_editor_changes(self):
        """Apply user edits to a remote device instance
        """
        widgets = [obj for obj in self._scene_obj_cache.values()
                   if isinstance(obj, ControllerContainer)]
        proxies = [proxy for widget in widgets
                   for proxy in widget.widget_controller.proxies
                   if proxy.edit_value is not None]
        send_property_changes(proxies)

    def decline_editor_changes(self):
        """Decline user edits in editor widgets
        """
        for obj in self._scene_obj_cache.values():
            if is_widget(obj):
                obj.decline_changes()

    def destroy(self):
        """Do some cleanup of the scene's objects before death.
        """
        for obj in self._scene_obj_cache.values():
            if is_widget(obj):
                obj.destroy()
        self.workflow_model.destroy()

        unregister_from_broadcasts(self)
        self._set_scene_model(None)
        self._scene_obj_cache.clear()
        self._widget_removal_queue.clear()

    def set_tab_visible(self, visible):
        """Sets whether this scene is visible.

        This method manages the visibilities of the device properties in
        this scene.
        """
        if self.tab_visible == visible:
            return

        for obj in self._scene_obj_cache.values():
            # NOTE: check also if widget is visible due to the fact that we
            # only hide widgets with got replaced by 'Change Widget...'
            if is_widget(obj) and obj.isVisible():
                obj.set_visible(visible)
        self.workflow_model.set_visible(visible)

        self.tab_visible = visible

    def update_model(self, scene_model):
        if scene_model is None:
            return

        # Set scene model
        self._set_scene_model(scene_model)
        # Set width and height
        self.resize(self.scene_model.width, self.scene_model.height)

        self._scene_obj_cache = {}
        fill_root_layout(self.layout, self.scene_model, self.inner,
                         self._scene_obj_cache, self.tab_visible)

    def item_at_position(self, pos):
        """Returns the topmost object whose bounds contain `pos`."""
        for child in self.scene_model.children[::-1]:
            obj = self._scene_obj_cache.get(child)
            if obj is not None and obj.geometry().contains(pos):
                return obj

    def controller_at_position(self, pos):
        """Returns the topmost controller whose bounds contain `pos`."""
        widget = self.inner.childAt(pos)
        while (widget is not None
               and not isinstance(widget, ControllerContainer)):
            widget = widget.parent()

        return widget

    def widget_at_position(self, pos):
        """Returns the topmost widget whose bounds contain `pos`."""
        widget = self.inner.childAt(pos)
        if widget is not None:
            while not is_widget(widget):
                widget = widget.parent()
        return widget

    def workflow_at_position(self, pos):
        """Returns the topmost workflow widget whose bounds contain `pos`."""
        widget = self.inner.childAt(pos)
        while (widget is not None
                and not isinstance(widget, WorkflowItemWidget)):
            widget = widget.parent()
        return widget

    def items_in_rect(self, rect):
        """Returns the topmost objects whose bounds are contained in `rect`."""
        items = []
        for child in self.scene_model.children[::-1]:
            obj = self._scene_obj_cache.get(child)
            if obj is not None and rect.contains(obj.geometry()):
                items.append(obj)
        return items

    def select_all(self):
        """Add all children to the current selection"""
        self.selection_model.clear_selection()
        for child in self.scene_model.children:
            obj = self._scene_obj_cache.get(child)
            if obj is not None:
                self.selection_model.select_object(obj)
        self.update()

    def select_model(self, model):
        """Select widget object for the given ``model``."""
        self.selection_model.clear_selection()
        obj = self._scene_obj_cache.get(model)
        if obj is not None:
            self.selection_model.select_object(obj)
        self.update()

    def set_cursor(self, name):
        """Sets the cursor for the scene view."""
        if name == 'none':
            self.unsetCursor()
        else:
            self.setCursor(QT_CURSORS[name])

    def set_tool(self, tool):
        """Sets the current tool being used by the view."""
        assert tool is None or isinstance(tool, BaseSceneTool)
        self.current_tool = tool
        if tool is None:
            if self.design_mode:
                self.resetToSelectionTool.emit()
            else:
                self.set_cursor('none')
                self.current_tool = SceneSelectionTool()

        self.update()

    def add_models(self, *models):
        """Adds new child models to the scene model."""
        self.scene_model.children.extend(models)

    def remove_model(self, model):
        """Removes the given ``model`` from the scene model."""
        self.scene_model.children.remove(model)

    def replace_model(self, old_model, new_model):
        """Replace the given ``old_model`` with the given ``new_model``."""
        # Find top level model to which ``old_model`` belongs
        top_level_model = find_top_level_model(self.scene_model, old_model)
        if top_level_model is None:
            return

        # Remove it from list
        self.remove_model(top_level_model)
        if top_level_model is not old_model:
            # Do the replacing in the top level model tree
            replace_model_in_top_level_model(self.scene_model,
                                             top_level_model,
                                             old_model, new_model)
        else:
            top_level_model = new_model
        # Add the modified top level model to the scene again
        self.add_models(top_level_model)
        self.selection_model.clear_selection()

    def bring_to_front(self, model):
        """The given ``model`` is moved to the end of the list."""
        # Remove model
        self.remove_model(model)
        # Add model to end
        self.add_models(model)

        scene_obj = self._scene_obj_cache.get(model)
        if scene_obj is not None:
            # In case of layouts or widgets
            bring_object_to_front(scene_obj)

    def send_to_back(self, model):
        """The given ``model`` is moved to the beginning of the list."""
        # Copy list to trigger traits notification only once when its set at
        # the end
        children = list(self.scene_model.children)
        # Clean list to redraw all models in correct order
        del self.scene_model.children[:]
        # Remove model
        children.remove(model)
        # Insert model at beginning
        children.insert(0, model)
        self.scene_model.children.extend(children)

        scene_obj = self._scene_obj_cache.get(model)
        if scene_obj is not None:
            # In case of layouts or widgets
            send_object_to_back(scene_obj)

    def mouse_left(self):
        """mouse left scene view area"""
        if not self.design_mode:
            # Deactivate special tools if not in design mode
            self.set_cursor('none')
            self.set_tool(None)
            self.enable_mouse_event_handling(False)

    # --------------------------------------------------------------------
    # Private methods

    @pyqtSlot()
    def _clean_removed_widgets(self):
        """Called by the `_widget_removal_timer` when there might be widgets to
        clean up.
        """
        remainders = []
        current_time = _get_time_milli()
        while len(self._widget_removal_queue) > 0:
            removal_time, widget, model = self._widget_removal_queue.pop()
            if widget.isVisible():
                continue  # already re-added. ignore

            if (current_time - removal_time) > _WIDGET_REMOVAL_DELAY:
                # Time's up!
                widget.destroy()
                widget.setParent(None)
                self._scene_obj_cache.pop(model, None)
            else:
                remainders.append((removal_time, widget, model))

        self._widget_removal_queue.extend(remainders)
        if len(self._widget_removal_queue) == 0:
            self._widget_removal_timer.stop()

    def _collect_removed_widgets(self, obj, model):
        """Collect widgets for the `widget_removal_queue`.
        """
        for item in iter_widgets_and_models(obj, model, self._scene_obj_cache):
            # Schedule the (possible) eventual cleanup of the widget
            removal_item = (_get_time_milli(),) + item
            self._widget_removal_queue.append(removal_item)

        if (self._widget_removal_queue and
                not self._widget_removal_timer.isActive()):
            self._widget_removal_timer.start()

    def _draw_selection(self, painter):
        """Draw a dashed rect around the selected objects."""
        if not self.selection_model.has_selection():
            return

        black = QPen(Qt.black)
        black.setStyle(Qt.DashLine)
        white = QPen(Qt.white)

        rect = self.selection_model.get_selection_bounds()
        with save_painter_state(painter):
            painter.setPen(white)
            painter.drawRect(rect)
            painter.setPen(black)
            painter.drawRect(rect)

    def _set_scene_model(self, scene_model):
        """The scene model is set and all notification handlers are defined.
        """
        if self.scene_model is not None:
            self.scene_model.on_trait_change(self._model_modified,
                                             'children_items', remove=True)

        self.scene_model = scene_model
        if scene_model is None:
            return

        self.scene_model.on_trait_change(self._model_modified,
                                         'children_items')
        self._add_workflow_items(self.scene_model.children)

    def _model_modified(self, event):
        """The scene model got modified."""
        for model in event.removed:
            obj = self._scene_obj_cache.get(model)
            if obj is not None:
                # Collect widgets _before_ removing
                self._collect_removed_widgets(obj, model)
                # Then remove
                remove_object_from_layout(obj, self.layout,
                                          self._scene_obj_cache,
                                          self.tab_visible)

        self._remove_workflow_items(event.removed)

        for model in event.added:
            create_object_from_model(self.layout, model, self.inner,
                                     self._scene_obj_cache, self.tab_visible)
        self._add_workflow_items(event.added)

    def _add_workflow_items(self, models):
        """Add new WorkflowItemModel instances to the workflow model."""
        items = [m for m in models if isinstance(m, WorkflowItemModel)]
        self.workflow_model.add_items(items)

    def _remove_workflow_items(self, models):
        """Remove WorkflowItemModel instances from the workflow model."""
        items = [m for m in models if isinstance(m, WorkflowItemModel)]
        self.workflow_model.remove_items(items)

    def _notify_alarm(self):
        """Update alarm indicators of widgets in need"""
        for obj in self._scene_obj_cache.values():
            if is_widget(obj):
                obj.update_alarm()

    def _update_widget_states(self):
        """The global access level has changed. Notify all widgets in the
        scene.
        """
        level = krb_globals.GLOBAL_ACCESS_LEVEL
        for obj in self._scene_obj_cache.values():
            if is_widget(obj):
                obj.update_global_access_level(level)
