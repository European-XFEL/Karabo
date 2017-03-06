#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QEvent, Qt
from PyQt4.QtGui import (QPalette, QPainter, QPen, QSizePolicy, QStackedLayout,
                         QWidget)

from karabo.common.scenemodel.api import (
    FixedLayoutModel, WorkflowItemModel, SCENE_MIN_WIDTH, SCENE_MIN_HEIGHT)
from karabo_gui.events import (
    KaraboEventSender, register_for_broadcasts, unregister_from_broadcasts
)
from .bases import BaseSceneTool
from .builder import (bring_object_to_front, create_object_from_model,
                      fill_root_layout, find_top_level_model, is_widget,
                      remove_object_from_layout,
                      replace_model_in_top_level_model, send_object_to_back)
from .const import QT_CURSORS
from .layout.api import GroupLayout
from .selection_model import SceneSelectionModel
from .tools.api import (ConfigurationDropHandler, NavigationDropHandler,
                        SceneSelectionTool, WidgetSceneHandler)
from .utils import save_painter_state
from .workflow.api import SceneWorkflowModel, WorkflowOverlay


class SceneView(QWidget):
    """ An object representing the view for a Karabo GUI scene.
    """
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
        # Also create an overlay for the workflow connections
        self.overlay = WorkflowOverlay(self, parent=self)

        layout = QStackedLayout(self)
        layout.setStackingMode(QStackedLayout.StackAll)
        layout.addWidget(self.overlay)
        layout.addWidget(self.inner)

        self.scene_model = None
        self.selection_model = SceneSelectionModel()
        self.workflow_model = SceneWorkflowModel()

        # List of scene drag n drop handlers
        self.scene_handler_list = [ConfigurationDropHandler(),
                                   NavigationDropHandler()]
        self.current_scene_handler = None

        self.current_tool = None
        self.design_mode = design_mode
        self.tab_visible = False
        self._scene_obj_cache = {}

        # Redraw when the workflow model changes
        self.workflow_model.on_trait_change(lambda *args: self.update(),
                                            'updated')

        self.setFocusPolicy(Qt.StrongFocus)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.setAcceptDrops(True)
        self.setAttribute(Qt.WA_MouseTracking)
        self.setBackgroundRole(QPalette.Window)
        self.resize(SCENE_MIN_WIDTH, SCENE_MIN_HEIGHT)

        self.update_model(model)

        register_for_broadcasts(self)

    @property
    def design_mode(self):
        # If the inner view is transparent to events, this view is handling
        return self.inner.testAttribute(Qt.WA_TransparentForMouseEvents)

    @design_mode.setter
    def design_mode(self, value):
        # Toggle mouse handling in the inner view
        self.inner.setAttribute(Qt.WA_TransparentForMouseEvents, value)
        if not value:
            self.selection_model.clear_selection()
            self.set_tool(None)

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
        if self.design_mode:
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
        """ Show view content.
        """
        with QPainter(self) as painter:
            self.layout.draw(painter)
            self._draw_selection(painter)
            if self.current_tool.visible:
                self.current_tool.draw(painter)

    def resizeEvent(self, event):
        model = self.scene_model
        if model is None:
            return

        size = event.size()
        model.width = size.width()
        model.height = size.height()

    def event(self, event):
        """ This needs to be reimplemented to show tooltips not only in control
            mode but also in design mode.

            If the design mode is active the widget attribute
            ``WA_TransparentForMouseEvents`` is set to ``False`` which prevents
            the forwarding of the events to the actual widgets.
        """
        if event.type() == QEvent.ToolTip:
            widget = self.widget_at_position(event.pos())
            if widget is not None:
                return widget.event(event)
        return super(SceneView, self).event(event)

    def karaboBroadcastEvent(self, event):
        if event.sender is KaraboEventSender.AlarmDeviceUpdate:
            device_id = event.data.get('deviceId')
            alarm_type = event.data.get('alarm_type')
            self._update_alarm_symbols(device_id, alarm_type)
        elif event.sender is KaraboEventSender.AccessLevelChanged:
            self._update_widget_states()
        return False

    def contextMenuEvent(self, event):
        """ Show scene view specific context menu. """
        if self.design_mode:
            widget = self.widget_at_position(event.pos())
            if widget is not None:
                widget_handler = WidgetSceneHandler(widget=widget)
                # This methods blocks here until an action is selected
                widget_handler.handle(self, event)
                event.accept()
                return
        event.ignore()

    # ----------------------------
    # Public methods

    def destroy(self):
        """ Do some cleanup of the scene's objects before death.
        """
        for obj in self._scene_obj_cache.values():
            if is_widget(obj):
                obj.destroy()
        self.workflow_model.destroy()

        unregister_from_broadcasts(self)

    def set_tab_visible(self, visible):
        """ Sets whether this scene is visible

        This method manages the visibilities of the boxes in this scene."""
        if self.tab_visible == visible:
            return

        for obj in self._scene_obj_cache.values():
            if is_widget(obj):
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
        """ Returns the topmost object whose bounds contain `pos`.
        """
        for child in self.scene_model.children[::-1]:
            obj = self._scene_obj_cache.get(child)
            if obj is not None and obj.geometry().contains(pos):
                return obj

    def widget_at_position(self, pos):
        """ Returns the topmost widget whose bounds contain `pos`.
        """
        widget = self.inner.childAt(pos)
        if widget is not None:
            while not is_widget(widget):
                widget = widget.parent()
        return widget

    def items_in_rect(self, rect):
        """ Returns the topmost objects whose bounds are contained in `rect`.
        """
        items = []
        for child in self.scene_model.children[::-1]:
            obj = self._scene_obj_cache.get(child)
            if obj is not None and rect.contains(obj.geometry()):
                items.append(obj)
        return items

    def select_all(self):
        """ Add all children to the current selection
        """
        self.selection_model.clear_selection()
        for child in self.scene_model.children:
            obj = self._scene_obj_cache.get(child)
            if obj is not None:
                self.selection_model.select_object(obj)
        self.update()

    def select_model(self, model):
        """ Select widget object for the given ``model``.
        """
        self.selection_model.clear_selection()
        obj = self._scene_obj_cache.get(model)
        if obj is not None:
            self.selection_model.select_object(obj)
        self.update()

    def set_cursor(self, name):
        """ Sets the cursor for the scene view.
        """
        if name == 'none':
            self.unsetCursor()
        else:
            self.setCursor(QT_CURSORS[name])

    def set_tool(self, tool):
        """ Sets the current tool being used by the view.
        """
        assert tool is None or isinstance(tool, BaseSceneTool)
        self.current_tool = tool
        if tool is None:
            self.set_cursor('none')
            self.current_tool = SceneSelectionTool()

        self.update()

    def add_models(self, *models):
        """ Adds new child models to the scene model."""
        self.scene_model.children.extend(models)

    def remove_model(self, model):
        """ Removes the given ``model`` from the scene model."""
        self.scene_model.children.remove(model)

    def replace_model(self, old_model, new_model):
        """ Replace the given ``old_model`` with the given ``new_model``. """
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
        """ The given ``model`` is moved to the end of the list."""
        # Remove model
        self.remove_model(model)
        # Add model to end
        self.add_models(model)

        scene_obj = self._scene_obj_cache.get(model)
        if scene_obj is not None:
            # In case of layouts or widgets
            bring_object_to_front(scene_obj)

    def send_to_back(self, model):
        """ The given ``model`` is moved to the beginning of the list."""
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

    # ----------------------------
    # Private methods (yes, I know... It's just a convention)

    def _draw_selection(self, painter):
        """ Draw a dashed rect around the selected objects. """
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
        """ The scene model is set and all notification handlers are defined.
        """
        self.scene_model = scene_model
        self.scene_model.on_trait_change(self._model_modified,
                                         'children_items')
        self._add_workflow_items(self.scene_model.children)

    def _model_modified(self, event):
        """ The scene model got modified."""
        for model in event.removed:
            obj = self._scene_obj_cache.get(model)
            if obj is not None:
                remove_object_from_layout(obj, self.layout,
                                          self._scene_obj_cache,
                                          self.tab_visible)
        self._remove_workflow_items(event.removed)

        for model in event.added:
            create_object_from_model(self.layout, model, self.inner,
                                     self._scene_obj_cache, self.tab_visible)
        self._add_workflow_items(event.added)

    def _add_workflow_items(self, models):
        """ Add new WorkflowItemModel instances to the workflow model. """
        items = [m for m in models if isinstance(m, WorkflowItemModel)]
        self.workflow_model.add_items(items)

    def _remove_workflow_items(self, models):
        """ Remove WorkflowItemModel instances from the workflow model. """
        items = [m for m in models if isinstance(m, WorkflowItemModel)]
        self.workflow_model.remove_items(items)

    def _update_alarm_symbols(self, device_id, alarm_type):
        """ Update alarm indicators of widgets in need
        """
        for obj in self._scene_obj_cache.values():
            if is_widget(obj):
                obj.update_alarm_symbol(device_id, alarm_type)

    def _update_widget_states(self):
        """The global access level has changed. Notify all widgets in the
        scene.
        """
        for obj in self._scene_obj_cache.values():
            if is_widget(obj):
                obj.update_global_access_level()
