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
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.api import DisplayTimeModel
from karabo.native import Configurable, Float, Hash, Timestamp
from karabogui.fonts import get_font_size_from_dpi
from karabogui.testing import get_class_property_proxy, set_proxy_hash

from ..timelabel import DisplayTimeLabel


class Object(Configurable):
    prop = Float()


@pytest.fixture
def time_label_setup(gui_app):
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    controller = DisplayTimeLabel(proxy=proxy, model=DisplayTimeModel())
    controller.create(None)
    yield controller, proxy


def test_time_label_basics(time_label_setup):
    controller, _ = time_label_setup
    assert controller.widget is not None

    controller.destroy()
    assert controller.widget is None

    model = controller.model
    assert model.time_format == "%H:%M:%S"
    assert model.font_size == 10
    assert model.font_weight == "normal"


def test_time_label_set_value(time_label_setup):
    controller, proxy = time_label_setup
    property_hash = Hash("prop", 2.0)
    time_stamp = Timestamp("2009-04-20T10:32:22")
    set_proxy_hash(proxy, property_hash, time_stamp)
    assert controller.widget.text() == "10:32:22"


def test_change_time_format(time_label_setup, mocker):
    controller, proxy = time_label_setup
    property_hash = Hash("prop", 4.0)
    time_stamp = Timestamp("2012-04-20T10:35:27")
    set_proxy_hash(proxy, property_hash, time_stamp)
    action = controller.widget.actions()[0]
    assert action.text() == "Change datetime format..."

    dsym = "karabogui.controllers.display.timelabel.QInputDialog"
    QInputDialog = mocker.patch(dsym)
    QInputDialog.getText.return_value = "%H:%M", True
    action.trigger()
    assert controller.widget.text() == "10:35"


def test_format_field(time_label_setup, mocker):
    """
    Test contex menu-item opens the dialog and the model values are
    set from the dialog.
    """
    controller, _ = time_label_setup
    action = controller.widget.actions()[1]
    assert action.text() == "Format field..."

    path = "karabogui.controllers.display.timelabel.FormatLabelDialog"
    dialog = mocker.patch(path)
    dialog().exec.return_value = QDialog.Accepted
    dialog().font_size = 11
    dialog().font_weight = "bold"
    action.trigger()
    assert dialog().exec.call_count == 1
    assert controller.model.font_size == 11
    assert controller.model.font_weight == "bold"


def test_apply_format(time_label_setup):
    """
    Test '_apply_format' applies the font size and weight to widget from
    the model.
    """
    controller, _ = time_label_setup
    widget = controller.widget
    controller._apply_format(widget)
    font = widget.font()
    size = font.pointSize()
    bold = font.bold()

    assert size == get_font_size_from_dpi(controller.model.font_size)
    assert bold == (controller.model.font_weight == "bold")
