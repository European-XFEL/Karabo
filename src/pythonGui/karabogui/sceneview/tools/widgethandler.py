#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 11, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtGui import QMenu
from traits.api import Any, ABCHasStrictTraits

from karabo.common.scenemodel.api import BaseWidgetObjectData
from karabogui.controllers.api import (
    get_class_const_trait, get_compatible_controllers, get_scene_model_class)
from karabogui.sceneview.widget.api import ControllerContainer


class WidgetSceneHandler(ABCHasStrictTraits):
    widget = Any  # Current widget

    def handle(self, scene_view, event):
        """A callback which is fired whenever the user requests a context menu
        in the SceneView.
        """
        if not isinstance(self.widget, ControllerContainer):
            return

        controller = self.widget.widget_controller
        if len(controller.proxies) > 1:
            # We currently don't allow user to implicitly mutate a controller
            # which contains more than one proxy.
            # XXX: Could provide sub menu for each property
            main_menu = QMenu(self.widget)
            info = main_menu.addAction('No mutation for multiple properties')
            info.setEnabled(False)
            main_menu.exec_(event.globalPos())
            return

        binding = controller.proxy.binding
        if binding is None:
            # The binding must be valid for this to work...
            return

        model = controller.model
        if model.parent_component == 'EditableApplyLaterComponent':
            can_edit = True
        else:
            can_edit = False
        klasses = get_compatible_controllers(binding, can_edit=can_edit)

        main_menu = QMenu(self.widget)
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
