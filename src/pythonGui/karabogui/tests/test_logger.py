from karabogui.logger import get_logger, StatusBarHandler

from karabogui.testing import GuiTestCase


class TestCase(GuiTestCase):

    def test_log_widget(self):
        logger = get_logger()
        handler = StatusBarHandler()
        log_widget = handler.get_log_widget()
        # add handler to logger
        logger.addHandler(handler)
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
        assert "DEBUG" in log_widget.toPlainText()
