# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import html
import re

import natsort

from karabo.native import Hash, KaraboError

MESSAGE_TAG = "Message...........:"
PYTHON_TRACEBACK_HEADER = "Traceback (most recent call last):"
MESSAGE_PATTERN = (r"(?<=\n)\s*(?:{message_tag})\s*(.*?)(?=\n|$)"
                   .format(message_tag=MESSAGE_TAG))
MESSAGE_REGEX = re.compile(MESSAGE_PATTERN)
EXCEPTION_REGEX = re.compile(r"(?<=\n)(.*?(?:Exception|Error)):"
                             r"\s*(.*?)(?=\n|$)")
MAX_CHARS = 150


def get_error_message(failure_reason):
    """Reason is a detailed traceback of the failure. In C++ devices, it could
    have a message string. But in Python devices, it could just have an
    exception message. We try to parse it (by regex)
    """

    # 0. Return the whole message if it's short
    if len(failure_reason) <= MAX_CHARS:
        return failure_reason

    # 1. Check if there's a verbose error message from the C++ devices
    messages = MESSAGE_REGEX.findall(failure_reason)
    verbose_message = None
    # We are more interested on the inner error message than the first ones.
    for message in reversed(messages):
        if not message.startswith(PYTHON_TRACEBACK_HEADER):
            verbose_message = message.lstrip()
            break

    # We didn't succeed on getting a verbose message. Now we parse from the
    # exception messages.
    if verbose_message is None:
        python_exceptions = EXCEPTION_REGEX.findall(failure_reason)
        # We are more interested on the inner error message as this provides
        # the significant message.
        for exception, message in reversed(python_exceptions):
            if KaraboError.__name__ not in exception:
                verbose_message = f"{exception}: {message}"
                break
        else:
            if python_exceptions:
                # Only KaraboErrors, show it!
                verbose_message = f"{message}"
            else:
                # XXX: No exception found ...
                verbose_message = "Unknown exception"

    return html.escape(verbose_message)


def realign_topo_hash(topo_hash, attr):
    """Realign the topo hash `topo_hash` according to the attribute `attr`"""
    sorted_attrs = natsort.natsorted(
        [(k, v, a) for k, v, a in topo_hash.iterall()],
        key=lambda ele: ele[2].get(attr))
    ret = Hash()
    for k, v, a in sorted_attrs:
        ret.setElement(k, v, a)

    return ret
