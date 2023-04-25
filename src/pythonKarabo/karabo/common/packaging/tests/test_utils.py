# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
