import os
import subprocess
import sys


def start_bound_api_server(server_id, args, plugin_dir=''):
    """Start a Bound API device server in its own process
    """
    # set the plugin directory to directory of this file
    # the static .egg-info file located in the test directory
    # assures the the pkg_resources plugin loader will indentify
    # the test device as a valid plugin with an entry point
    if plugin_dir:
        env = os.environ.copy()
        env['PYTHONPATH'] = plugin_dir

    entrypoint = "from karabo.bound_api.device_server import main;main()"
    cmd = [sys.executable, "-c", entrypoint, "serverId=" + server_id] + args
    return subprocess.Popen(cmd, env=env)
