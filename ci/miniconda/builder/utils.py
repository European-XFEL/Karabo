from functools import wraps
import os
import subprocess

from conda.cli.python_api import Commands, run_command
from conda.core.envs_manager import list_all_known_prefixes
from paramiko import AutoAddPolicy, SSHClient


def conda_run(command, *args, **kwargs):
    stdout, stderr, ret_code = run_command(command, *args, **kwargs)
    if ret_code != 0:
        msg = f'Command {command} [{args}] ' \
              f'{kwargs} returned {ret_code}\n{stderr}'
        raise RuntimeError(msg)
    return stdout


def command_run(cmd):
    try:
        return subprocess.check_output(cmd)
    except subprocess.CalledProcessError as e:
        print(f"Error in running command: {e.output}")
        raise e

def get_conda_prefix(env_name):
    prefix = None
    for path in list_all_known_prefixes():
        if os.path.basename(path) == env_name:
            prefix = path
    return prefix


def get_host_from_env():
    """Retrieve hostname, user and password from the environment"""
    msg = "Env. Variables MIRROR_USER, MIRROR_HOSTNAME and MIRROR_PWD"\
          " are required to upload to the upstream channel"
    host = os.environ.get('MIRROR_HOSTNAME', None)
    user = os.environ.get('MIRROR_USER', None)
    pw = os.environ.get('MIRROR_PWD', None)
    if any([v is None for v in (pw, user, host)]):
        raise RuntimeError(msg)
    return host, user, pw


def connected_to_remote(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        hostname, user, password = get_host_from_env()
        with SSHClient() as ssh_client:
            ssh_client.set_missing_host_key_policy(AutoAddPolicy())
            ssh_client.connect(hostname=hostname, username=user,
                               password=password)
            return func(*args, **kwargs, ssh=ssh_client)

    return wrapper


def mkdir(remote_path, sftp=None):
    try:
        sftp.stat(remote_path)
    except FileNotFoundError:
        print(f"Creating missing directory {remote_path}..")
        sftp.mkdir(remote_path)


def chdir(remote_path, sftp=None):
    try:
        sftp.stat(remote_path)
    except FileNotFoundError:
        print(f"Creating missing directory {remote_path}..")
        sftp.mkdir(remote_path)
    finally:
        sftp.chdir(remote_path)


def rmdir(remote_path, keep_dir=False, sftp=None):
    files = sftp.listdir(remote_path)

    for file in files:
        path = '/'.join([remote_path, file])
        try:
            sftp.remove(path)
        except IOError:
            # It is a directory, we do recursion
            rmdir(path)

    if not keep_dir:
        sftp.rmdir(remote_path)
