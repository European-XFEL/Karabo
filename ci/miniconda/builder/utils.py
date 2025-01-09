import os
import subprocess
from functools import wraps

from paramiko import AutoAddPolicy, SSHClient


def conda_run_command(cmd: list, *, env_name: str = None):
    """Run the 'conda run' command as subprocess on the provided environment"""
    conda_command = ["conda", "run"]
    if env_name is not None:
        conda_command.extend(["-n", env_name])
    command = conda_command + cmd
    try:
        result = subprocess.run(command, capture_output=True, text=True,
                                check=True)
        print(f"Executed in the '{env_name}'  environment: {cmd}")
        print(result.stdout)
        return result.stdout
    except subprocess.CalledProcessError as err:
        print("\n---------------------------------------------------")
        print(f"Failed to execute in the '{env_name}' environment: {cmd}")
        print(f"ERROR: {err.stderr}\n")
        print(err.stdout)
        raise


def command_run(cmd) -> str:
    """Run the command and capture the output, errors, and decode
    it to string"""

    try:
        output = subprocess.check_output(cmd)
        return output.decode("utf-8")
    except subprocess.CalledProcessError as e:
        # Print the error message and raise the exception again
        print(f"Error in running command: {e.output}")
        raise e


def environment_exists(env_name: str) -> bool:
    """Check if the environment already exists"""
    result = command_run(["conda","env", "list"])
    return any(
        env_name in line.split()[0] for line in result.splitlines() if
        line.strip())

def get_host_from_env():
    """Retrieve hostname, user and password from the environment"""
    msg = (
        "Env. Variables MIRROR_USER, MIRROR_HOSTNAME and MIRROR_PWD"
        " are required to upload to the upstream channel"
    )
    host = os.environ.get("MIRROR_HOSTNAME", None)
    user = os.environ.get("MIRROR_USER", None)
    pw = os.environ.get("MIRROR_PWD", None)
    if any([v is None for v in (pw, user, host)]):
        raise RuntimeError(msg)
    return host, user, pw


def connected_to_remote(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        hostname, user, password = get_host_from_env()
        with SSHClient() as ssh_client:
            ssh_client.set_missing_host_key_policy(AutoAddPolicy())
            ssh_client.connect(
                hostname=hostname, username=user, password=password
            )
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
        path = "/".join([remote_path, file])
        try:
            sftp.remove(path)
        except IOError:
            # It is a directory, we do recursion
            rmdir(path)

    if not keep_dir:
        sftp.rmdir(remote_path)
