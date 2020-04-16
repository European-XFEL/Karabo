from unittest import mock, skip

from nose.tools import assert_raises
from PyQt5.QtGui import QIcon, QPixmap
from PyQt5.QtCore import QSize

from karabogui.testing import GuiTestCase
from .. import const


class TestConst(GuiTestCase):

    def test_get_alarm_key_index(self):
        idx_id = const.get_alarm_key_index('id')
        assert idx_id == 0
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
