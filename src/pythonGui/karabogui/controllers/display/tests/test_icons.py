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
import operator

from qtpy.QtGui import QPixmap

from karabo.common.scenemodel.api import (
    DigitIconsModel, IconData, SelectionIconsModel, TextIconsModel)
from karabo.native import Configurable, Int32, String
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..icons import DigitIcons, SelectionIcons, TextIcons

NUM_OPTIONS = [1, 2, 3, 4]
TEXT_OPTIONS = ["foo", "bar", "baz", "qux"]

ICON_ITEM_PATH = "karabogui.controllers.icons_dialogs.IconItem"


class NumberObject(Configurable):
    prop = Int32(options=NUM_OPTIONS)


class StringObject(Configurable):
    prop = String(options=TEXT_OPTIONS)


def _digits_model():
    values = [IconData(value=2, equal=True), IconData(value=4, equal=False)]
    return DigitIconsModel(values=values)


def _selection_model():
    values = [IconData(value=i) for i in TEXT_OPTIONS[:2]]
    return SelectionIconsModel(values=values)


def _text_model():
    values = [IconData(value=i) for i in TEXT_OPTIONS[:2]]
    return TextIconsModel(values=values)


def _assert_current_item(controller, proxy, mocker, value,
                         valid=True, changed=True, digit_icon=False):
    # Save a reference on the previous item
    current_item = controller.current_item

    # Set the proxy with mocked icon item to avoid pixmap problems
    method_path = ICON_ITEM_PATH + "._load_pixmap"
    mocker.patch(method_path, return_value=QPixmap())
    set_proxy_value(proxy, "prop", value)

    if not valid:
        assert controller.current_item is None
        return

    if changed:
        assert current_item is not controller.current_item
    else:
        assert current_item is controller.current_item

    current_item = controller.current_item
    assert current_item is not None

    if digit_icon:
        compare = operator.le if current_item.equal else operator.lt
        assert compare(value, float(current_item.value))
    else:
        assert value == current_item.value


def test_digit_icons(gui_app, mocker):
    # set up
    schema = NumberObject.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    controller = DigitIcons(proxy=proxy, model=_digits_model())
    controller.create(None)
    assert controller.widget is not None
    assert controller.current_item is None

    # set values
    _assert_current_item(controller, proxy, mocker, 2, changed=True,
                         digit_icon=True)
    _assert_current_item(controller, proxy, mocker, 1, changed=False,
                         digit_icon=True)
    _assert_current_item(controller, proxy, mocker, 3, changed=True,
                         digit_icon=True)
    _assert_current_item(controller, proxy, mocker, 4, valid=False,
                         digit_icon=True)

    # teardown
    controller.destroy()
    assert controller.widget is None


def test_selection_icons(gui_app, mocker):
    # setup
    schema = StringObject.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    controller = SelectionIcons(proxy=proxy, model=_selection_model())
    controller.create(None)
    assert controller.widget is not None
    assert controller.current_item is None

    # set values
    _assert_current_item(controller, proxy, mocker, "foo", changed=True)
    _assert_current_item(controller, proxy, mocker, "foo", changed=False)
    _assert_current_item(controller, proxy, mocker, "bar", changed=True)

    # widget will automatically create the items for each option
    _assert_current_item(controller, proxy, mocker, "qux", changed=True)

    # teardown
    controller.destroy()
    assert controller.widget is None


def test_text_icons(gui_app, mocker):
    # setup
    schema = StringObject.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    controller = TextIcons(proxy=proxy, model=_text_model())
    controller.create(None)
    assert controller.widget is not None

    # set value
    _assert_current_item(controller, proxy, mocker, "foo", changed=True)
    _assert_current_item(controller, proxy, mocker, "foo", changed=False)
    _assert_current_item(controller, proxy, mocker, "bar", changed=True)

    # widget will NOT automatically create the items for each option
    _assert_current_item(controller, proxy, mocker, "qux", valid=False)

    # teardown
    controller.destroy()
    assert controller.widget is None
