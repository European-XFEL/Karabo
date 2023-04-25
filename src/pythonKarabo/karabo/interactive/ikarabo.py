# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from IPython import start_ipython

SCRIPT = """
# This provides the namespace
from karabo.middlelayer.cli import *
from karabo.macros.cli import *

# This initializes everything
from karabo.middlelayer_api import ikarabo
ikarabo.start_ikarabo()
del ikarabo
"""


def main():
    start_ipython(code_to_run=SCRIPT, force_interact=True,
                  display_banner=False)


if __name__ == '__main__':
    main()
