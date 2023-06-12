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
from karabo.common.scenemodel.api import DaemonManagerModel
from karabo.native import Configurable, Hash, String, VectorHash
from karabogui.binding.config import apply_configuration
from karabogui.testing import get_class_property_proxy

from ..daemon import DisplayDaemonService


class Row(Configurable):
    name = String(defaultValue="")
    status = String(defaultValue="")
    since = String(defaultValue="")
    duration = String(defaultValue="0.0")
    host = String(defaultValue="")


class Object(Configurable):
    prop = VectorHash(displayType="DaemonManager",
                      rows=Row)


def test_daemon(gui_app):
    # setup
    proxy = get_class_property_proxy(Object.getClassSchema(), "prop")
    model = DaemonManagerModel()
    controller = DisplayDaemonService(proxy=proxy, model=model)
    controller.create(None)

    # set values
    daemon_hash = Hash("prop",
                       [Hash("name", "KaraboDB", "status", "up, running",
                             "since", "Tue, 05 Nov 2019 22:51:08",
                             "duration", "22.0", "host", "exflhost")])
    apply_configuration(daemon_hash, proxy.root_proxy.binding)
    model = controller.table_model
    data = model.index(0, 0).data()
    assert data == "KaraboDB"
    assert data != "NoKaraboDB"
    data = model.index(0, 1).data()
    assert data == "up, running"
    data = model.index(0, 2).data()
    assert data == "Tue, 05 Nov 2019 22:51:08"
    data = model.index(0, 3).data()
    assert data == "22.0"
    data = model.index(0, 4).data()
    assert data == "exflhost"

    # teardown
    controller.destroy()
    assert controller.widget is None
