#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 11, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QMenu

from traits.api import Any, ABCHasStrictTraits

from .const import WIDGET_FACTORIES
from karabo_gui.scenemodel.api import BaseWidgetObjectData
from karabo_gui.sceneview.widget.api import BaseWidgetContainer
from karabo_gui.widget import DisplayWidget, EditableWidget


class WidgetSceneHandler(ABCHasStrictTraits):
    widget = Any  # Current widget

    def can_handle(self):
        """ Check whether the handler can handle ``self.widget``. """
        if isinstance(self.widget, BaseWidgetContainer):
            return True
        return False

    def handle(self, scene_view, event):
        """ A callback which is fired whenever the user requests a context menu
        in the SceneView.
        """
        if not self.can_handle():
            return

        box = self.widget.boxes[0]
        model = self.widget.model
        if model.parent_component == 'EditableApplyLaterComponent':
            widget_classes = EditableWidget.getClasses(box)
        else:
            widget_classes = DisplayWidget.getClasses(box)

        main_menu = QMenu(self.widget)
        change_menu = main_menu.addMenu("Change Widget")
        for klass in widget_classes:
            ac = change_menu.addAction(klass.alias)
            ac.triggered.connect(partial(self._change_widget,
                                         scene_view, klass))
        main_menu.exec_(event.globalPos())

    @pyqtSlot(str)
    def _change_widget(self, scene_view, widget_class):
        """ """
        model_klass = WIDGET_FACTORIES[widget_class.__name__]

        old_model = self.widget.model
        # Get old model traits
        traits = {}
        for key in BaseWidgetObjectData.class_editable_traits():
            if key in ('height', 'width'):
                # Ignore width and height for correct layout recalculation
                traits[key] = 0
                continue
            traits[key] = getattr(old_model, key)

        new_model = model_klass(**traits)
        scene_view.replace_model(old_model, new_model)
