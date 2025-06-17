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
import contextlib
import os
import sys
import traceback
import unittest
from platform import system
from xml.etree.ElementTree import parse

import pytest
from qtpy.QtCore import Qt
from qtpy.QtTest import QTest
from qtpy.QtWidgets import QApplication

import karabogui.singletons.api as singletons_mod
from karabo.common.api import Capabilities, State
from karabo.common.scenemodel.api import set_scene_reader
from karabo.native import (
    AccessLevel, AccessMode, Configurable, Double, Hash, Int32, String, Unit,
    VectorString)
from karabogui.background import create_background_timer
from karabogui.binding.api import (
    BindingRoot, DeviceClassProxy, DeviceProxy, PropertyProxy, ProxyStatus,
    apply_configuration, build_binding)
from karabogui.controllers.api import populate_controller_registry
from karabogui.util import process_qt_events


class GuiTestCase(unittest.TestCase):
    """ A convenient base class for gui test cases"""

    def setUp(self):
        os.environ["KARABO_TEST_GUI"] = "1"
        if system() == "Darwin" and "QT_MAC_WANTS_LAYER" not in os.environ:
            os.environ["QT_MAC_WANTS_LAYER"] = "1"
        app = QApplication.instance()
        if app is None:
            app = QApplication(sys.argv)
        self.app = app
        create_background_timer()
        # AFTER the QApplication is created!
        import karabogui.access as krb_access
        krb_access.GLOBAL_ACCESS_LEVEL = AccessLevel.OPERATOR
        from karabogui import icons
        icons.init()
        populate_controller_registry()

    def tearDown(self):
        pass

    def process_qt_events(self, ms=10):
        # Give the event loop 10ms to process its events
        process_qt_events(timeout=ms)

    def click(self, button_widget, button=Qt.LeftButton):
        QTest.mouseClick(button_widget, button)


def click_button(button_widget, button=Qt.LeftButton):
    QTest.mouseClick(button_widget, button)


def keySequence(widget, sequence):
    QTest.keySequence(widget, sequence)


@contextlib.contextmanager
def assert_trait_change(obj, name):
    """A context manager which watches for traits events
    """
    events = []

    def handler():
        events.append(None)

    obj.on_trait_change(handler, name)
    try:
        yield
    finally:
        obj.on_trait_change(handler, name, remove=True)
        if len(events) == 0:
            msg = "Expected change for trait '{}' was not observed!"
            raise AssertionError(msg.format(name))


@contextlib.contextmanager
def flushed_registry():
    """Avoid polluting the global binding controller registry with
    test controller classes
    """
    from karabogui.controllers import registry as registrymod

    controller_registry = registrymod._controller_registry.copy()
    registrymod._controller_registry.clear()
    model_registry = registrymod._controller_models.copy()
    registrymod._controller_models.clear()

    yield

    registrymod._controller_registry = controller_registry
    registrymod._controller_models = model_registry


def get_property_proxy(schema, name, device_id="TestDevice"):
    """Given a device schema and a property name, return a complete
    PropertyProxy object with a `root_proxy` of type `DeviceProxy`.
    """
    binding = BindingRoot() if schema is None else build_binding(schema)
    status = (ProxyStatus.ONLINEREQUESTED if schema is None
              else ProxyStatus.SCHEMA)
    root_proxy = DeviceProxy(binding=binding, device_id=device_id)
    root_proxy.status = status
    return PropertyProxy(root_proxy=root_proxy, path=name)


def get_class_property_proxy(schema, name):
    """Given a device schema and a property name, return a complete
    PropertyProxy object with a `root_proxy` of type `DeviceClassProxy`.
    """
    binding = build_binding(schema)
    root = DeviceClassProxy(binding=binding, server_id="Test",
                            status=ProxyStatus.OFFLINE)
    return PropertyProxy(root_proxy=root, path=name)


def set_proxy_value(proxy, name, value):
    """Use `apply_configuration` to set a single property value on a proxy
    """
    apply_configuration(Hash(name, value), proxy.root_proxy.binding)


def set_proxy_hash(proxy, hsh, timestamp=None):
    """Use `apply_configuration` to set a a hash on a proxy

    :param timestamp: Optionally a timestamp can be provided for the hash
    """
    if timestamp is not None:
        timestamp.toHashAttributes(hsh)

    apply_configuration(hsh, proxy.root_proxy.binding)


@contextlib.contextmanager
def singletons(**objects):
    """Provide a collection of singletons to be used for the duration of a
    `with`-block.
    """
    singletons_dict = singletons_mod.__singletons
    # Remember what got replaced
    replaced = {k: singletons_dict[k] for k in objects if k in singletons_dict}
    try:
        # Replace and yield
        for key, obj in objects.items():
            singletons_dict[key] = obj
        yield
    finally:
        # Put things back as they were
        for key, obj in replaced.items():
            singletons_dict[key] = obj
        for key in objects:
            if key not in replaced:
                del singletons_dict[key]


@contextlib.contextmanager
def assert_no_throw():
    """A simple context manager to assert a no-throw"""
    try:
        yield None
    except Exception as exception:
        trace = ''.join(traceback.format_exception(
            type(exception), exception, exception.__traceback__))
        raise pytest.fail(f"Did raise unexpected exception: {trace}")


@contextlib.contextmanager
def access_level(level):
    """Temporary move the access level to `level`"""
    import karabogui.access as krb_access
    old_level = krb_access.GLOBAL_ACCESS_LEVEL
    try:
        krb_access.GLOBAL_ACCESS_LEVEL = level
        yield
    finally:
        krb_access.GLOBAL_ACCESS_LEVEL = old_level


DEVICE_CAPA = Capabilities.PROVIDES_SCENES + Capabilities.PROVIDES_INTERFACES


def system_hash():
    """Generate a system hash which will be built into a system tree
    """
    h = Hash()

    h["server.swerver"] = None
    h["server.swerver", ...] = {
        "host": "BIG_IRON",
        "visibility": AccessLevel.OBSERVER,
        "deviceClasses": ["FooClass", "BarClass"],
        "type": "server",
        "visibilities": [AccessLevel.OBSERVER, AccessLevel.OBSERVER]
    }
    h["device.divvy"] = None
    h["device.divvy", ...] = {
        "host": "BIG_IRON",
        "archive": True,
        "visibility": AccessLevel.OBSERVER,
        "type": "device",
        "capabilities": DEVICE_CAPA,
        "serverId": "swerver",
        "classId": "FooClass",
        "status": "ok",
        "interfaces": 0,
    }
    h["macro.macdonald"] = None
    h["macro.macdonald", ...] = {
        "host": "BIG_IRON",
        "archive": False,
        "visibility": AccessLevel.OBSERVER,
        "serverId": "swerver",
        "classId": "BarClass",
        "capabilities": 0,
        "status": "ok",
        "interfaces": 0,
    }

    h["device.orphan"] = None
    h["device.orphan", ...] = {
        "host": "BIG_IRON",
        "visibility": AccessLevel.OBSERVER,
        "archive": True,
        "serverId": "__none__",
        "classId": "Parentless",
        "capabilities": 0,
        "status": "ok",
        "interfaces": 0,
    }

    h["client.charlie"] = None
    h["client.charlie", ...] = {
        "host": "BIG_IRON",
        "visibility": AccessLevel.OBSERVER,
        "archive": False,
        "serverId": "__none__",
        "classId": "NoClass",
        "status": "ok",
    }

    return h


def development_system_hash():
    h = Hash()

    h["server.devserver"] = None
    h["server.devserver", ...] = {
        "host": "BIG_IRON",
        "visibility": AccessLevel.OBSERVER,
        "deviceClasses": ["MetaMacro"],
        "type": "server",
        "lang": "macro",
        "serverFlags": 1,
        "visibilities": [AccessLevel.OBSERVER]
    }

    h["server.stableserver"] = None
    h["server.stableserver", ...] = {
        "host": "BIG_IRON",
        "visibility": AccessLevel.OBSERVER,
        "deviceClasses": ["MetaMacro"],
        "type": "server",
        "lang": "macro",
        "serverFlags": 0,
        "visibilities": [AccessLevel.OBSERVER]
    }

    return h


def device_hash():
    """Generate a device hash which will be built into a device tree
    """
    h = Hash()

    h["device.XFEL/FOO/1"] = None
    h["device.XFEL/FOO/1", ...] = {
        "host": "BIG_IRON",
        "archive": False,
        "visibility": AccessLevel.OBSERVER,
        "capabilities": DEVICE_CAPA,
        "serverId": "swerver",
        "classId": "FooClass",
        "status": "ok",
        "interfaces": 0,
    }

    h["device.XFEL/FOO/2"] = None
    h["device.XFEL/FOO/2", ...] = {
        "host": "BIG_IRON",
        "archive": True,
        "visibility": AccessLevel.OBSERVER,
        "capabilities": DEVICE_CAPA,
        "serverId": "swerver",
        "classId": "FooClass",
        "status": "ok",
        "interfaces": 0,
    }

    h["device.XFEL/BAR/1"] = None
    h["device.XFEL/BAR/1", ...] = {
        "host": "BIG_IRON",
        "archive": True,
        "visibility": AccessLevel.OBSERVER,
        "capabilities": DEVICE_CAPA,
        "serverId": "swerver",
        "classId": "BarClass",
        "status": "error",
        "interfaces": 0,
    }

    return h


def system_hash_server_and_plugins():
    """Generate a system hash which will be built into a system tree
    """
    h = Hash()

    h["server.myserver"] = None
    h["server.myserver", ...] = {
        "host": "exflpxc_something",
        "visibility": AccessLevel.OBSERVER,
        "deviceClasses": ["FooClass", "BarClass"],
        "visibilities": [AccessLevel.OBSERVER, AccessLevel.OBSERVER]
    }
    h["server.samedeviceclasses"] = None
    h["server.samedeviceclasses", ...] = {
        "host": "exflpxc_something",
        "visibility": AccessLevel.EXPERT,
        "deviceClasses": ["FooClass", "BlahClass", "HooClass_0"],
        "visibilities": [AccessLevel.OBSERVER, AccessLevel.OBSERVER,
                         AccessLevel.OBSERVER]
    }

    h["server.differentaccesslevel"] = None
    h["server.differentaccesslevel", ...] = {
        "host": "exflpxc_something",
        "visibility": AccessLevel.EXPERT,
        "deviceClasses": ["FooClass", "BarClass", "HooClass_1"],
        "visibilities": [AccessLevel.OBSERVER, AccessLevel.EXPERT,
                         AccessLevel.OBSERVER]
    }

    h["device.orphan"] = None
    h["device.orphan", ...] = {
        "host": "exflpxc_something",
        "visibility": AccessLevel.OBSERVER,
        "serverId": "myserver",
        "classId": "FooClass",
        "status": "ok",
        "interfaces": 0,
        "capabilities": 0,
    }

    h["device.eddie"] = None
    h["device.eddie", ...] = {
        "host": "exflpxc_something",
        "visibility": AccessLevel.OBSERVER,
        "serverId": "myserver",
        "classId": "BarClass",
        "status": "ok",
        "interfaces": 0,
        "capabilities": 0,
    }

    return h


class SimpleDeviceSchema(Configurable):
    """A simple device schema"""
    state = String(
        defaultValue=State.ON,
        displayType="State",
        enum=State)
    doubleProperty = Double(
        defaultValue=None,
        unitSymbol=Unit.METER)
    stringProperty = String(
        defaultValue="foo",
        options=["foo", "bar", "baz", "qux"])
    readOnlyProperty = Int32(
        defaultValue=0,
        accessMode=AccessMode.READONLY)
    availableScenes = VectorString(
        defaultValue=["scene"])


class DeviceSchemaAllowedState(SimpleDeviceSchema):
    """Device Schema with allowed state defined for an integer property"""
    intProperty = Int32(
        defaultValue=10,
        unitSymbol=Unit.METER,
        allowedStates={State.PASSIVE})

    initOnlyString = String(
        displayedName="Init Only String",
        defaultValue="Karabo",
        accessMode=AccessMode.INITONLY, )


def get_device_schema():
    return SimpleDeviceSchema.getClassSchema()


def get_device_schema_allowed_state():
    return DeviceSchemaAllowedState.getClassSchema()


def check_renderer_against_svg(renderer, svgfile):
    """Yeah... Check an SVG for elements which we suspect it should have.
    """

    def _get_path_ids(filename):
        tree = parse(filename)
        ids = []
        for elem in tree.iter('{http://www.w3.org/2000/svg}path'):
            ids.append(elem.get('id'))
        return ids

    for name in _get_path_ids(svgfile):
        assert renderer.elementExists(name)


@contextlib.contextmanager
def lazy_scene_reading():
    try:
        set_scene_reader(lazy=True)
        yield
    finally:
        set_scene_reader(lazy=False)
