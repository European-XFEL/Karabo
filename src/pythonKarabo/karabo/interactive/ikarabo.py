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
from IPython import start_ipython

SCRIPT = """
# This provides the namespace
from karabo.middlelayer.cli import *

# This initializes everything
from karabo.middlelayer import ikarabo
ikarabo.start_ikarabo()
del ikarabo
"""


def main():
    start_ipython(code_to_run=SCRIPT, force_interact=True,
                  display_banner=False)


if __name__ == '__main__':
    main()
