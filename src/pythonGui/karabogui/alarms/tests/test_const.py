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
from unittest import mock, skip

from pytest import raises as assert_raises
from qtpy.QtCore import QSize
from qtpy.QtGui import QIcon, QPixmap

from karabogui.testing import GuiTestCase

from .. import const


class TestConst(GuiTestCase):

    def test_get_alarm_key_index(self):
        idx_id = const.get_alarm_key_index('id')
        assert idx_id == 7
        with assert_raises(ValueError):
            const.get_alarm_key_index('Bob')

    def test_get_alarm_icon(self):
        alarmicon = const.get_alarm_icon(const.ALARM_GLOBAL)
        assert isinstance(alarmicon, QIcon)

    # FIXME: patching fails on installed packages
    @skip(reason="Patching fails")
    @mock.patch('karabogui.alarms.const.get_alarm_pixmap')
    def test_get_alarm_pixmap(self, pixmap):
        pixmap.return_value = QPixmap(QSize(16, 16))
        bm = const.get_alarm_pixmap(const.ALARM_HIGH)
        assert QSize(16, 16) == bm.size()
