import re


def macro_sleep_check(code):
    """Check for the usage of time.sleep in a string.

    This is used to verify that macros don't use the blocking
    :func:`time.sleep`

    Importing the time module and using it otherwise is fine, e.g.
    :func:`time.perf_counter`. However, the complication is that there
    are a few ways to import time:

        from time import sleep
        import time; time.sleep
        import numpy; from time import perf_counter, sleep
        from time import perf_counter; from time import sleep
        exec("FROM TIME IMPORT SLEEP".lower())

    Yet, it's okay to mention time.sleep in comments, which users may well
    do:

        from time import perf_counter #, sleep
        from karabo.middlelayer import sleep  # CAS does not allow time.sleep

    So we check that the mention of time.sleep is not in a comment either.

    :param str code: the code
    :return list line_numbers: the lines where the import or the usage is done,
            or an empty list if not in the code.
    """
    # Match occurences of sleep preceeded, at some point, by a pound, ie,
    # commented out
    exp = re.compile("#(.*?)sleep")

    lines = code.splitlines()
    line_numbers = []

    for lineno, line in enumerate(lines, 1):
        line = line.lower()
        if "from time" in line or "time.sleep" in line:
            if not exp.search(line):
                line_numbers.append(lineno)

    return line_numbers
