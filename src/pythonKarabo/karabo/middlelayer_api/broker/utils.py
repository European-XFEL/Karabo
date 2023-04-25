# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import re

_BROKER_SCHEMES = ("tcp", "amqp", "mqtt")
_SCHEME_DELIMITER = "://"
_AMQP_REGEX = re.compile(
    r"^(amqp:\/\/)([A-Za-z0-9]+)(\:)+([A-Za-z0-9]+)"
    r"(\@)+([A-Za-z0-9\-]+)(\:)+([0-9]+)")


def all_equal(iterable):
    """Check if all elements of an iterable are equal"""
    iterator = iter(iterable)
    try:
        first = next(iterator)
    except StopIteration:
        return True
    return all(first == x for x in iterator)


def all_match(iterable, regex):
    """Check if all iterables match the regex"""
    iterator = iter(iterable)
    return all(regex.match(x) for x in iterator)


def check_broker_scheme(urls: list):
    """Check the list of broker hosts

    :param hosts: The list of broker hosts

    This function will raise a `RuntimeError` if the provided list
    contains invalid broker host data.
    """
    if not len(urls):
        raise RuntimeError(
            "The list of broker hosts does contain any url.")

    schemes = [url.split(_SCHEME_DELIMITER)[0] for url in urls]
    if not len(schemes) or not all([scheme in _BROKER_SCHEMES
                                    for scheme in schemes]):
        raise RuntimeError(
            "The list of broker hosts does provide unknown schemes.")

    if not all_equal(schemes):
        raise RuntimeError(
            "The list of broker hosts contains different schemes.")

    scheme = schemes[0]
    if scheme == "amqp":
        if not all_match(urls, _AMQP_REGEX):
            raise RuntimeError(
                "Incomplete broker schemes for amqp provided. It must be "
                "assembled like `amqp://user:password@host:port`.")
