# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from qtpy.QtCore import Qt

from karabo.common.api import (
    KARABO_SCHEMA_DISPLAY_TYPE, KARABO_SCHEMA_ROW_SCHEMA)
from karabo.native import Bool, Configurable, Hash, String
from karabogui.binding.api import (
    BindingNamespace, BindingRoot, BoolBinding, DeviceClassProxy,
    ListOfNodesBinding, PropertyProxy, StringBinding, TableBinding)
from karabogui.testing import GuiTestCase, set_proxy_value

from ..runconfigurator import NODE_CLASS_NAME, RunConfiguratorEdit


class _TableRow(Configurable):
    """Sub-schema of the 'sources' table.

    This is possibly useless, but it's here for completeness/documentation.
    """
    source = String(displayedName='Source', defaultValue='Source')
    type = String(displayedName='Type', options=['control', 'instrument'],
                  defaultValue='control')
    behavior = String(displayedName='Behavior', defaultValue='record-all',
                      options=['init', 'read-only', 'record-all'])
    monitored = Bool(displayedName='Monitor out', defaultValue=False)
    access = String(displayedName='Access', defaultValue='expert',
                    options=['expert', 'user'])


def _build_binding():
    """Thanks to the middlelayer not implementing ListOfNodes correctly, we
    have to build the binding by hand. Like an animal.
    """
    node_binding = BindingRoot(class_id=NODE_CLASS_NAME)
    node_binding.value.groupId = StringBinding()
    node_binding.value.use = BoolBinding()
    attributes = {KARABO_SCHEMA_ROW_SCHEMA: _TableRow.getClassSchema()}
    node_binding.value.sources = TableBinding(attributes=attributes)
    prop_namespace = BindingNamespace(item_type=BindingRoot)
    setattr(prop_namespace, NODE_CLASS_NAME, node_binding)

    device_binding = BindingRoot(class_id='Test')
    attributes = {KARABO_SCHEMA_DISPLAY_TYPE: 'RunConfigurator'}
    device_binding.value.prop = ListOfNodesBinding(choices=prop_namespace,
                                                   attributes=attributes)
    return device_binding


def _build_value():
    def _source_hash(name):
        return Hash('source', name, 'type', 'instrument',
                    'behavior', 'read-only', 'monitored', True,
                    'access', 'expert')

    value = Hash('groupId', 'Grouper', 'use', True)
    value['sources'] = [_source_hash('inst')]
    return [Hash(NODE_CLASS_NAME, value)]


class TestRunConfiguratorEdit(GuiTestCase):
    def setUp(self):
        super(TestRunConfiguratorEdit, self).setUp()
        binding = _build_binding()
        device = DeviceClassProxy(binding=binding)
        self.proxy = PropertyProxy(root_proxy=device, path='prop')
        self.controller = RunConfiguratorEdit(proxy=self.proxy)
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_focus_policy(self):
        assert self.controller.widget.focusPolicy() == Qt.StrongFocus

    def test_set_value(self):
        item_model = self.controller.widget.model()
        assert item_model.rowCount() == 0
        set_proxy_value(self.proxy, 'prop', _build_value())
        assert item_model.rowCount() == 1

    def test_edit_value(self):
        value = _build_value()
        set_proxy_value(self.proxy, 'prop', value)
        item_model = self.controller.widget.model()
        root = item_model.invisibleRootItem()
        child = root.child(0)
        state = (Qt.Unchecked if child.checkState() == Qt.Checked
                 else Qt.Checked)
        child.setCheckState(state)

        assert self.proxy.edit_value != value
