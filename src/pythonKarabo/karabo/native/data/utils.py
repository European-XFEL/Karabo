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
from xml.sax.saxutils import escape

import numpy as np
import tabulate

from .hash import Hash, HashList, HashListFormat
from .typenums import HashType

__all__ = ['create_html_hash', 'dictToHash', 'dtype_from_number',
           'get_array_data', 'get_image_data', 'hashToDict',
           'numpy_from_number']


def dtype_from_number(number):
    """Return the dtype object matching the Karabo Types number

    in case of missing numpy definition in `Types`, returns an object_.
    >> dtype_from_number(16)
    >> dtype('int64')
    """
    return np.dtype(numpy_from_number(number))


def numpy_from_number(number, default=np.object_):
    """Return the numpy dtype class matching the Karabo Types number

    In case of missing numpy definition, returns an the `default`.
    >> numpy_from_number(16)
    >> numpy.int64
    """

    _TYPE = {
        HashType.VectorBool: np.bool_,
        HashType.VectorInt8: np.int8,
        HashType.VectorUInt8: np.uint8,
        HashType.VectorInt16: np.int16,
        HashType.VectorUInt16: np.uint16,
        HashType.VectorInt32: np.int32,
        HashType.VectorUInt32: np.uint32,
        HashType.VectorInt64: np.int64,
        HashType.VectorUInt64: np.uint64,
        HashType.VectorFloat: np.float32,
        HashType.VectorDouble: np.float64,

        HashType.Bool: np.bool_,
        HashType.Int8: np.int8,
        HashType.UInt8: np.uint8,
        HashType.Int16: np.int16,
        HashType.UInt16: np.uint16,
        HashType.Int32: np.int32,
        HashType.UInt32: np.uint32,
        HashType.Int64: np.int64,
        HashType.UInt64: np.uint64,
        HashType.Float: np.float32,
        HashType.Double: np.float64,
    }
    try:
        h_type = HashType(number)
    except Exception:
        return default

    return _TYPE.get(h_type, default)


def get_array_data(data, path=None, squeeze=True):
    """Method to extract an ``ndarray`` from a raw Hash

    :param data: A hash containing the data hash
    :param path: The path of the NDArray. If `None` (default) the
                 input Hash is taken.
    :param squeeze: If the array should be squeezed if the
                    latest dimension is 1. Default is `True`.

    :returns: A numpy array containing the extracted data
    """
    text = "Expected a Hash, but got type %s instead!" % type(data)
    assert isinstance(data, Hash), text
    array = _build_ndarray(data, path=path, squeeze=squeeze)

    return array


def _build_ndarray(data, path=None, squeeze=False):
    """Internal method to extract an ``ndarray`` from a raw Hash

    :param data: A hash containing the hash
    :param path: The path of the NDArray. If `None` (default) the
                 input Hash is taken.
    :param squeeze: If the array should be squeezed if the
                    latest dimension is 1. Default is `False`.
    """
    if path is not None:
        data = data[path]
    dtype = dtype_from_number(data["type"])
    dtype = dtype.newbyteorder(">" if data["isBigEndian"] else "<")
    array = np.frombuffer(data["data"], count=data["shape"].prod(),
                          dtype=dtype)
    array.shape = data["shape"]
    if squeeze and array.shape[-1] == 1:
        array = np.squeeze(array)

    return array


def get_image_data(data, path=None):
    """Method to extract an ``image`` from a Hash into a numpy array

    This is a common use case when processing pipeline data (receiving from an
    output channel)

    :param data: A hash containing the image data hash
    :param path: optional path of the image data. If no path is provided,
                 the default checks if the data hash contains paths with
                 `data.image` or `image` and get the image data.

    :returns: A numpy array containing the extracted pixel data
    """
    text = "Expected a Hash, but got type %s instead!" % type(data)
    assert isinstance(data, Hash), text
    if path is not None:
        return _build_ndarray(data[path], path="pixels")
    elif "data.image" in data:
        return _build_ndarray(data["data.image"], path="pixels")
    elif "image" in data:
        return _build_ndarray(data["image"], path="pixels")


def create_html_hash(hsh, include_attributes=True):
    """Create the HTML representation of a Hash"""
    assert isinstance(hsh, Hash), "An input of type ``Hash`` is required!"

    def _html_attributes(nest, attrs):
        for key, value in attrs.items():
            yield ('<tr><td style="padding-left:{}em">'
                   '<font size="1" color="red">'
                   '{}</td><td>'.format(nest + 1, key))
            yield f'<font size="1" color="red">{escape(str(value))}'

    def _html_hash_generator(hsh, nest=0):
        if nest == 0:
            yield "<table>"
        for key, value, attr in hsh.iterall():
            if isinstance(value, Hash):
                yield ('<tr><td style="padding-left:{}em"><b>{}</b></td>'
                       '<td/></tr>'.format(nest + 1, key))
                yield from _html_hash_generator(value, nest + 1)
            elif isinstance(value, HashList):
                yield ('<tr><td style="padding-left:{}em">{}</td><td>'
                       .format(nest + 1, key))
                if value:
                    list_format = HashList.hashlist_format(value)
                    if list_format is HashListFormat.Unknown:
                        yield "HashList[Unknown Format]"
                    elif list_format is HashListFormat.Table:
                        table = tabulate.tabulate(value, headers="keys",
                                                  stralign="center",
                                                  numalign="center",
                                                  tablefmt="html")
                        yield table
                    elif list_format is HashListFormat.ListOfNodes:
                        for hsh in value:
                            yield from _html_hash_generator(hsh, nest + 1)
                else:
                    yield "[]"
                yield '</td></tr>'
                if include_attributes:
                    yield from _html_attributes(nest + 1, attr)
            else:
                yield ('<tr><td style="padding-left:{}em">{}</td><td>'
                       .format(nest + 1, key))
                yield escape(str(value))
                if include_attributes:
                    yield from _html_attributes(nest + 1, attr)
                yield '</td></tr>'
        if nest == 0:
            yield "</table>"

    return "".join(_html_hash_generator(hsh))


def dictToHash(d):
    """Convert a nested dictionary into a `Hash`"""
    h = Hash()
    for k, v in d.items():
        if isinstance(v, dict):
            h[k] = dictToHash(v)
        elif isinstance(v, (list, tuple)):
            if len(v) > 0 and isinstance(v[0], dict):
                # Note: This is a VectorHash
                h[k] = HashList(dictToHash(vv) for vv in v)
            else:
                h[k] = v
        else:
            h[k] = v
    return h


def hashToDict(h):
    """Convert a nested Hash `h` into a `dict`

    Convert a Hash h into a dict. If h is nested, the result will be as well.

    Note: This is greedy as we lose all possible attributes of the Hash.
    """
    d = dict()
    for k, v in h.items():
        if isinstance(v, Hash):
            d[k] = hashToDict(v)
        elif isinstance(v, (list, tuple)):
            if len(v) > 0 and isinstance(v[0], Hash):
                # Note: This is a VectorHash
                d[k] = [hashToDict(vv) for vv in v]
            else:
                d[k] = v
        else:
            d[k] = v
    return d
