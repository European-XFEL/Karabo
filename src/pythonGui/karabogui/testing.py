from collections import OrderedDict
import contextlib
import sys
import unittest

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QApplication
from qtpy.QtTest import QTest

from karabo.common.api import Capabilities, ProxyStatus
from karabo.native import AccessLevel, Hash
from karabogui.alarms.api import (
    ACKNOWLEDGEABLE, ALARM_HIGH, ALARM_ID, ALARM_NONE, ALARM_TYPE, DESCRIPTION,
    DEVICE_ID, NEEDS_ACKNOWLEDGING, PROPERTY, TIME_OF_FIRST_OCCURENCE,
    TIME_OF_OCCURENCE)
from karabogui.background import create_background_timer
from karabogui.binding.api import (
    DeviceClassProxy, DeviceProxy, PropertyProxy, apply_configuration,
    build_binding)
from karabogui.controllers.api import populate_controller_registry
import karabogui.singletons.api as singletons_mod
from karabogui.util import process_qt_events


class GuiTestCase(unittest.TestCase):
    """ A convenient base class for gui test cases
    """

    def setUp(self):
        app = QApplication.instance()
        if app is None:
            app = QApplication(sys.argv)
        self.app = app
        create_background_timer()
        # AFTER the QApplication is created!
        from karabogui import globals
        globals.GLOBAL_ACCESS_LEVEL = AccessLevel.OPERATOR
        from karabogui import icons
        icons.init()
        populate_controller_registry()

    def tearDown(self):
        self.process_qt_events()
        self.app.deleteLater()

    def process_qt_events(self, ms=10):
        # Give the event loop 10ms to process its events
        process_qt_events(self.app, timeout=ms)

    def click(self, button_widget, button=Qt.LeftButton):
        QTest.mouseClick(button_widget, button)


def alarm_data():
    data = OrderedDict()
    data['entry1'] = {'uptype1': {
            ALARM_ID: 0,
            PROPERTY: 'choochability',
            DESCRIPTION: 'choochability unsufficient',
            ACKNOWLEDGEABLE: True,
            ALARM_TYPE: ALARM_HIGH,
            DEVICE_ID: 'Bobby',
            NEEDS_ACKNOWLEDGING: True,
            TIME_OF_OCCURENCE: '2017-04-20T10:32:22 UTC',
            TIME_OF_FIRST_OCCURENCE: '2017-04-20T09:32:22 UTC'}}
    data['entry2'] = {'uptype1': {
            ALARM_ID: 1,
            PROPERTY: 'choochness',
            DESCRIPTION: 'choochness over 90000',
            ACKNOWLEDGEABLE: False,
            ALARM_TYPE: ALARM_HIGH,
            DEVICE_ID: 'Jenny',
            NEEDS_ACKNOWLEDGING: False,
            TIME_OF_OCCURENCE: '2017-04-20T10:12:22 UTC',
            TIME_OF_FIRST_OCCURENCE: '2017-04-20T09:12:22 UTC'}}
    data['entry3'] = {'uptype1': {
            ALARM_ID: 1,
            PROPERTY: 'choochness',
            DESCRIPTION: 'choochness over 90000',
            ACKNOWLEDGEABLE: False,
            ALARM_TYPE: ALARM_NONE,
            DEVICE_ID: 'Frank',
            NEEDS_ACKNOWLEDGING: False,
            TIME_OF_OCCURENCE: '2017-04-20T10:12:22 UTC',
            TIME_OF_FIRST_OCCURENCE: '2017-04-20T09:12:22 UTC'}}
    return data


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
            msg = 'Expected change for trait "{}" was not observed!'
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


def get_property_proxy(schema, name, device_id='TestDevice'):
    """Given a device schema and a property name, return a complete
    PropertyProxy object with a `root_proxy` of type `DeviceProxy`.
    """
    binding = build_binding(schema)
    root_proxy = DeviceProxy(binding=binding, device_id=device_id)
    root_proxy.status = ProxyStatus.SCHEMA
    return PropertyProxy(root_proxy=root_proxy, path=name)


def get_class_property_proxy(schema, name):
    """Given a device schema and a property name, return a complete
    PropertyProxy object with a `root_proxy` of type `DeviceClassProxy`.
    """
    binding = build_binding(schema)
    root = DeviceClassProxy(binding=binding, server_id='Test',
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
    # XXX: Yes, we're being naughty here. It's for testing though...
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


def system_hash():
    """Generate a system hash which will be built into a system tree
    """
    h = Hash()

    h['server.swerver'] = None
    h['server.swerver', ...] = {
        'host': 'BIG_IRON',
        'visibility': AccessLevel.OBSERVER,
        'deviceClasses': ['FooClass', 'BarClass'],
        'visibilities': [AccessLevel.OBSERVER, AccessLevel.OBSERVER]
    }
    h['device.divvy'] = None
    h['device.divvy', ...] = {
        'host': 'BIG_IRON',
        'visibility': AccessLevel.OBSERVER,
        'capabilities': Capabilities.PROVIDES_SCENES,
        'serverId': 'swerver',
        'classId': 'FooClass',
        'status': 'online'
    }
    h['macro.macdonald'] = None
    h['macro.macdonald', ...] = {
        'host': 'BIG_IRON',
        'visibility': AccessLevel.OBSERVER,
        'serverId': 'swerver',
        'classId': 'BarClass',
        'status': 'incompatible'
    }

    h['device.orphan'] = None
    h['device.orphan', ...] = {
        'visibility': AccessLevel.OBSERVER,
        'serverId': '__none__',
        'classId': 'Parentless',
        'status': 'noserver'
    }

    return h


def system_hash_server_and_plugins():
    """Generate a system hash which will be built into a system tree
    """
    h = Hash()

    h['server.myserver'] = None
    h['server.myserver', ...] = {
        'host': 'exflpxc_something',
        'visibility': AccessLevel.OBSERVER,
        'deviceClasses': ['FooClass', 'BarClass'],
        'visibilities': [AccessLevel.OBSERVER, AccessLevel.OBSERVER]
    }
    h['server.samedeviceclasses'] = None
    h['server.samedeviceclasses', ...] = {
        'host': 'exflpxc_something',
        'visibility': AccessLevel.EXPERT,
        'deviceClasses': ['FooClass', 'BlahClass', 'HooClass_0'],
        'visibilities': [AccessLevel.OBSERVER, AccessLevel.OBSERVER,
                         AccessLevel.OBSERVER]
    }

    h['server.differentaccesslevel'] = None
    h['server.differentaccesslevel', ...] = {
        'host': 'exflpxc_something',
        'visibility': AccessLevel.EXPERT,
        'deviceClasses': ['FooClass', 'BarClass', 'HooClass_1'],
        'visibilities': [AccessLevel.OBSERVER, AccessLevel.EXPERT,
                         AccessLevel.OBSERVER]
    }

    h['device.orphan'] = None
    h['device.orphan', ...] = {
        'host': 'exflpxc_something',
        'visibility': AccessLevel.OBSERVER,
        'serverId': 'myserver',
        'classId': 'FooClass',
        'status': 'online'
    }

    h['device.eddie'] = None
    h['device.eddie', ...] = {
        'host': 'exflpxc_something',
        'visibility': AccessLevel.OBSERVER,
        'serverId': 'myserver',
        'classId': 'BarClass',
        'status': 'online'
    }

    return h
