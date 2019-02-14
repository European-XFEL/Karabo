import requests
from contextlib import contextmanager
from pathlib import Path
from unittest import mock, skip

from karabogui.dialogs.extensions_updater import UpdateDialog, \
    extensions_updater as updater
from karabogui.testing import GuiTestCase


class TestCase(GuiTestCase):

    @skip(reason='Cannot test dialogs in the current Qt version. Should be '
                 'tested and reviewed after PyQt5')
    def test_extensions_dialog(self):
        dialog = UpdateDialog()
        updater = dialog.updater

        self.assertTrue(dialog.bt_update.isDisabled())
        self.assertEqual(dialog.lb_current.text(), updater.UNDEFINED_VERSION)
        self.assertEqual(dialog.lb_latest.text(), updater.UNDEFINED_VERSION)

        with mock.patch.object(
                updater, 'get_current_version',
                return_value='1'):
            with mock.patch.object(
                    updater, 'get_latest_version',
                    return_value='2'):
                self.click(dialog.bt_refresh)

                # Should refresh current and latest versions
                self.assertEqual(dialog.lb_current.text(), '1')
                self.assertEqual(dialog.lb_latest.text(), '2')

                self.assertTrue(dialog.bt_update.isEnabled())

    def test_latest_version(self):
        """Tests the model of the extension updater"""
        html = Path(__file__).parent.joinpath('test_html.html').read_text()

        with mock.patch.object(updater,
                               '_retrieve_remote_html',
                               return_value=html):
            latest_version = updater.get_latest_version()
            assert latest_version == '2.3.4'

    def _create_get_mock(self, content, status_code):
        """Used to mock a requests.get method call with a fake content
        and status code"""
        class MockResponse:
            def __init__(self, content, status):
                self.content = content
                self.status_code = status

        @contextmanager
        def _mock_response(*args, **kwargs):
            response = MockResponse(content, status_code)
            yield response

        return _mock_response

    @mock.patch.object(requests, 'get')
    def test_outgoing_messages(self, get_mock):
        """Tests if the outgoing messages are called with the right
        parameters"""
        expected_wheel = 'http://exflserv05.desy.de/karabo/karaboExtensions' \
                         '/tags/0.0.0/GUI_Extensions-0.0.0-py3-none-any.whl'

        get_mock.side_effect = self._create_get_mock(b'', requests.codes.ok)
        with updater.download_file_for_tag('0.0.0'):
            pass

        # Assert outgoing messages
        get_mock.assert_called_once_with(
            expected_wheel,
            stream=True)

        # Emulate a <not ok> get request
        get_mock.side_effect = self._create_get_mock(b'',
                                                     requests.codes.not_found)
        with updater.download_file_for_tag('0.0.0'):
            pass

        assert get_mock.call_count == 2
        get_mock.assert_called_with(
            expected_wheel,
            stream=True)
