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

from karabo.bound import (
    INPUT_CHANNEL, INT32_ELEMENT, OUTPUT_CHANNEL, ChannelMetaData, Epochstamp,
    Schema, TimeId, Timestamp)


def test_metadata_general_functionality():
    es = Epochstamp(1356441936, 789333123456789123)
    ts = Timestamp(es, TimeId(987654321))
    meta = ChannelMetaData('abc', ts)

    assert meta.getSource() == 'abc'
    assert meta.getTimestamp().getTid() == 987654321
    assert meta.getTimestamp().getSeconds() == 1356441936
    assert meta.getTimestamp().getFractionalSeconds() == 789333123456789123
    assert meta.getTimestamp().toFormattedString() == ts.toFormattedString()
    # Equality operator is bound
    assert meta.getTimestamp() == ts

    meta.setSource('xyz')
    assert meta.getSource() != 'abc'
    assert meta.getSource() == 'xyz'

    ts1 = Timestamp()
    meta.setTimestamp(ts1)
    assert meta.getTimestamp().getTid() == 0
    assert meta.getTimestamp().toFormattedString() != ts.toFormattedString()
    assert meta.getTimestamp().toFormattedString() == ts1.toFormattedString()


def test_channel_schema_declarations():
    pipeSchema = Schema()
    INT32_ELEMENT(pipeSchema).key("int32").readOnly().commit()

    s = Schema()
    (
        OUTPUT_CHANNEL(s)
        .key('validkey')
        .displayedName('ValidKey')
        .description('Valid key description')
        .dataSchema(pipeSchema)
        .commit(),

        INPUT_CHANNEL(s)
        .key('input')
        .displayedName('Input key')
        .description('Input key description')
        .dataSchema(pipeSchema)
        .commit(),
    )

    assert s.has("validkey") and s.has("input") is True
    assert s.getDisplayedName('validkey') == 'ValidKey'
    assert s.getDisplayedName('input') == 'Input key'
    assert s.getDescription('validkey') == 'Valid key description'
    assert s.getDescription('input') == 'Input key description'
    assert s.has("validkey.schema.int32") is True
