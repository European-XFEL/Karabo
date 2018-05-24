#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 11, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtCore import QPoint, QRect
from PyQt4.QtGui import QAction, QDialog, QMenu
from traits.api import Any, ABCHasStrictTraits

from karabo.common.scenemodel.api import BaseWidgetObjectData
from karabogui.controllers.api import (
    get_class_const_trait, get_compatible_controllers, get_scene_model_class)
from karabogui.dialogs.dialogs import SceneItemDialog
from karabogui.sceneview.widget.api import ControllerContainer
# Padding constrains for resize action
QPAD_CART = 2
QPAD_GEO = 4


class WidgetSceneHandler(ABCHasStrictTraits):
    """Widget Scene Handler to provide basic operation on our SceneView

    The widget handler is used to change the controller class as well as
    to move and resize the selected layouts on the scene.
    """
    widget = Any  # Current widget

    def handle(self, scene_view, event):
        """A callback which is fired whenever the user requests a context menu
        in the SceneView.
        """
        # The place action is always available!
        main_menu = QMenu(self.widget)
        move_action = QAction("Move Layout", self.widget)
        move_action.triggered.connect(partial(self._move_dialog,
                                              scene_view))
        main_menu.addAction(move_action)

        # NOTE: Only if we don't have more items selected than a single one, we
        # are allowed to resize
        if not len(scene_view.selection_model) > 1:
            resize_action = QAction("Resize Layout", self.widget)
            resize_action.triggered.connect(partial(self._resize_dialog,
                                                    scene_view))
            main_menu.addAction(resize_action)

        if not isinstance(self.widget, ControllerContainer):
            # Not further actions are required!
            main_menu.exec_(event.globalPos())
            return

        controller = self.widget.widget_controller
        if len(controller.proxies) > 1:
            # We currently don't allow user to implicitly mutate a controller
            # which contains more than one proxy.
            info = main_menu.addAction('No mutation for multiple properties')
            info.setEnabled(False)
            main_menu.exec_(event.globalPos())
            return

        binding = controller.proxy.binding
        if binding is None:
            # We don't allow changing widgets without binding!
            info = main_menu.addAction('No mutation for offline properties')
            info.setEnabled(False)
            main_menu.exec_(event.globalPos())
            return

        model = controller.model
        if model.parent_component == 'EditableApplyLaterComponent':
            can_edit = True
        else:
            can_edit = False
        klasses = get_compatible_controllers(binding, can_edit=can_edit)

        change_menu = main_menu.addMenu('Change Widget')
        # Add actions which are bound to the actual Qt widget
        qwidget = controller.widget
        if qwidget.actions():
            change_menu.addActions(qwidget.actions())
            change_menu.addSeparator()

        klasses.sort(key=lambda w: get_class_const_trait(w, '_ui_name'))
        for klass in klasses:
            ui_name = get_class_const_trait(klass, '_ui_name')
            ac = change_menu.addAction(ui_name)
            ac.triggered.connect(partial(self._change_widget,
                                         scene_view, klass))
        main_menu.exec_(event.globalPos())

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
        """Resize the given layout selection in the sceneview
        """
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
