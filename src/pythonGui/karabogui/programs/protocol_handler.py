import operator
import os
from platform import system
import subprocess
import sys
from urllib.parse import parse_qs, urlparse

GUI = "gui"
CINEMA = "cinema"

KARABO_GUI = "karabogui.programs.gui_runner"
KARABO_CINEMA = "karabogui.programs.cinema"

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
    scene_uuid = [uuid.lower() for uuid in queries["scene_uuid"]]
    args = [domain, *scene_uuid]

    # Check optional arguments:
    if queries.get("nosplash", False):
        args.append("--nosplash")
    if "username" in queries:
        args.extend(["--username", queries["username"][-1]])
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
