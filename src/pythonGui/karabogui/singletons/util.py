import re

from karabo.native.exceptions import KaraboError

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
            # Only KaraboErrors, show it!
            verbose_message = f"{message}"

    return verbose_message
