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
from html.parser import HTMLParser

import numpy as np
import pytest

from ..hash import Hash, HashList
from ..utils import (
    HashListFormat, create_html_hash, dictToHash, dtype_from_number,
    get_array_data, get_image_data, hashToDict)


class HashHtmlParser(HTMLParser):

    def __init__(self):

        super().__init__()

        # The data in table cells.
        # Keys and their corresponding values in the
        # original Hash must be adjacent in this list.
        self.cells_data = []

        # A dictionary that will contain the frequencies
        # of tags found. The key is the tag, like "table"
        # and "td" and the value is the number of times
        # each tag has been found in the html.
        self.tags_count = {}

    def handle_starttag(self, tag, attrs):
        self.handle_tag(tag)

    def handle_endtag(self, tag):
        self.handle_tag(tag)

    def handle_tag(self, tag):
        if tag in self.tags_count:
            self.tags_count[tag] += 1
        else:
            self.tags_count[tag] = 1

    def handle_data(self, data):
        # All the data in the Hash html is expected
        # to be in table cells.
        self.cells_data.append(data)


def test_numpy_number():
    dtype = dtype_from_number(1)
    assert dtype == np.bool_
    dtype = dtype_from_number(2)
    assert dtype == np.object_
    dtype = dtype_from_number(3)
    assert dtype == np.object_
    dtype = dtype_from_number(4)
    assert dtype == np.int8
    dtype = dtype_from_number(5)
    assert dtype == np.int8
    dtype = dtype_from_number(6)
    assert dtype == np.uint8
    dtype = dtype_from_number(7)
    assert dtype == np.uint8
    dtype = dtype_from_number(8)
    assert dtype == np.int16
    dtype = dtype_from_number(9)
    assert dtype == np.int16
    dtype = dtype_from_number(10)
    assert dtype == np.uint16
    dtype = dtype_from_number(11)
    assert dtype == np.uint16
    dtype = dtype_from_number(12)
    assert dtype == np.int32
    dtype = dtype_from_number(13)
    assert dtype == np.int32
    dtype = dtype_from_number(14)
    assert dtype == np.uint32
    dtype = dtype_from_number(15)
    assert dtype == np.uint32
    dtype = dtype_from_number(16)
    assert dtype == np.int64
    dtype = dtype_from_number(17)
    assert dtype == np.int64
    dtype = dtype_from_number(18)
    assert dtype == np.uint64
    dtype = dtype_from_number(19)
    assert dtype == np.uint64
    dtype = dtype_from_number(55)
    assert dtype == np.object_


def test_get_image_data():
    h = Hash()
    h['data.image.pixels'] = Hash()
    h['data.image.pixels.data'] = np.array([[2, 4, 6], [6, 8, 10]], np.int64)
    h['data.image.pixels.type'] = 16
    h['data.image.pixels.shape'] = np.array([2, 3], np.uint64)
    h['data.image.pixels.isBigEndian'] = False
    image = get_image_data(h)
    np.testing.assert_array_equal(
        image, np.array([[2, 4, 6], [6, 8, 10]], np.int64))

    h = Hash()
    h['schema.image.pixels'] = Hash()
    h['schema.image.pixels.data'] = np.array([[2, 4, 6], [6, 8, 10]], np.int64)
    h['schema.image.pixels.type'] = 16
    h['schema.image.pixels.shape'] = np.array([2, 3], np.uint64)
    h['schema.image.pixels.isBigEndian'] = False
    image = get_image_data(h, path='schema.image')
    np.testing.assert_array_equal(
        image, np.array([[2, 4, 6], [6, 8, 10]], np.int64))


def test_get_array_data():
    h = Hash()
    h['data'] = Hash()
    h['data.data'] = np.array([[2, 4, 6], [6, 8, 10]],
                              dtype=np.int64)
    h['data.type'] = 16
    h['data.shape'] = np.array([2, 3], dtype=np.uint64)
    h['data.isBigEndian'] = False
    array = get_array_data(h, 'data')
    np.testing.assert_array_equal(
        array, np.array([[2, 4, 6], [6, 8, 10]], dtype=np.int64))
    assert array.shape == (2, 3)

    # Squeeze happens for single dimensions default
    h['data'] = Hash()
    h['data.data'] = np.array([2, 4, 6], dtype=np.int64)
    h['data.type'] = 16
    h['data.shape'] = np.array([3, 1], dtype=np.uint64)
    h['data.isBigEndian'] = False
    array = get_array_data(h, 'data')
    np.testing.assert_array_equal(
        array, np.array([2, 4, 6], dtype=np.int64))
    assert array.shape == (3,)

    # Squeeze deactivated
    h['round'] = Hash()
    h['round.data'] = np.array([2, 4, 6], np.int64)
    h['round.type'] = 16
    h['round.shape'] = np.array([3, 1], np.uint64)
    h['round.isBigEndian'] = False
    array = get_array_data(h, 'round', squeeze=False)
    np.testing.assert_array_equal(
        array, np.array([[2], [4], [6]], dtype=np.int64))
    assert array.shape == (3, 1)

    # This method is expected to throw a `KeyError` if not
    # all information is found!
    h = Hash('data', Hash())
    with pytest.raises(KeyError):
        get_array_data(h, 'data')

    h['data'] = Hash()
    h['data.data'] = 2.2
    with pytest.raises(KeyError):
        get_array_data(h, 'data')


def test_create_hash_html():
    h = Hash()
    h["int"] = 2
    h["hash"] = Hash("float", 6.4, "int", 8)
    h["hashlist"] = HashList([Hash("float", 5.2, "int", 6)])
    h["goofylist"] = HashList([Hash("float", 5.2),
                               Hash("int", 6, "hash", Hash("hidden", -1))])
    h["lon"] = HashList([Hash("NetworkHandler", Hash("file", "none"))])

    html = create_html_hash(h)

    parser = HashHtmlParser()
    parser.feed(html)

    # The Hash html is expected to consist of a single table
    # with adjancent cells for each of the Hash's key / value
    # pairs. There may also be cells for Hash nodes.
    assert "table" in parser.tags_count
    assert "tr" in parser.tags_count
    assert "td" in parser.tags_count
    # Two tables, one for the Hash and another for the HashList key.
    # Each table has a pair of (<table>, </table>) tags, hence the 4.
    assert parser.tags_count["table"] == 4
    # Each <tr> is matched by a </tr>
    assert parser.tags_count["tr"] % 2 == 0
    # Each <td> is matched by a </td>
    assert parser.tags_count["td"] % 2 == 0

    # The hash keys and their corresponding values should be
    # adjacent.
    int_key_idx = parser.cells_data.index('int')
    int_val_idx = parser.cells_data.index('2')
    assert int_val_idx == int_key_idx + 1

    hash_node_idx = parser.cells_data.index('hash')
    assert hash_node_idx > int_val_idx

    float_key_idx = parser.cells_data.index('float')
    float_val_idx = parser.cells_data.index('6.4')
    assert float_val_idx == float_key_idx + 1

    int_key_idx = parser.cells_data.index('int', float_val_idx + 1)
    int_val_idx = parser.cells_data.index('8', float_val_idx + 1)
    assert int_val_idx == int_key_idx + 1

    hashlist_key_idx = parser.cells_data.index('hashlist', int_val_idx + 1)
    assert parser.cells_data[hashlist_key_idx] == 'hashlist'
    assert parser.cells_data[hashlist_key_idx + 1] == '\n'
    assert parser.cells_data[hashlist_key_idx + 2] == '\n'
    assert parser.cells_data[hashlist_key_idx + 3] == ' float '
    assert parser.cells_data[hashlist_key_idx + 4] == ' int '
    assert parser.cells_data[hashlist_key_idx + 5] == '\n'
    assert parser.cells_data[hashlist_key_idx + 6] == '\n'
    assert parser.cells_data[hashlist_key_idx + 7] == '\n'
    assert parser.cells_data[hashlist_key_idx + 8] == '  5.2  '
    assert parser.cells_data[hashlist_key_idx + 9] == '  6  '

    goofy_idx = parser.cells_data.index('goofylist', hashlist_key_idx + 10)
    assert parser.cells_data[goofy_idx] == 'goofylist'
    assert parser.cells_data[goofy_idx + 1] == 'HashList[Unknown Format]'

    lon_idx = parser.cells_data.index('lon', goofy_idx + 2)
    assert parser.cells_data[lon_idx] == 'lon'
    assert parser.cells_data[lon_idx + 1] == 'NetworkHandler'
    assert parser.cells_data[lon_idx + 2] == 'file'
    assert parser.cells_data[lon_idx + 3] == 'none'


def test_dict_hash():
    """Test that a dict can be moved to a Hash and vice versa"""
    b = {"b": 2}
    e = ["a", "b", "c"]
    f = [{"a": 1}, {"b": 2}]
    d = {"a": 1, "c": 3, "d": 4, "b": b, "e": e, "f": f}

    h = dictToHash(d)
    assert h["a"] == 1
    node = h["b"]
    assert isinstance(node, Hash)
    assert h["b.b"] == 2
    assert h["e"] == e
    node = h["f"]
    assert isinstance(node, HashList)
    assert node[0] == Hash("a", 1)
    assert node[1] == Hash("b", 2)

    new_d = hashToDict(h)
    assert new_d == d


def test_hashlist_format():
    h = HashList()
    f = HashList.hashlist_format(h)
    assert f is HashListFormat.Unknown

    h = HashList([Hash()])
    f = HashList.hashlist_format(h)
    assert f is HashListFormat.Unknown

    h = HashList([Hash("float", 5.2, "int", 6)])
    f = HashList.hashlist_format(h)
    assert f is HashListFormat.Table

    h = HashList([Hash("float", 5.2),
                  Hash("int", 6, "hash", Hash("hidden", -1))])
    f = HashList.hashlist_format(h)
    assert f is HashListFormat.Unknown

    h = HashList([Hash("NetworkHandler", Hash())])
    f = HashList.hashlist_format(h)
    assert f is HashListFormat.ListOfNodes
