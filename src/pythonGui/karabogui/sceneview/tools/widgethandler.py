#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 11, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from abc import abstractmethod
from functools import partial

from PyQt4.QtCore import QPoint, QRect
from PyQt4.QtGui import QAction, QDialog, QMenu
from traits.api import Any, ABCHasStrictTraits

from karabo.common.scenemodel.api import BaseWidgetObjectData
from karabogui.controllers.api import (
    get_class_const_trait, get_compatible_controllers, get_scene_model_class)
from karabogui.dialogs.dialogs import SceneItemDialog
from karabogui.sceneview.widget.api import ControllerContainer
from karabogui.sceneview.tools.api import send_to_back, send_to_front, ungroup

# Padding constrains for resize action
QPAD_CART = 2
QPAD_GEO = 4


class SceneWidgetHandler(ABCHasStrictTraits):
    # Current widget
    widget = Any

    def handle(self, scene_view, event):
        """Handle the widget and event."""
        menu = QMenu(self.widget)
        move_action = QAction("Move Layout", self.widget)
        move_action.triggered.connect(partial(self._move_dialog, scene_view))
        menu.addAction(move_action)

        # NOTE: Only if we don't have more items selected than a single one, we
        # are allowed to resize the layout!
        if not len(scene_view.selection_model) > 1:
            resize_action = QAction("Resize Layout", self.widget)
            resize_action.triggered.connect(partial(self._resize_dialog,
                                                    scene_view))
            menu.addAction(resize_action)
        ungroup_action = QAction("Ungroup Layout", self.widget)
        ungroup_action.triggered.connect(partial(self._ungroup,
                                                 scene_view))
        menu.addAction(ungroup_action)

        to_front_action = QAction("Send To Front", self.widget)
        to_front_action.triggered.connect(partial(self._to_front,
                                                  scene_view))
        menu.addAction(to_front_action)
        to_back_action = QAction("Send To Back", self.widget)
        to_back_action.triggered.connect(partial(self._to_back,
                                                 scene_view))
        menu.addAction(to_back_action)

        self.handle_widget(scene_view, event, menu)

    # ------------------------------
    # Private interface

    def _move_dialog(self, scene_view):
        """Move the layout selection on the sceneview via a dialog interaction
        """
        selection_model = scene_view.selection_model
        rect = selection_model.get_selection_bounds()
        max_x = scene_view.scene_model.width - rect.width()
        max_y = scene_view.scene_model.height - rect.height()
        dialog = SceneItemDialog(x=rect.x(), y=rect.y(),
                                 title='Move Layout',
                                 max_x=max_x, max_y=max_y, parent=self.widget)
        if dialog.exec_() == QDialog.Accepted:
            x_coord = dialog.x - rect.x()
            y_coord = dialog.y - rect.y()
            offset = QPoint(x_coord, y_coord)
            for c in selection_model:
                c.translate(offset)

    def _resize_dialog(self, scene_view):
        """Resize the given layout selection in the sceneview"""
        selection_model = scene_view.selection_model
        rect = selection_model.get_selection_bounds()
        max_x = scene_view.scene_model.width - rect.x()
        max_y = scene_view.scene_model.height - rect.y()
        dialog = SceneItemDialog(x=rect.width(), y=rect.height(),
                                 title='Resize Layout',
                                 max_x=max_x, max_y=max_y, parent=self.widget)
        if dialog.exec_() == QDialog.Accepted:
            # The padding has to be considered differently for cartesian
            # and geometrical magnitudes!
            new_rect = QRect(rect.x() + QPAD_CART, rect.y() + QPAD_CART,
                             dialog.x - QPAD_GEO, dialog.y - QPAD_GEO)
            for c in selection_model:
                c.set_geometry(new_rect)

    def _to_back(self, scene_view):
        send_to_back(scene_view)

    def _to_front(self, scene_view):
        send_to_front(scene_view)

    def _ungroup(self, scene_view):
        ungroup(scene_view)

    # ------------------------------
    # Abstract public interface

    @abstractmethod
    def can_handle(self):
        """Check whether the event can be handled."""

    @abstractmethod
    def handle_widget(self, scene_view, event, menu):
        """Handle the event from the sceneview and attach actions to the menu!
        """


class SceneToolHandler(SceneWidgetHandler):
    """SceneTool handler to provide operations on workflow items and layouts
    """

    def can_handle(self):
        return not isinstance(self.widget, ControllerContainer)

    def handle_widget(self, scene_view, event, menu):
        # The non-controller widget might have some actions for us!
        if self.widget.actions():
            menu.addSeparator()
            menu.addActions(self.widget.actions())
        menu.exec_(event.globalPos())


class SceneControllerHandler(SceneWidgetHandler):
    """Scene Controller handler to provide basic operation on our SceneView

    The widget handler can be used to change the controller class
    """

    def can_handle(self):
        return isinstance(self.widget, ControllerContainer)

    def handle_widget(self, scene_view, event, menu):
        """A callback which is fired whenever the user requests a context menu
        in the SceneView.
        """
        # We don't allow to mutate a controller which contains many proxies
        controller = self.widget.widget_controller
        if len(controller.proxies) > 1:
            info = menu.addAction('No mutation for multiple properties')
            info.setEnabled(False)
            menu.exec_(event.globalPos())
            return

        # We don't allow changing widgets without binding!
        binding = controller.proxy.binding
        if binding is None:
            info = menu.addAction('No mutation for offline properties')
            info.setEnabled(False)
            menu.exec_(event.globalPos())
            return

        # We can finally mutate our widget!
        model = controller.model
        if model.parent_component == 'EditableApplyLaterComponent':
            can_edit = True
        else:
            can_edit = False
        klasses = get_compatible_controllers(binding, can_edit=can_edit)

        change_menu = menu.addMenu('Change Widget')
        # Add actions which are bound to the actual Qt widget
        qwidget = controller.widget
        if qwidget.actions():
            change_menu.addActions(qwidget.actions())
            change_menu.addSeparator()

        klasses.sort(key=lambda w: get_class_const_trait(w, '_ui_name'))
        for klass in klasses:
            ui_name = get_class_const_trait(klass, '_ui_name')
            action = change_menu.addAction(ui_name)
            action.triggered.connect(partial(self._change_widget,
                                             scene_view, klass))
            if isinstance(controller, klass):
                action.setCheckable(True)
                action.setChecked(True)

        menu.exec_(event.globalPos())

    # -------------------------------------------------
    # Private interface

    def _change_widget(self, scene_view, klass):
        """The model of ``self.widget`` is about to be changed.

        The given ``klass`` object is a `BaseBindingController` class which the
        widget is about to be replaced with.
        """
        model_klass = get_scene_model_class(klass)

        old_model = self.widget.model
        # Get old model traits
        traits = {}
        for key in BaseWidgetObjectData.class_editable_traits():
            traits[key] = getattr(old_model, key)
        # Set the `klass`, if settable
        if 'klass' in model_klass.class_editable_traits():
            traits['klass'] = get_class_const_trait(klass, '_klassname')
        # Ignore width and height for correct layout recalculation
        traits['height'] = traits['width'] = 0
        new_model = model_klass(**traits)
        scene_view.replace_model(old_model, new_model)
