from unittest import TestCase

from PyQt5.QtGui import QValidator

from ..webdialog import WebValidator


class TestWebValidator(TestCase):

    def setUp(self):
        self._validator = WebValidator()

    def test_valid_url(self):
        user_pass_url = "http://username:password@someurl.com"
        ip_address = "192.168.1.1"
        localhost_with_port = "localhost:8080"
        simple_website = "google.com"
        query_url = "ftp://someurl.com/page.html?param1=1&param2=2"

        self._assert_url(user_pass_url, valid=True)
        self._assert_url(ip_address, valid=True)
        self._assert_url(localhost_with_port, valid=True)
        self._assert_url(simple_website, valid=True)
        self._assert_url(query_url, valid=True)
        self._assert_url("invalid url", valid=False)

    def _assert_url(self, url, *, valid):
        result, _, _ = self._validator.validate(url, pos=0)

        if valid:
            self.assertEqual(result, QValidator.Acceptable)
        else:
            self.assertNotEqual(result, QValidator.Acceptable)
