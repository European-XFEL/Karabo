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
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.api import DisplayListModel
from karabo.native import Configurable, VectorInt32
from karabogui.binding.api import apply_default_configuration
from karabogui.controllers.display.list import DisplayList
from karabogui.fonts import get_font_size_from_dpi
from karabogui.testing import get_class_property_proxy, set_proxy_value


class Object(Configurable):
    prop = VectorInt32(defaultValue=[1])


def test_list_controller(gui_app, mocker):
    proxy = get_class_property_proxy(Object.getClassSchema(), "prop")
    controller = DisplayList(proxy=proxy,
                             model=DisplayListModel())
    controller.create(None)
    apply_default_configuration(proxy.root_proxy.binding)
    assert controller.widget.focusPolicy() == Qt.NoFocus

    set_proxy_value(proxy, "prop", [0, 2])
    assert controller.widget.text() == "0,2"

    model = controller.model
    assert model.font_size == 10
    assert model.font_weight == "normal"

    action = controller.widget.actions()[0]
    assert action.text() == "Format field..."

    path = "karabogui.controllers.display.list.FormatLabelDialog"
    dialog = mocker.patch(path)
    dialog().exec.return_value = QDialog.Accepted
    dialog().font_size = 12
    dialog().font_weight = "bold"
    action.trigger()
    assert dialog().exec.call_count == 1
    assert controller.model.font_size == 12
    assert controller.model.font_weight == "bold"

    widget = controller.widget
    controller._apply_format(widget)
    font = widget.font()
    size = font.pointSize()
    bold = font.bold()

    assert size == get_font_size_from_dpi(controller.model.font_size)
    assert bold == (controller.model.font_weight == "bold")

    controller.destroy()
    assert controller.widget is None
