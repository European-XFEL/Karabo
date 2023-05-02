# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
