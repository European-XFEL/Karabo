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

from karabo.common.api import State
from karabo.common.scenemodel.api import LineEditModel
from karabo.native import Char, Configurable, String
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..textedit import EditableLineEdit


class Object(Configurable):
    state = String(defaultValue=State.ON)
    prop = String(allowedStates={State.ON})


class ObjectChar(Configurable):
    prop = Char()


def test_editable_line_edit(gui_app):
    # set up
    proxy = get_class_property_proxy(Object.getClassSchema(), "prop")
    controller = EditableLineEdit(proxy=proxy, model=LineEditModel())
    controller.create(None)

    # test focus policy
    assert controller.widget.focusPolicy() == Qt.StrongFocus

    # test set value
    controller._last_cursor_pos = 0
    set_proxy_value(proxy, "prop", "Blah")
    assert controller.widget.text() == "Blah"
    assert controller.widget.cursorPosition() == 0

    # test edit value
    controller.widget.textChanged.emit("Wha??")
    assert proxy.edit_value == "Wha??"

    # access level
    assert controller.widget.isEnabled()
    set_proxy_value(proxy, "state", "ERROR")
    assert not controller.widget.isEnabled()

    # enabled
    controller.setEnabled(True)
    assert controller.widget.isEnabled()
    controller.setEnabled(False)
    assert not controller.widget.isEnabled()

    # teardown
    controller.destroy()
    assert controller.widget is None


def test_char_editable_line_edit(gui_app):
    # setup
    proxy = get_class_property_proxy(ObjectChar.getClassSchema(), "prop")
    controller = EditableLineEdit(proxy=proxy, model=LineEditModel())
    controller.create(None)

    # test set value
    controller._last_cursor_pos = 0
    set_proxy_value(proxy, "prop", "D")
    assert controller.widget.text() == "D"
    assert controller.widget.cursorPosition() == 0

    # test edit value
    set_proxy_value(proxy, "prop", "$")
    assert controller.widget.text() == "$"
    controller.widget.setText("12345")
    # stop at the first char
    assert proxy.edit_value == "1"

    # teardown
    controller.destroy()
    assert controller.widget is None
