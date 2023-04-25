# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
