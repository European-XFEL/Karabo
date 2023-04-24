# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.

import unittest

from karabo.bound import ChannelMetaData, Epochstamp, Timestamp, Trainstamp


class Metadata_TestCase(unittest.TestCase):
    def test_metadata_general_functionality(self):
        es = Epochstamp(1356441936, 789333123456789123)
        ts = Timestamp(es, Trainstamp(987654321))
        meta = ChannelMetaData('abc', ts)

        self.assertTrue(meta.getSource() == 'abc')
        self.assertTrue(meta.getTimestamp().getTrainId() == 987654321)
        self.assertTrue(meta.getTimestamp().getSeconds() == 1356441936)
        self.assertTrue(
            meta.getTimestamp().getFractionalSeconds() == 789333123456789123)
        self.assertTrue(
            meta.getTimestamp().toFormattedString() == ts.toFormattedString())
        self.assertFalse(meta.getTimestamp() == ts)

        meta.setSource('xyz')
        self.assertFalse(meta.getSource() == 'abc')
        self.assertTrue(meta.getSource() == 'xyz')

        ts1 = Timestamp()
        meta.setTimestamp(ts1)
        self.assertTrue(meta.getTimestamp().getTrainId() == 0)
        self.assertFalse(
            meta.getTimestamp().toFormattedString() == ts.toFormattedString())
        self.assertTrue(
            meta.getTimestamp().toFormattedString() == ts1.toFormattedString())


if __name__ == '__main__':
    unittest.main()
