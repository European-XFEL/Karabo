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
from unittest import TestCase

from karabo.common.packaging import utils


class Test(TestCase):
    def test_regex_extraction(self):
        sample = "2.3.0rc15.dev34+gd916e0e.d20181218"
        self.assertEqual(utils.extract_base_version(sample), "2.3.0rc15")
        self.assertEqual(utils.extract_full_version(sample), "2.3.0rc15.dev34")

        sample = "2.3.0a2.dev33+g17872b7"
        self.assertEqual(utils.extract_base_version(sample), "2.3.0a2")
        self.assertEqual(utils.extract_full_version(sample), "2.3.0a2.dev33")

        # Incorrect versions
        self.assertEqual(utils.extract_base_version("rc15.dev34"), "")
        self.assertEqual(utils.extract_base_version("dev34"), "")
