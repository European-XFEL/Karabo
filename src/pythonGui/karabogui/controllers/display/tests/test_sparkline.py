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
import pytest

from karabo.common.scenemodel.api import SparklineModel
from karabo.native import Configurable, Float
from karabogui.testing import (
    get_class_property_proxy, get_property_proxy, set_proxy_value, singletons)
from karabogui.util import process_qt_events

from ..sparkline import DisplaySparkline
from .data import build_historic_data_float

TIMEBASES = {"60s": 60, "10m": 600, "10h": 36000}


class Object(Configurable):
    prop = Float(defaultValue=0.0)


@pytest.fixture
def sparkline_setup(gui_app):
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    model = SparklineModel()
    controller = DisplaySparkline(proxy=proxy, model=model)
    controller.create(None)
    assert controller.widget is not None

    yield controller, proxy

    controller.destroy()
    assert controller.widget is None


def test_basics(sparkline_setup, mocker):
    controller, proxy = sparkline_setup

    # Make sure there is a value with timestamp on it
    set_proxy_value(proxy, "prop", 12.0)

    mocker.patch.object(controller.line_edit, "setVisible")
    sym = "karabogui.controllers.display.sparkline.QInputDialog"
    QInputDialog = mocker.patch(sym)
    for ac in controller.widget.actions():
        ac_name = ac.text()
        if ac_name == "Set value format":
            QInputDialog.getText.return_value = ".3e", True
            ac.trigger()
            assert controller.model.show_format == ".3e"
        elif ac_name == "Show value":
            assert controller.model.show_value == ac.isChecked()
            ac.setChecked(not ac.isChecked())
            assert controller.model.show_value == ac.isChecked()
        elif ac_name == "Use alarm range":
            assert controller.model.alarm_range == ac.isChecked()
            ac.setChecked(not ac.isChecked())
            assert controller.model.alarm_range == ac.isChecked()
        else:
            tb_val = TIMEBASES.get(ac_name[:3], None)
            if tb_val is None:
                continue
            ac.trigger()
            assert tb_val == controller.model.time_base


def test_device(gui_app, mocker):
    network = mocker.Mock()
    with singletons(network=network):
        schema = Object.getClassSchema()
        proxy = get_property_proxy(schema, "prop")

        model = SparklineModel()
        controller = DisplaySparkline(proxy=proxy, model=model)
        controller.create(None)

        proxy.start_monitoring()
        # can't guess the time argument because it's now
        network.onGetPropertyHistory.assert_called()


def test_historic_data(sparkline_setup, mocker):
    controller, proxy = sparkline_setup

    set_proxy_value(proxy, "prop", 42.0)

    historic_data = build_historic_data_float()
    proxy.binding.historic_data = historic_data
    # mock the QPainter, so the code path can be walked deeper down
    # without the need of creating a backend draw device
    mocker.patch("karabogui.controllers.display.sparkline.QPainter")
    controller.render_area.paintEvent(None)
    process_qt_events()
