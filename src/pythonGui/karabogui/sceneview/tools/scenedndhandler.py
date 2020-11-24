#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 30, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from abc import abstractmethod
import json

from PyQt5.QtCore import QPoint
from PyQt5.QtWidgets import QBoxLayout
from traits.api import ABCHasStrictTraits

from karabogui.binding.api import ImageBinding, SlotBinding
from karabo.common.scenemodel.api import (
    BoxLayoutModel, LabelModel, SceneLinkModel)
from karabo.common.scenemodel.const import SceneTargetWindow
from karabo.native import AccessMode
from karabogui.controllers.api import (
    get_class_const_trait, get_compatible_controllers, get_scene_model_class)
from karabogui.enums import ProjectItemTypes
from karabogui.sceneview.utils import round_down_to_grid
from karabogui.sceneview.widget.utils import get_proxy

_STACKED_WIDGET_OFFSET = 30
_NO_LABEL_BINDINGS = (ImageBinding, SlotBinding)


class SceneDnDHandler(ABCHasStrictTraits):
    @abstractmethod
    def can_handle(self, event):
        """Check whether the drag event can be handled."""

    @abstractmethod
    def handle(self, scene_view, event):
        """Handle the drop event."""


class ConfigurationDropHandler(SceneDnDHandler):
    """Scene D&D handler for drops originating from the configuration view.
    """

    def can_handle(self, event):
        source_type = event.mimeData().data('source_type')
        if source_type == 'ParameterTreeWidget':
            return True
        return False

    def handle(self, scene_view, event):
        mime_data = event.mimeData()
        items = []
        if mime_data.data('tree_items'):
            items_data = mime_data.data('tree_items').data()
            items = json.loads(items_data.decode())

        # Create the proxies first
        proxies = [get_proxy(*item['key'].split('.', 1)) for item in items]

        # Handle the case when dropped on an existing scene widget
        pos = event.pos()
        widget = scene_view.widget_at_position(pos)
        if widget is not None and widget.add_proxies(proxies):
            return

        # Handle the case when dropped as new scene widgets
        models = []
        for item, proxy in zip(items, proxies):
            if proxy.binding is None:
                continue

            model = self._create_model_from_parameter_item(item, proxy, pos)
            models.append(model)
            pos += QPoint(0, _STACKED_WIDGET_OFFSET)
        scene_view.add_models(*models, initialize=True)

    def _create_model_from_parameter_item(self, item, proxy, pos):
        """Create the scene models for a single item
        """

        def _create_model(klass, key):
            """Create a single widget from a class `klass` with a `key`"""
            model_klass = get_scene_model_class(klass)
            model = model_klass(keys=[key])
            model.x = round_down_to_grid(pos.x())
            model.y = round_down_to_grid(pos.y())
            if hasattr(model, 'klass'):
                model.klass = get_class_const_trait(klass, '_klassname')
            return model

        if isinstance(proxy.binding, _NO_LABEL_BINDINGS):
            # Note: Some bindings do not require a label widget and will
            # only appear with one widget
            klasses = get_compatible_controllers(proxy.binding)
            if klasses:
                return _create_model(klasses[0], proxy.key)

        # Horizonal layout
        layout_model = BoxLayoutModel(direction=QBoxLayout.LeftToRight,
                                      x=round_down_to_grid(pos.x()),
                                      y=round_down_to_grid(pos.y()))
        # Add label to layout model
        label_model = LabelModel(text=item['label'], foreground='#000000')
        layout_model.children.append(label_model)

        def insert_model_layout(klass, key, layout_model):
            model_klass = get_scene_model_class(klass)
            model = model_klass(keys=[key])
            if hasattr(model, 'klass'):
                model.klass = get_class_const_trait(klass, '_klassname')
            layout_model.children.append(model)

        # Add the display and editable widgets, as needed
        klasses = get_compatible_controllers(proxy.binding)
        if klasses:
            insert_model_layout(klasses[0], proxy.key, layout_model)

        # Only editable if RECONFIGURABLE
        if proxy.binding.access_mode is AccessMode.RECONFIGURABLE:
            klasses = get_compatible_controllers(proxy.binding, can_edit=True)
            if klasses:
                insert_model_layout(klasses[0], proxy.key, layout_model)

        return layout_model


class ProjectDropHandler(SceneDnDHandler):
    """Scene D&D handler for drops originating from the project view"""

    def can_handle(self, event):
        # We can handle a drop if it contains at least one item
        items = self._extract_items(event.mimeData())
        return (len(items) > 0 and
                items[0].get('type') == ProjectItemTypes.SCENE)

    def handle(self, scene_view, event):
        dropped_items = self._extract_items(event.mimeData())
        if len(dropped_items) == 0:
            return

        item = dropped_items[0]
        if item.get('type') == ProjectItemTypes.SCENE:
            uuid = item.get('uuid')
            simple_name = item.get('simple_name')
            target = '{}:{}'.format(simple_name, uuid)
            position = event.pos()
            model = SceneLinkModel(target=target,
                                   target_window=SceneTargetWindow.Dialog,
                                   x=position.x(), y=position.y())
            scene_view.add_models(model, initialize=True)

    def _extract_items(self, mime_data):
        known_types = (ProjectItemTypes.DEVICE, ProjectItemTypes.SCENE)
        items_data = mime_data.data('treeItems').data()
        if items_data:
            items = json.loads(items_data.decode())
            return [it for it in items if it['type'] in known_types]
        return []
