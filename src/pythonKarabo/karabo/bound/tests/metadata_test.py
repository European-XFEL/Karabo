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
# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.

import unittest

from karabo.bound import ChannelMetaData, Epochstamp, TimeId, Timestamp


class Metadata_TestCase(unittest.TestCase):
    def test_metadata_general_functionality(self):
        es = Epochstamp(1356441936, 789333123456789123)
        ts = Timestamp(es, TimeId(987654321))
        meta = ChannelMetaData('abc', ts)

        self.assertTrue(meta.getSource() == 'abc')
        self.assertTrue(meta.getTimestamp().getTid() == 987654321)
        self.assertTrue(meta.getTimestamp().getSeconds() == 1356441936)
        self.assertTrue(
            meta.getTimestamp().getFractionalSeconds() == 789333123456789123)
        self.assertTrue(
            meta.getTimestamp().toFormattedString() == ts.toFormattedString())

        self.assertTrue(meta.getTimestamp() == ts)

        meta.setSource('xyz')
        self.assertFalse(meta.getSource() == 'abc')
        self.assertTrue(meta.getSource() == 'xyz')

        ts1 = Timestamp()
        meta.setTimestamp(ts1)
        self.assertTrue(meta.getTimestamp().getTid() == 0)
        self.assertFalse(
            meta.getTimestamp().toFormattedString() == ts.toFormattedString())
        self.assertTrue(
            meta.getTimestamp().toFormattedString() == ts1.toFormattedString())


if __name__ == '__main__':
    unittest.main()
