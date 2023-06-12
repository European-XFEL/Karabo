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
from unittest import main, mock

from qtpy.QtCore import QPoint

from karabogui.logger import StatusLogWidget, get_logger
from karabogui.testing import GuiTestCase


class TestCase(GuiTestCase):

    def test_log_widget(self):
        logger = get_logger()
        widget = StatusLogWidget()
        log_widget = widget.log_widget
        # add handler to logger
        assert log_widget.toPlainText() == ""
        msg = "Once upon a time ..."
        logger.info(msg)
        assert msg in log_widget.toPlainText()
        assert "INFO" in log_widget.toPlainText()
        assert "ERROR" not in log_widget.toPlainText()
        logger.error(msg)
        assert msg in log_widget.toPlainText()
        assert "ERROR" in log_widget.toPlainText()
        assert "INFO" in log_widget.toPlainText()
        assert "DEBUG" not in log_widget.toPlainText()
        assert log_widget.toPlainText().count('\n') == 1
        log_widget.clear()
        assert log_widget.toPlainText().count('\n') == 0
        logger.debug(msg)
        assert "DEBUG" not in log_widget.toPlainText()

        with mock.patch("karabogui.logger.QMenu") as m:
            widget._show_context_menu(QPoint(0, 0))
            self.process_qt_events()
            m.assert_called()


if __name__ == "__main__":
    main()
