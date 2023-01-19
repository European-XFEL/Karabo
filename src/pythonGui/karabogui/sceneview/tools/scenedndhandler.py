#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 30, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import json
from abc import abstractmethod

from qtpy.QtCore import QPoint
from qtpy.QtWidgets import QBoxLayout
from traits.api import ABCHasStrictTraits, Undefined

from karabo.common.enums import Capabilities
from karabo.common.scenemodel.api import (
    BoxLayoutModel, DeviceSceneLinkModel, LabelModel, SceneLinkModel)
from karabo.common.scenemodel.const import SceneTargetWindow
from karabogui import messagebox
from karabogui.binding.api import ImageBinding, SlotBinding
from karabogui.controllers.api import (
    get_class_const_trait, get_controller_klass, get_scene_model_class)
from karabogui.fonts import get_font_metrics
from karabogui.itemtypes import NavigationItemTypes, ProjectItemTypes
from karabogui.sceneview.utils import round_down_to_grid
from karabogui.sceneview.widget.utils import get_proxy
from karabogui.singletons.api import get_topology

_STACKED_WIDGET_OFFSET = 30
_NO_LABEL_BINDINGS = (ImageBinding, SlotBinding)
_LINK_MARGIN = 10
_LINK_SIZE_HIT = 30

_DEVICE_TYPES = (NavigationItemTypes.DEVICE, ProjectItemTypes.DEVICE)


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

        is_online = False
        if mime_data.data('online_device'):
            is_online = mime_data.data('online_device').data()

        # Create the proxies first
        proxies = [get_proxy(*item['key'].split('.', 1)) for item in items]
        # Handle the case when dropped on an existing scene widget
        pos = event.pos()
        widget = scene_view.widget_at_position(pos)
        if widget is not None and is_online and widget.add_proxies(proxies):
            return

        # Handle the case when dropped as new scene widgets
        models = []
        for item, proxy in zip(items, proxies):
            model = self._create_model_from_name(item, proxy, pos)
            models.append(model)
            pos += QPoint(0, _STACKED_WIDGET_OFFSET)
        scene_view.add_models(*models, initialize=True)

    def insert_model_layout(self, klass, key, layout_model):
        model_klass = get_scene_model_class(klass)
        model = model_klass(keys=[key])
        if hasattr(model, 'klass'):
            model.klass = get_class_const_trait(klass, '_klassname')
        layout_model.children.append(model)
        return model

    def _create_model_from_name(self, item, proxy, pos):
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

        binding = item["binding"]
        if binding in ("SlotBinding", "ImageBinding"):
            # Note: Some bindings do not require a label widget and will
            # only appear with one display widget
            if klass := get_controller_klass(item['display_widget_class']):
                return _create_model(klass, proxy.key)
            return

        # Horizonal layout
        layout_model = BoxLayoutModel(direction=QBoxLayout.LeftToRight,
                                      x=round_down_to_grid(pos.x()),
                                      y=round_down_to_grid(pos.y()))
        # Add label to layout model
        label_model = LabelModel(text=item['label'], foreground='#000000')
        layout_model.children.append(label_model)

        # Add the display and editable widgets, as needed
        klass = get_controller_klass(item['display_widget_class'])
        self.insert_model_layout(klass, proxy.key, layout_model)

        # Only editable if RECONFIGURABLE
        if name := item.get('edit_widget_class'):
            if klass := get_controller_klass(name):
                self.insert_model_layout(klass, proxy.key, layout_model)

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

            fm = get_font_metrics()
            width = max(fm.width(simple_name) + _LINK_MARGIN, _LINK_SIZE_HIT)
            height = max(fm.height() + _LINK_MARGIN, _LINK_SIZE_HIT)

            model = SceneLinkModel(target=target,
                                   text=simple_name,
                                   target_window=SceneTargetWindow.Dialog,
                                   x=position.x(), y=position.y(),
                                   width=width, height=height)
            scene_view.add_models(model, initialize=True)

    def _extract_items(self, mime_data):
        known_types = (ProjectItemTypes.DEVICE, ProjectItemTypes.SCENE)
        items_data = mime_data.data('treeItems').data()
        if items_data:
            items = json.loads(items_data.decode())
            return [it for it in items if it['type'] in known_types]
        return []


class NavigationDropHandler(SceneDnDHandler):
    """Scene D&D handler for drops originating from the project view"""

    def can_handle(self, event):
        # We can handle a drop if it contains at least one item
        items = self._extract_items(event.mimeData())
        if not len(items):
            return False

        from_device = items[0].get('type') in _DEVICE_TYPES
        capa = Capabilities.PROVIDES_SCENES
        has_scene = items[0].get('capabilities') & capa == capa
        return from_device and has_scene

    def handle(self, scene_view, event):
        dropped_items = self._extract_items(event.mimeData())
        if len(dropped_items) == 0:
            return

        item = dropped_items[0]
        if item.get('type') in _DEVICE_TYPES:
            device_id = item['deviceId']

            def attach_device_link(scene_name):
                """Attach a device scene link"""
                nonlocal event, device_id, scene_view

                fm = get_font_metrics()
                width = max(fm.width(device_id) + _LINK_MARGIN, _LINK_SIZE_HIT)
                height = max(fm.height() + _LINK_MARGIN, _LINK_SIZE_HIT)

                position = event.pos()
                model = DeviceSceneLinkModel(
                    text=device_id,
                    keys=[f"{device_id}.availableScenes"],
                    target=scene_name,
                    target_window=SceneTargetWindow.Dialog,
                    x=position.x(), y=position.y(),
                    width=width, height=height)
                scene_view.add_models(model, initialize=True)

            def _config_handler():
                """Act on the arrival of the configuration"""
                proxy.on_trait_change(_config_handler, 'config_update',
                                      remove=True)
                scenes = proxy.binding.value.availableScenes.value
                if scenes is Undefined or not len(scenes):
                    messagebox.show_warning(
                        "The device <b>{}</b> does not specify a scene "
                        "name!".format(device_id))
                else:
                    scene_name = scenes[0]
                    attach_device_link(scene_name)

            def _schema_handler():
                """Act on the arrival of the schema"""
                proxy.on_trait_change(_schema_handler, 'schema_update',
                                      remove=True)
                scenes = proxy.binding.value.availableScenes.value
                if scenes is Undefined:
                    proxy.on_trait_change(_config_handler, 'config_update')
                elif not len(scenes):
                    messagebox.show_warning(
                        "The device <b>{}</b> does not specify a scene "
                        "name!".format(device_id))
                else:
                    scene_name = scenes[0]
                    attach_device_link(scene_name)

            proxy = get_topology().get_device(device_id)
            if not len(proxy.binding.value):
                # We completely miss our schema and wait for it.
                proxy.on_trait_change(_schema_handler, 'schema_update')
            elif proxy.binding.value.availableScenes.value is Undefined:
                # The configuration did not yet arrive and we cannot get
                # a scene name from the availableScenes.
                proxy.on_trait_change(_config_handler, 'config_update')
            else:
                scenes = proxy.binding.value.availableScenes.value
                if not len(scenes):
                    messagebox.show_warning(
                        "The device <b>{}</b> does not specify a scene "
                        "name!".format(device_id))
                else:
                    scene_name = scenes[0]
                    attach_device_link(scene_name)

    def _extract_items(self, mime_data):
        items_data = mime_data.data('treeItems').data()
        if items_data:
            items = json.loads(items_data.decode())
            return [it for it in items if it['type'] in _DEVICE_TYPES]
        return []
