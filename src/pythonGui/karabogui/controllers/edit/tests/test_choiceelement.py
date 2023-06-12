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
from qtpy.QtCore import Qt

from karabo.native import Bool, ChoiceOfNodes, Configurable
from karabogui.binding.api import apply_default_configuration
from karabogui.testing import get_class_property_proxy

from ..choiceelement import EditableChoiceElement


class ChoicesBase(Configurable):
    pass


class ChoiceOne(ChoicesBase):
    prop = Bool()


class ChoiceTwo(ChoicesBase):
    prop = Bool()


class Object(Configurable):
    prop = ChoiceOfNodes(ChoicesBase, defaultValue="ChoiceOne")


@pytest.fixture
def choiceelement_setup(gui_app):
    # setup
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    controller = EditableChoiceElement(proxy=proxy)
    controller.create(None)
    assert controller.widget is not None
    yield controller, proxy
    # teardown
    controller.destroy()
    assert controller.widget is None


def test_choiceelement_basics(choiceelement_setup):
    # focus policy
    controller, proxy = choiceelement_setup
    assert controller.widget.focusPolicy() == Qt.StrongFocus

    # test set up
    assert controller.widget.count() == 2

    # set value
    apply_default_configuration(proxy.root_proxy.binding)
    assert controller.widget.currentText() == "ChoiceOne"
    proxy.value = "ChoiceTwo"
    assert controller.widget.currentText() == "ChoiceTwo"

    # edit value
    widget = controller.widget
    index = widget.findText("ChoiceTwo")
    widget.setCurrentIndex(index)
    assert proxy.binding.choice == "ChoiceTwo"
