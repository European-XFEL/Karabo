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
import operator
import os
import subprocess
import sys
from platform import system
from urllib.parse import parse_qs, urlparse

GUI = "gui"
CINEMA = "cinema"
THEATRE = "theatre"

KARABO_GUI = "karabogui.programs.gui_runner"
KARABO_CINEMA = "karabogui.programs.cinema"
KARABO_THEATRE = "karabogui.programs.theatre"

KEEP_BLANK_VALUES = True

PYTHON = sys.executable


def parse_url(url):
    parsed = urlparse(url)
    return parsed.hostname, parse_qs(parsed.query,
                                     keep_blank_values=KEEP_BLANK_VALUES)


def get_cinema_args(queries):
    # Check required arguments:
    required = {"domain": lambda x: operator.eq(x, 1),
                "scene_uuid": lambda x: operator.ge(x, 1)}
    _check_args(queries, required)

    domain = queries["domain"][-1].upper()
    # NOTE: `scene_uuid` maps to the `scene_uuids` option in the
    # karabo-cinema executable. This is because the standard way of passing
    # a list of the same option in a url is `something?id=1&id=2`
    # to pass 1 and 2 to the id in the same query.
    scene_uuid = [uuid.lower() for uuid in queries["scene_uuid"]]
    args = [domain, *scene_uuid]

    # Check optional arguments:
    if queries.get("nosplash", False):
        args.append("--nosplash")
    # Check if host and port is both supplied
    if "host" in queries and "port" in queries:
        args.extend(["--host", queries["host"][-1],
                     "--port", queries["port"][-1]])
    return args


def get_theatre_args(queries):
    # Check required arguments:
    required = {"scene_id": lambda x: operator.ge(x, 1)}
    assert _check_args(queries, required)

    # NOTE: `scene_id` maps to the `scene_ids` option in the
    # karabo-theatre executable. This is because the standard way of passing
    # a list of the same option in a url is `something?id=1&id=2`
    # to pass 1 and 2 to the id in the same query.
    args = queries["scene_id"]

    # Check optional arguments:
    if queries.get("nosplash", False):
        args.append("--nosplash")
    # Check if host and port is both supplied
    if "host" in queries and "port" in queries:
        args.extend(["--host", queries["host"][-1],
                     "--port", queries["port"][-1]])
    return args


def _check_args(queries, required=None):
    if required is None:
        # Nothing to do here!
        return True

    invalid = []
    for arg, requirements in required.items():
        default = [''] if KEEP_BLANK_VALUES else None
        value = queries.get(arg, default)
        if value == default or not requirements(len(value)):
            invalid.append(arg)

    if invalid:
        raise ValueError(f"Invalid arguments found: {', '.join(invalid)}")

    return True


def main(url):
    # This is run from the application shortcut
    if not len(url):
        host, queries = None, ()
    else:
        # Assume that there's only one argument from the handler,
        # which is the URL
        assert len(url) == 1, "Invalid URL is supplied."
        url, = url
        host, queries = parse_url(url)

    if host is None or host == GUI:
        # Run GUI
        cmd = [PYTHON, "-m", KARABO_GUI]
    elif host == CINEMA:
        # Run Cinema
        cmd = [PYTHON, "-m", KARABO_CINEMA, *get_cinema_args(queries)]
    elif host == THEATRE:
        # Run Theatre
        cmd = [PYTHON, "-m", KARABO_THEATRE, *get_theatre_args(queries)]
    else:
        # Nothing to do here!
        return

    # Run command!
    if system() == "Windows":
        _run_detached_command_windows(cmd)
    else:
        subprocess.Popen(cmd, preexec_fn=os.setsid)


def _run_detached_command_windows(cmd):
    flags = 0
    flags |= 0x00000008  # DETACHED_PROCESS
    flags |= 0x00000200  # CREATE_NEW_PROCESS_GROUP
    flags |= 0x08000000  # CREATE_NO_WINDOW
    subprocess.Popen(cmd, creationflags=flags, close_fds=True)


if __name__ == "__main__":
    main(sys.argv[1:])
