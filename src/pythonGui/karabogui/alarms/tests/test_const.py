from nose.tools import assert_raises
from PyQt4.QtGui import QIcon
from PyQt4.QtCore import QSize

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

    def test_get_alarm_pixmap(self):
        bm = const.get_alarm_pixmap(const.ALARM_GLOBAL)
        assert QSize(16, 16) == bm.size()
