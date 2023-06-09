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


class TestCase(unittest.TestCase):

    def test_versioning_bug(self):
        import karabo

        karabo._version.version = '2.2.2a2.dev240+g53ea62a'
        assert versioning.get_karabo_version() == '2.2.2a2.dev240'

        version = versioning.get_package_version(karabo.__file__)
        assert version == '2.2.2a2.dev240'
