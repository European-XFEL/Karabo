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

from functools import wraps
import os
import os.path as osp
import shutil
import subprocess
import sys
from tempfile import mkdtemp
from textwrap import dedent


def absolute(*path):
    return osp.join(os.environ["KARABO"], *path)


def entrypoint(func):
    @wraps(func)
    def invocation():
        if "KARABO" not in os.environ:
            print('Please activate Karabo '
                  '(type ". activate" in your Karabo directory)')
            return 1

        try:
            assert len(sys.argv) <= 1 or sys.argv[1] not in {"-h", "--help"}
            return func()
        except AssertionError:
            print(dedent("    " + func.__doc__))
            return 2
    return invocation


def supervise():
    svok = subprocess.call([absolute("extern", "bin", "svok"),
                            absolute("service")])
    if svok == 0:
        return
    print("starting supervisor")
    subprocess.Popen([absolute("extern", "bin", "supervise"),
                      absolute("service")],
                     stdout=open(absolute("var", "log", "global"), "a"))


def defaultall():
    os.chdir(absolute("var", "service"))
    if len(sys.argv) > 1:
        return sys.argv[1:]
    else:
        return os.listdir()


def exec_defaultall(cmd, *args):
    supervise()
    path = absolute("extern", "bin", cmd)
    os.execv(path, [cmd] + list(args) + defaultall())


def isexecutable(fn):
    return os.path.isfile(fn) and os.access(fn, os.X_OK)


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

      karabo-stop [-h|--help] device-server*

    stops the given device servers. If no device server is given, all
    device servers are stopped. Nothing happens for an already stopped
    device server.
    """
    exec_defaultall("svc", "-d")


@entrypoint
def checkkarabo():
    """karabo-check - check Karabo device servers

      karabo-check [-h|--help] device-servers*

    this shows the status of given Karabo device servers.

    If no device server is given, show the status of all device servers.
    """
    exec_defaultall("svstat")


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
    servers = [fn for fn in defaultall()
               if isexecutable(osp.join(fn, "log", "run"))]
    cmd = sum((["--tab", "-e",
                r'bash -c "echo -en \\\e]0\;{0}\\\a;'  # set tab title
                          'tail -n 100 -F {0}/log/current"'.format(s)]
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
    servers = [fn for fn in defaultall()
               if isexecutable(osp.join(fn, "log", "run"))]
    for server in servers:
        subprocess.Popen(["xterm", "-T", server, "-e", "tail", "-f",
                          "{}/log/current".format(server)])


@entrypoint
def adddeviceserver():
    """karabo-add-deviceserver - create a new Karabo device server

      karabo-add-deviceserver [-h|--help]
      karabo-add-deviceserver name type arguments*

    This creates a new device server in $KARABO/var/service.

    name is the name of the new device server
    type is one of: cppserver, middlelayerserver or pythonserver
    arguments are passed to said device server

    The serverId will be set to name.
    """
    assert len(sys.argv) > 2

    _, server_id, server_type, *options = sys.argv
    target_dir = server_id.replace("/", "_")
    abs_target = absolute("var", "service", target_dir)

    if osp.exists(abs_target):
        print("ERROR service/{} already exists".format(target_dir))
        return 3
    if not osp.exists(absolute("service", server_type)):
        print("ERROR server type {} is not known".format(server_type))
        return 4
    tmpdir = mkdtemp(dir=absolute("var"))
    try:
        os.symlink("../../../service/{}".format(server_type),
                   osp.join(tmpdir, "run"))
        open(osp.join(tmpdir, "down"), "w").close()
        with open(osp.join(tmpdir, "parameters"), "w") as params:
            print("serverId={}".format(server_id), file=params)
            for arg in options:
                print(arg, file=params)
        os.mkdir(osp.join(tmpdir, "log"))
        os.symlink("../../../../service/logger",
                   osp.join(tmpdir, "log", "run"))
        os.rename(tmpdir, abs_target)
    except:
        shutil.rmtree(tmpdir)
        raise
