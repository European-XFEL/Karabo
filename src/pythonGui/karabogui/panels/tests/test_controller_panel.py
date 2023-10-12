# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from karabo.common.scenemodel.api import TrendGraphModel
from karabo.native import Configurable, Float, Hash
from karabogui.binding.api import ProxyStatus, build_binding
from karabogui.generic_scenes import get_property_proxy_model
from karabogui.panels.widgetcontrollerpanel import WidgetControllerPanel
from karabogui.singletons.mediator import Mediator
from karabogui.testing import (
    get_property_proxy, set_proxy_hash, singletons, system_hash)
from karabogui.topology.api import SystemTopology


class Object(Configurable):
    prop = Float(
        defaultValue=0.2)


def test_panel_controller_basics(gui_app):
    """Test the basics of the widget controller"""
    mediator = Mediator()
    topology = SystemTopology()
    h = system_hash()
    topology.initialize(h)
    with singletons(mediator=mediator, topology=topology):
        proxy = get_property_proxy(Object.getClassSchema(), "prop",
                                   device_id="divvy")
        model = get_property_proxy_model(proxy)
        assert isinstance(model, TrendGraphModel)

        # Create a panel and start monitoring
        panel = WidgetControllerPanel("TestTitle", model)
        assert panel.controller._showing
        assert panel.proxy is not proxy
        proxy = panel.proxy
        # Root device is offline, we are showing the pixmap
        assert not panel.status_symbol.isHidden()
        assert proxy.visible
        # Root devices comes online
        proxy.root_proxy.status = ProxyStatus.ONLINE
        # Moves to requested
        assert proxy.root_proxy.status is ProxyStatus.ONLINEREQUESTED
        assert proxy.root_proxy._monitor_count == 1

        # Simulate schema and config walk through
        build_binding(Object.getClassSchema(), proxy.root_proxy.binding)
        assert proxy.root_proxy.status is ProxyStatus.SCHEMA

        set_proxy_hash(proxy, Hash("prop", 1.2))
        proxy.root_proxy.config_update = True

        assert proxy.root_proxy.status is ProxyStatus.MONITORING
        assert panel.status_symbol.isHidden()
        panel.close()

        # And we stop monitoring if we close the panel
        assert proxy.root_proxy._monitor_count == 0
        assert proxy.root_proxy.status is ProxyStatus.ONLINE
        assert not panel.controller._showing

        panel.destroy()


def test_repr(gui_app):
    """Test the basics of the widget controller"""
    mediator = Mediator()
    topology = SystemTopology()
    topology.initialize(system_hash())
    with singletons(mediator=mediator, topology=topology):
        proxy = get_property_proxy(Object.getClassSchema(), "prop",
                                   device_id="divvy")
        model = get_property_proxy_model(proxy)

        panel = WidgetControllerPanel("TestTitle", model)
        assert repr(panel) == "<WidgetControllerPanel property=divvy.prop>"
