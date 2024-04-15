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
import argparse
import re

FILE_HEADER = """/*
Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
*/

#include <sstream>

std::string getScene(const std::string& instanceId) {
    std::ostringstream ret;

    ret << ""
"""

FILE_FOOTER = """
        << "";
    return ret.str();
}
"""


def main():
    description = 'Convert a Karabo scene file to C++ code (scene.hh).'
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('filename',
                        help='An SVG file containing a Karabo scene')
    parser.add_argument('deviceId', nargs='?', default=None,
                        help='deviceId of the source device'
                             ' (optional argument)')
    ns = parser.parse_args()
    with open(ns.filename) as f:
        ls_lines = f.read()

    ls_lines = re.split("(/>)", ls_lines.replace('"', r'\"'))
    with open("scene.hh", 'w') as f:
        f.write(FILE_HEADER)
        for line in ls_lines:
            line = line.replace(str(ns.deviceId), '" << instanceId << "')
            f.write(f'        << "{line}"\n')
        f.write(FILE_FOOTER)


if __name__ == '__main__':
    main()
