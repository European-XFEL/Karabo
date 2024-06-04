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
"""Starting and stopping Karabo
============================

Karabo uses daemontools to supervise its device servers. Every device
server corresponds to one directory in `$KARABO/var/service`.
Typically, the device server's ID is used as a name for this
directory, just slashes replaced by underscores.

Within this directory, there is one `run` file, which is executed by
supervise. Typically, they are just symbolic links to files in
`$KARABO/service`, which contains ready-made code to execute Karabo
devices servers, but you are free to put whatever code you want.  If
you do use the ready-mades, a `parameters` file simply contains the
command line parameters to the server to be started.

There should be a `log` directory beside the `run` file, which should
again contain a `run` file which is executed for logging the standard
output from the device server. Again, there is a ready-made code in
`$KARABO/service/logger`, which you may link to.

Given that said set-up is actually pretty complicated, the
`karabo-add-deviceserver` script creates it for you. You just pass the
name of the device server, its type (middlelayerserver, cppserver or
pythonserver) and additional command line parameters, and all the
files and directories mentioned above are created.

Those device servers are then started with `karabo-start`. This is
just a small wrapper around daemontools' `svc` command, feel free to
use the underlying command as you may please. `karabo-stop` stops the
servers again, and `karabo-check` tells you which servers are
currently running and how they are doing.

If the standard logger described above is used, you may see the output
using `karabo-xterm`, which opens an xterm per started device server
to continuously show its output, or `karabo-gterm` which shows the
same as tabs in a gnome terminal.
"""

import os
import os.path as osp
import re
import shutil
import subprocess
import sys
from functools import wraps
from tempfile import mkdtemp
from textwrap import dedent
from time import sleep

COLORS = {
    "NORMAL": "\033[m",
    "RED": "\033[1;31m",
    "YELLOW": "\033[33m",
    "GREEN": "\033[0;32m",
}


def absolute(*path):
    return osp.join(os.environ["KARABO"], *path)


def entrypoint(func):
    @wraps(func)
    def invocation():
        if "KARABO" not in os.environ:
            print('Please activate Karabo '
                  '("source" the activate file in your Karabo directory)')
            return 1

        try:
            assert len(sys.argv) <= 1 or sys.argv[1] not in {"-h", "--help"}
            return func()
        except AssertionError:
            print(dedent("    " + func.__doc__))
            return 2
    return invocation


def supervise():
    if not check_service_dir():
        return False
    svok = subprocess.call([absolute("extern", "bin", "svok"),
                            absolute("var", "service", ".svscan")])
    if svok != 0:
        print("starting supervisor")
        supervise = subprocess.Popen(
            [absolute("extern", "bin", "supervise"),
             absolute("var", "service", ".svscan")],
            stdout=subprocess.PIPE)
        subprocess.Popen([absolute("extern", "bin", "multilog"),
                          absolute("var", "log", "svscan")],
                         stdin=supervise.stdout.fileno())
        sleep(1)  # give it some time to actually start
    return True


def defaultall():
    if not check_service_dir():
        return []
    existing = available_services()
    argv = [arg.replace("/", "_") for arg in sys.argv[1:]
            if not arg.startswith("-")]
    argv = [arg if arg in existing else arg.lower()
            for arg in argv]
    if not argv:
        return sorted(existing)
    else:
        return argv


def exec_defaultall(cmd, *args):
    if not supervise():
        return
    path = absolute("extern", "bin", cmd)
    os.execv(path, [cmd] + list(args) + defaultall())


def isexecutable(fn):
    return os.path.isfile(fn) and os.access(fn, os.X_OK)


def check_service_dir():
    service_dir = absolute("var", "service")
    if osp.exists(service_dir) and osp.isdir(service_dir):
        return True
    else:
        print("service directory not present."
              " Please create one with `karabo-create-services`")
        return False


def available_services():
    """Get all available services"""
    os.chdir(absolute("var", "service"))
    existing = {d for d in os.listdir() if not d.startswith(".")}
    return existing


@entrypoint
def make_service_dir():
    """karabo-create-services - creates Karabo servers folder

      karabo-create-services [-h|--help] template*

    creates a Karabo servers folder for a specific template.
    If none is given, creates a default template.

    Options are:
        default - A standard backbone installation. NB: this
                  configuration is likely targeted to the
                  infrastructure of the European XFEL
        empty   - An empty service folder.
                  Services can be added with the
                  `karabo-add-deviceserver` option
        jms_local
                - A stand-alone installation.
                  it will start the JMS broker on the local port
                  7777. To use this configuration, set the
                  KARABO_BROKER file in the var/environment folder
                  to 'tcp://localhost:7777
    """
    if osp.isdir(absolute("var", "service")):
        print("service directory already existing")
        return

    template = "default"
    if len(sys.argv[1:]) == 1:
        template = sys.argv[1]

    if not osp.isdir(absolute("service.in", template)):
        print(f"template service '{template}' directory not recognised.")
        dirs = [dir
                for dir in os.listdir(absolute("service.in"))
                if osp.isdir(absolute("service.in", dir))]
        print(f"Available templates are : {' ,'.join(dirs)}")
        return

    src = absolute("service.in", template)
    target = absolute("var", "service")
    shutil.copytree(src, target)


@entrypoint
def startkarabo():
    """karabo-start - start Karabo device servers

      karabo-start [-h|--help] device-servers*

    start given device servers. If none is given, start all.

    If the device server is already running, nothing happens.
    """
    exec_defaultall("svc", "-u")


@entrypoint
def stopkarabo():
    """karabo-stop - stop Karabo device servers

      karabo-stop [-h|--help] [-a|--all] [deviceserver1 [deviceserver2 [...]]]

    stops the given device servers.
    If the -a or --all flag is given, all device servers are stopped.
    Nothing happens for an already stopped device server.
    """
    assert len(sys.argv) > 1
    exec_defaultall("svc", "-d")


@entrypoint
def killkarabo():
    """karabo-kill - kill Karabo device servers

      karabo-kill [-h|--help] [-udopcaitkx ]

    send a signal to the listed device servers, by default to all.

    The signals are as follows:

    -i
      send an INT signal (as if Ctrl-C had been pressed)

    -t
      send a TERM signal (this should stop a server cleanly)

    -k
      send a KILL signal (kill the server for good)

    -p
      send a STOP signal

    -c
      send a CONT signal

    -a
      send an ALRM signal

    Several signals can be given on the command line. A "-l" signifies that
    not the device server, but its logger is meant. A + sign before a signal
    means (as in -+p) that the signal should be sent to the entire process
    group. This is implicit for the "-k" option to always kill all bound
    Python devices of a device server.

    There are other commands that can be given to a device server:

    -u
      Up. This is what karabo-start does.

    -d
      Down. This is what karabo-stop does.

    -o
      Start the device server once, but don't restart it if it stops.

    -x
      Kill the supervisor of the device server. Note that it will be restarted
      by svscan.

    If you want to kill all of karabo, use the following:

      karabo-kill -dx .svscan  # prevent supervisors from getting restarted
      karabo-kill -dx          # kill all device servers and their supervisors
      karabo-kill -l -dx       # kill all loggers and their supervisors

    This command does not try to start supervisors before running, so it is
    the only command usable to shut down Karabo completely.
    """
    assert len(sys.argv) > 1
    path = absolute("extern", "bin", "svc")
    args = [arg for arg in sys.argv if arg.startswith("-")]
    dirs = defaultall()
    if "-l" in args:
        args.remove("-l")
        dirs = [osp.join(d, "log") for d in dirs]

    # Add '+' option to '-k' arguments
    re1 = re.compile('-k+$')
    args = ['-+k' if re1.match(a) else a for a in args]

    # Split arguments containing 'k' and add '+' option to the latter
    re2 = re.compile('-[a-z]*k[a-z]*')
    for i in range(len(args)):
        if re2.match(args[i]) is not None:
            args[i] = args[i].replace('k', '')
            args.append('-+k')
    args = list(set(args))  # get rid of duplicates

    os.execv(path, ["svc"] + args + dirs)


def colorize(status_line):
    """
    Colorize the line of the following structure:
    `karabo_server: up (pid 26862) 448450 seconds, normally down, running`
    according to the up/down status
    """
    def color_string(color_name, line):
        return COLORS[color_name] + line + COLORS["NORMAL"]

    match = re.search(r"^(.+:)(\s+(\w+)\s+((?:\(.+\)\s)*)(\d+).+\s(.+))$",
                      status_line)
    if not match:
        return status_line
    server = match.group(1)
    rest = match.group(2)
    status = match.group(3)
    uptime = int(match.group(5))
    service_status = match.group(6)
    if status == "up" and service_status == "stopping":
        server = color_string("YELLOW", server)
    elif status == "up" and uptime > 1:
        server = color_string("GREEN", server)
    elif status == "up":
        server = color_string("YELLOW", server)
    elif status == "down":
        server = color_string("RED", server)
    else:
        server = COLORS["RED"] + server + COLORS["NORMAL"]
    return f"{server}{rest}"


@entrypoint
def checkkarabo():
    """karabo-check - check Karabo device servers

      karabo-check [-h|--help] device-servers*
    this shows the status of given Karabo device servers.

    If no device server is given, show the status of all device servers.
    """
    if not supervise():
        return
    svstat = subprocess.run(
        [absolute("extern", "bin", "svstat")] + defaultall(),
        stdout=subprocess.PIPE, encoding="utf8")
    print('\n'.join(map(colorize, svstat.stdout.strip().split('\n'))))


@entrypoint
def gnometermlog():
    """karabo-gterm - show logs in tabs of a gnome terminal

      karabo-gterm [-h|--help] device-server*

    This shows the logs of the given Karabo device servers in the
    tabs of a gnome terminal. If no device server is given, the
    logs of all device servers are shown.
    All this only works for device servers where logging is
    enabled.
    """
    if shutil.which("gnome-terminal") is None:
        print("'gnome-terminal' cannot be found - abort!")
        print("Try to use 'karabo-xterm' instead.")
        return 1
    use_lnav = False
    if shutil.which("lnav") is not None:
        use_lnav = True
    servers = [fn for fn in defaultall()
               if isexecutable(osp.join(fn, "log", "run"))]
    if use_lnav:
        cmd = sum((["--tab", "-e",
                    r'bash -c "echo -en \\\e]0\;{0}\\\a;'  # set tab title
                    'lnav $KARABO/var/log/{0}/current"'
                    .format(s)]
                   for s in servers), ["gnome-terminal"])
    else:
        cmd = sum((["--tab", "-e",
                    r'bash -c "echo -en \\\e]0\;{0}\\\a;'  # set tab title
                    'tail -n 100 -F $KARABO/var/log/{0}/current"'
                    .format(s)]
                   for s in servers), ["gnome-terminal"])
    os.execvp("gnome-terminal", cmd)


@entrypoint
def xtermlog():
    """karabo-xterm - show logs in an xterm

      karabo-xterm [-h|--help] device-server*

    This shows the logs of all given device servers in a xterm.
    This only works if logging is enabled for that device server.
    If no device server is given, the logs of all device servers
    are shown in their xterms.
    """
    if shutil.which("xterm") is None:
        print("'xterm' cannot be found - abort!")
        return 1
    use_lnav = False
    if shutil.which("lnav") is not None:
        use_lnav = True
    servers = [fn for fn in defaultall()
               if isexecutable(osp.join(fn, "log", "run"))]
    for server in servers:
        if use_lnav:
            subprocess.Popen(["xterm", "-T", server, "-e", "lnav",
                              absolute("var", "log", server, "current")])
        else:
            # Capital '-F' to follow renaming - 'current' is a rolling log file
            subprocess.Popen(["xterm", "-T", server, "-e", "tail", "-n", "100",
                              "-F", absolute("var", "log", server, "current")])


@entrypoint
def less():
    """karabo-less - show log in the current terminal

      karabo-less [-h|--help] device-server

    This shows the log of the given device server in the current terminal.
    This only works if logging is enabled for that device server.

    "lnav" will be used to display the log, "less --follow-name +F" otherwise.
    In the latter case, use "CTRL-C" to un-follow, "F" to follow again,
    "q" to quit.
    """
    servers = [fn for fn in defaultall()
               if isexecutable(osp.join(fn, "log", "run"))]

    if len(servers) != 1:
        print("Please pass a single valid device server to the command.")
        return

    fn = absolute("var", "log", servers[0], "current")
    if shutil.which("lnav") is not None:
        subprocess.call(["lnav", fn])
    else:
        cmd = ("less", "--follow-name", "+F", fn)  # follow file
        while True:
            try:
                subprocess.call(cmd)
                break  # "less" was quit normally
            except KeyboardInterrupt:
                # The user hit "CTRL-C" to unfollow the log. "less" will be
                # restarted without "+F" to allow qutting the loop.
                cmd = ("less", "--follow-name", "+G", fn)
        subprocess.call(["reset"])


server_template = """#!/bin/bash
# this file has been generated by karabo-add-deviceserver
cd $KARABO/var/data
exec envdir $KARABO/var/environment karabo-{server_type} serverId={server_id}\
 {options} 2>&1
"""

logger_template = """#!/bin/bash
# this file has been generated by karabo-add-deviceserver
exec multilog $KARABO/var/log/{target_dir}
"""


@entrypoint
def adddeviceserver():
    """karabo-add-deviceserver - create a new Karabo device server

      karabo-add-deviceserver [-h|--help]
      karabo-add-deviceserver name type arguments*

    This creates a new device server in $KARABO/var/service.

    name is the name of the new device server
    type is one of: cppserver, middlelayerserver, macroserver,
    pythonserver, webserver or webaggregatorserver.
    arguments are passed to said device server

    The serverId will be set to name.
    """
    assert len(sys.argv) > 2

    _, server_id, server_type, *options = sys.argv

    if not check_service_dir():
        return 3

    assert server_type in {
        "cppserver", "macroserver", "middlelayerserver", "pythonserver",
        "webaggregatorserver", "webserver"}

    target_dir = server_id.lower().replace("/", "_")

    abs_target = absolute("var", "service", target_dir)

    if osp.exists(abs_target):
        print(f"ERROR service/{target_dir} already exists")
        return 3
    # hide the new dir from svscan by prefixing with a . until we are done
    tmpdir = mkdtemp(dir=absolute("var", "service"), prefix=".tmp-")

    try:
        with open(osp.join(tmpdir, "run"), "w") as fout:
            fout.write(server_template.format(
                server_type=server_type, server_id=server_id,
                options="'{}'".format("' '".join(options)) if options else ""))
            # make file executable, but not writable
            os.chmod(fout.fileno(), 0o555)
        open(osp.join(tmpdir, "down"), "w").close()
        open(osp.join(tmpdir, "orphanage"), "w").close()
        with open(osp.join(tmpdir, "name"), "w") as fout:
            fout.write(server_id)
        os.mkdir(osp.join(tmpdir, "log"))
        with open(osp.join(tmpdir, "log", "run"), "w") as fout:
            fout.write(logger_template.format(target_dir=target_dir))
            os.chmod(fout.fileno(), 0o555)
        os.rename(tmpdir, abs_target)
    except Exception:
        shutil.rmtree(tmpdir)
        raise


@entrypoint
def removedeviceserver():
    """karabo-remove-deviceserver - remove a Karabo device server

      karabo-remove-deviceserver [-h|--help] name

    This removes a device server from $KARABO/var/service.

    name is the name of the device server to remove.
    """
    assert len(sys.argv) == 2
    if not check_service_dir():
        return 3

    name = sys.argv[1].replace("/", "_")
    # hide the service from svscan by prefixing with a . so it doesn't restart
    tmppath = absolute("var", "service", f".{name}")
    if osp.exists(absolute("var", "service", name)):
        os.rename(absolute("var", "service", name), tmppath)
    else:
        name = name.lower()
        os.rename(absolute("var", "service", name), tmppath)
    subprocess.call([absolute("extern", "bin", "svc"), "-dx", tmppath])
    subprocess.call([absolute("extern", "bin", "svc"), "-dx",
                     osp.join(tmppath, "log")])
    shutil.rmtree(tmppath)
