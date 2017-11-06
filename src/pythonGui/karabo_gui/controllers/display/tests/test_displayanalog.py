from unittest.mock import patch

import numpy as np

from karabo.common.api import DeviceStatus
from karabo.middlelayer import Configurable, Float
from karabo_gui.binding.api import (
    DeviceClassProxy, PropertyProxy, build_binding
)
from karabo_gui.testing import GuiTestCase
from ..displayanalog import DisplayAnalog


class ObjectWithAlarms(Configurable):
    prop = Float(defaultValue=0.0,
                 warnLow=-5.0, alarmLow=-10.0,
                 warnHigh=5.0, alarmHigh=10.0)


class ObjectWithoutAlarms(Configurable):
    prop = Float(defaultValue=0.0)


def _get_pixmap_buffer(pixmap):
    img = pixmap.toImage()
    byte_count = img.byteCount()
    buffer = img.constBits().asarray(byte_count)  # A sip.array object...
    return np.asarray(buffer)


class TestDisplayAnalog(GuiTestCase):
    def setUp(self):
        super(TestDisplayAnalog, self).setUp()

        schema = ObjectWithAlarms.getClassSchema()
        binding = build_binding(schema)
        device = DeviceClassProxy(binding=binding, server_id='Fake',
                                  status=DeviceStatus.OFFLINE)
        self.proxy = PropertyProxy(root_proxy=device, path='prop')
        self.controller = DisplayAnalog(proxy=self.proxy)
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        # exercise the code paths...
        self.proxy.value = -11.0
        self.proxy.value = -7.0
        self.proxy.value = 7.0
        self.proxy.value = 11.0

        # Do a lame check to make sure it redraws when the value changes
        before = _get_pixmap_buffer(self.controller.widget.pixmap())
        self.proxy.value = 42.0
        after = _get_pixmap_buffer(self.controller.widget.pixmap())
        # XXX: hard to compare drawings...
        assert not np.all(before == after)

    def test_no_alarms_messagebox(self):
        try:
            schema = ObjectWithoutAlarms.getClassSchema()
            sym = 'karabo_gui.controllers.display.displayanalog.messagebox'
            with patch(sym) as messagebox:
                build_binding(schema, existing=self.proxy.root_proxy.binding)
                assert messagebox.show_warning.call_count == 1
        finally:
            # Put things back as they were!
            schema = ObjectWithAlarms.getClassSchema()
            build_binding(schema, existing=self.proxy.root_proxy.binding)
