import sys
from os import path as op
from unittest import mock, skip

import requests

from karabogui.dialogs import update_dialog
from karabogui.dialogs.update_dialog import (
    EXTENSIONS_URL_TEMPLATE, KARABO_CHANNEL, UNDEFINED_VERSION)
from karabogui.testing import GuiTestCase


class MockResponse:
    def __init__(self, content, status):
        self.content = content
        self.status_code = status


class TestCase(GuiTestCase):

    def _get_loaded_modules(self):
        extensions = []
        for module in sys.modules:
            if module.startswith('extensions'):
                extensions.append(module)
        return extensions

    @skip(reason='Install is not working properly in CI')
    def test_current_version_bug(self):
        """Bug found when updating a package version. After the
        package is removed pkg_resources still thinks it's there."""
        wheel = op.join(op.dirname(__file__),
                        'GUI_Extensions-0.1.0-py3-none-any.whl')
        update_dialog.uninstall_package()
        assert update_dialog.get_current_version() == UNDEFINED_VERSION
        assert self._get_loaded_modules() == []

        update_dialog.install_package(wheel)
        assert update_dialog.get_current_version() == '0.1.0'
        assert self._get_loaded_modules() == [
            'extensions', 'extensions.display_ipm_quadrant',
            'extensions.models', 'extensions.models.simple']

        update_dialog.uninstall_package()
        assert update_dialog.get_current_version() == UNDEFINED_VERSION
        assert self._get_loaded_modules() == []

    def test_extensions_dialog(self):
        mock_obj = mock.patch.object
        m1 = mock_obj(update_dialog, 'get_current_version', return_value='1')
        m2 = mock_obj(update_dialog, 'get_latest_version', return_value='1')

        with m1, m2:
            dialog = update_dialog.UpdateDialog()

            assert not dialog.bt_update.isEnabled()
            assert dialog.lb_current.text() == '1'
            assert dialog.lb_latest.text() == '1'

            update_dialog.get_latest_version.return_value = '2'

            self.click(dialog.bt_refresh)

            # Should refresh current and latest versions
            self.assertEqual(dialog.lb_current.text(), '1')
            self.assertEqual(dialog.lb_latest.text(), '2')

            self.assertTrue(dialog.bt_update.isEnabled())

    def test_latest_version(self):
        """Tests the model of the extension updater"""
        html_file = op.join(op.dirname(__file__), 'test_html.html')
        with open(html_file) as f:
            html = f.read()

        with mock.patch.object(update_dialog,
                               'retrieve_remote_html',
                               return_value=html):
            remote_server = EXTENSIONS_URL_TEMPLATE.format(KARABO_CHANNEL)
            latest_version = update_dialog.get_latest_version(remote_server)
            assert latest_version == '2.3.4'

    @mock.patch.object(requests, 'get')
    def test_outgoing_messages(self, get_mock):
        """Tests if the outgoing messages are called with the right
        parameters"""
        expected_wheel = ('http://exflctrl01.desy.de/karabo/karaboExtensions'
                          '/tags/0.0.0/GUIExtensions-0.0.0-py3-none-any.whl')
        remote_server = EXTENSIONS_URL_TEMPLATE.format(KARABO_CHANNEL)

        get_mock.return_value = MockResponse(b'', requests.codes.ok)
        with update_dialog.download_file_for_tag('0.0.0', remote_server):
            pass

        # Assert outgoing messages
        get_mock.assert_called_once_with(expected_wheel, stream=True,
                                         timeout=0.5)

        # Emulate a <not ok> get request
        get_mock.return_value = MockResponse(b'', requests.codes.not_found)
        with update_dialog.download_file_for_tag('0.0.0', remote_server):
            pass

        assert get_mock.call_count == 2
        get_mock.assert_called_with(expected_wheel, stream=True, timeout=0.5)
