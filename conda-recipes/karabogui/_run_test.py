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
from functools import reduce
from operator import or_

import pytest


def run(module, generate_coverage=False):
    opts = ["-v", "--disable-warnings", "--pyargs",
            f"--junitxml=junit.{module}.xml", module]
    if generate_coverage:
        opts.extend(["--cov-report", "term",
                     "--cov-report", "xml"])
    return pytest.main(opts)


def main():
    exit(reduce(or_,
                [run("karabo.common"),
                 run("karabo.native"),
                 run("karabogui", generate_coverage=True),
                 ]))


if __name__ == "__main__":
    main()
