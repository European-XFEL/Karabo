# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import unittest

from karabo.packaging import versioning


class TestCase(unittest.TestCase):

    def test_versioning_bug(self):
        import karabo

        karabo._version.version = '2.2.2a2.dev240+g53ea62a'
        assert versioning.get_karabo_version() == '2.2.2a2.dev240'

        version = versioning.get_package_version(karabo.__file__)
        assert version == '2.2.2a2.dev240'
