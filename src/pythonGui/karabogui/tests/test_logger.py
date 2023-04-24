# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
