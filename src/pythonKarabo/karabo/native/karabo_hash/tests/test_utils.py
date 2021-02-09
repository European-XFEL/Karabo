import numpy as np

from html.parser import HTMLParser

from ..hash import Hash, HashList
from ..utils import (
    create_html_hash, dtype_from_number, dictToHash, hashToDict,
    get_array_data, get_image_data)


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


def test_numpy_int32_dtype():
    dtype = dtype_from_number(12)
    assert dtype == np.int32


def test_numpy_object_dtype():
    dtype = dtype_from_number(55)
    assert dtype == np.object_


def test_numpy_int64_dtype():
    dtype = dtype_from_number(16)
    assert dtype == np.int64


def test_numpy_unknwon():
    dtype = dtype_from_number(39)
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
    h['data'] = Hash()
    h['data.data'] = np.array([2, 4, 6], np.int64)
    h['data.type'] = 16
    h['data.shape'] = np.array([3, 1], np.uint64)
    h['data.isBigEndian'] = False
    array = get_array_data(h, 'data', squeeze=False)
    np.testing.assert_array_equal(
        array, np.array([[2], [4], [6]], dtype=np.int64))
    assert array.shape == (3, 1)


def test_create_hash_html():
    h = Hash()
    h["int"] = 2
    h["hash"] = Hash("float", 6.4, "int", 8)
    h["hashlist"] = HashList(Hash("float", 5.2, "int", 6))

    html = create_html_hash(h)

    parser = HashHtmlParser()
    parser.feed(html)

    # The Hash html is expected to consist of a single table
    # with adjancent cells for each of the Hash's key / value
    # pairs. There may also be cells for Hash nodes.
    assert "table" in parser.tags_count
    assert "tr" in parser.tags_count
    assert "td" in parser.tags_count
    # One table for <table> and other for </table>
    assert parser.tags_count["table"] == 2
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

    int_key_idx = parser.cells_data.index('int', float_val_idx+1)
    int_val_idx = parser.cells_data.index('8', float_val_idx+1)
    assert int_val_idx == int_key_idx + 1

    # TODO: Handle vector of hash type in the utils.create_html_hash function.
    #       The vector of hash doesn't make into the generated html.


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
