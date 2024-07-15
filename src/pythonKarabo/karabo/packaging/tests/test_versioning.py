# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import unittest

from karabo.packaging import versioning
from packaging.version import InvalidVersion, Version


class TestCase(unittest.TestCase):

    def test_versioning_bug(self):
        import karabo

        karabo._version.version = '2.2.2a2.dev240+g53ea62a'
        assert versioning.get_karabo_version() == '2.2.2a2.dev240'

        version = versioning.get_package_version(karabo.__file__)
        assert version == '2.2.2a2.dev240'

    def test_filter_major_minor_micro(self):
        from unittest.mock import patch

        from karabo.packaging.versioning import get_karabo_framework_version

        p = "karabo.packaging.versioning._read_version_file"
        with patch(p) as mock_version_read:
            mock_version_read.return_value = "2.20.0rc1.dev21+g30"
            ver = get_karabo_framework_version()
            assert ver == Version("2.20.0rc1.dev21+g30")
            assert ver.is_devrelease
            assert not ver.is_postrelease
            assert ver.public == "2.20.0rc1.dev21"
            assert ver.local == "g30"

            mock_version_read.return_value = "2.20"
            assert get_karabo_framework_version() == Version("2.20.0")

            mock_version_read.return_value = "2.20.0"
            assert get_karabo_framework_version() == Version("2.20.0")

            mock_version_read.return_value = "2.02.0"
            assert get_karabo_framework_version() == Version("2.2.0")

            mock_version_read.return_value = "a2.20.0"
            with self.assertRaises(InvalidVersion):
                get_karabo_framework_version(strict_fallback=False)
            with self.assertRaises(InvalidVersion):
                # For this case, the fallback is not effective.
                get_karabo_framework_version()

            mock_version_read.return_value = "2.20.0rc1.dev21-g30"
            # Without the fallback, the string above fails
            with self.assertRaises(InvalidVersion):
                # For full details of the semver grammar that defines what a
                # valid Version string is:
                # https://semver.org/#backusnaur-form-grammar-for-valid-semver-versions
                # The hifen, in this case, is in an invalid position.
                get_karabo_framework_version(strict_fallback=False)
            # For this case, the fallback is effective.
            assert get_karabo_framework_version() == Version("2.20.0")
