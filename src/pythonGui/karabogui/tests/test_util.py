import unittest

from ..util import _get_invalid_chars


class InvalidFilenameTestCase(unittest.TestCase):

    def test_basics(self):
        self._assert_filename("foo", invalid='')
        self._assert_filename("FoO/Bar-bAz_123", invalid='')
        self._assert_filename("foo bar", invalid=' ')
        self._assert_filename("foo!@#$%&*()+<>@:", invalid='!@#$%&*()+<>:')

    def _assert_filename(self, filename, *, invalid):
        invalid_chars = _get_invalid_chars(filename)
        self.assertEqual(set(list(invalid)), set(invalid_chars))
