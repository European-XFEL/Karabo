# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from functools import reduce
from operator import or_

import pytest


def run(module, generate_coverage=False):
    opts = ["-v", "--disable-warnings", "--pyargs", f"--junitxml=junit.{module}.xml", module]
    if generate_coverage:
        opts.extend([f"--cov={module}", "--cov-report", "term", "--cov-report", "xml"])
    return pytest.main(opts)


def main():
    exit(reduce(or_,
                [run("karabo.common"),
                run("karabo.native"),
                run("karabogui", generate_coverage=True),
                ]))

if __name__ == "__main__":
    main()
