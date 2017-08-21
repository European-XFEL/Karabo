"""This module contains helper routines"""
import collections


def flatten(lis):
    """ Flattens a list that may contain sublists"""
    for e in lis:
        if (isinstance(e, collections.Iterable) and
                not isinstance(e, (str, bytes))):
            for ee in flatten(e):
                yield ee
        else:
            yield e
