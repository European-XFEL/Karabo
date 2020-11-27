import argparse
from contextlib import contextmanager
from distutils.version import LooseVersion
import os
import os.path as op
import pkg_resources
from setuptools_scm import get_version
import shutil
import tempfile

from conda.cli.python_api import Commands
from conda_pack import CondaEnv, File
from conda_pack.compat import on_win

from .build import Builder, append_build_args
from .compare import execute as compare_env
from .utils import (
    command_run, conda_run, connected_to_remote, get_conda_prefix, mkdir, rmdir)


COMPRESSED_EXTENSIONS = {
    "win-64": "windows.zip",
    "osx-64": "macosx.tar.gz",
    "linux-64": "linux.tar.gz"
}

KARABOGUI = "karabogui"
TEST_ENVIRONMENT = "karabogui-test"
ENVIRONMENT_YML = "environment.yml"

REMOTE_KARABO_DIR = "/var/www/html/karabo"

PACKING_SUPPORTED = (KARABOGUI, )
INCLUDED_SCRIPT = op.join(op.dirname(__file__), "..", "karabo-update.py")


class Packer(Builder):
    """Main Packing Manager for the Karabo Control System

    Will take an argument recipe in input and will optionally clean, test, and
    upload the environment and the recipe to the desired destionation.
    """

    def __init__(self, args):
        super(Packer, self).__init__(args)
        self.version = get_version(self.root_path)
        self.remote_root_path = '/'.join([REMOTE_KARABO_DIR, args.remote_env_dir])
        self.remote_build_path = '/'.join([self.remote_root_path, self.version])
        self.remote_os_path = '/'.join([self.remote_build_path, self.platform])

    # -----------------------------------------------------------------------
    # Reimplemented methods

    def run(self):
        super(Packer, self).run()
        if not self.args.upload_env:
            return

        for recipe in self.recipes:
            if recipe not in PACKING_SUPPORTED:
                continue
            # Check if there's an existing devenv environment
            if get_conda_prefix(recipe) is None:
                self.create_devenv(recipe)
            self.upload_environment(env=recipe)

    def clean(self):
        super(Packer, self).clean()
        for entry in os.scandir(self.root_path):
            if TEST_ENVIRONMENT in entry.name:
                print(f"Deleting existing directory: {entry.name}")
                shutil.rmtree(entry.path)

    def test(self, recipe):
        print("Downloading the latest environment for testing.")
        packed_env = self.download_environment()
        if packed_env is None:
            # We fail to download the latest packed environment. We use the
            # super implementation of the test()
            print("Latest environment is not found. Will proceed testing by "
                  "creating a devenv.")
            return super(Packer, self).test(recipe)

        # Extract environment
        extracted_env_path = op.join(self.root_path, TEST_ENVIRONMENT)
        shutil.unpack_archive(packed_env, extracted_env_path)

        # Install Karabo codes
        cwd = os.getcwd()
        bin_path = '' if on_win else 'bin'
        python_path = op.join(extracted_env_path, bin_path, 'python')
        for source in ('pythonKarabo', 'pythonGui'):
            # setup.py paths
            print(f"Installing {source}..")
            os.chdir(op.join(self.root_path, 'src', source))
            command_run([python_path, '-m',
                         'pip', 'install', '-e', '.', '--no-deps'])
        os.chdir(cwd)

        # Unpack the environment to resolve variables
        scripts_path = 'bin'
        if on_win:
            scripts_path = 'Scripts'
            command_run([op.join(extracted_env_path, scripts_path, 'activate.bat')])
        command_run([op.join(extracted_env_path, scripts_path, 'conda-unpack')])

        # Set the Qt platform plugin variable prior to test
        if self.platform == 'win-64':
            plugin_path = op.join(extracted_env_path, "Library", "plugins", "platforms")
            os.environ["QT_QPA_PLATFORM_PLUGIN_PATH"] = plugin_path

        # Test environment
        errors = []
        for module in ['karabogui', 'karabo.native', 'karabo.common']:
            try:
                cmd = [python_path, '-m', 'pytest', '-v', '--pyargs'
                   f'--junitxml=junit.{module}.xml', module]
                command_run(cmd)
            except RuntimeError as e:
                errors.append(f'{module}:\n{str(e)}')

        if errors:
            message = '\n\n'.join(errors)
            raise RuntimeError(f"Failures in the following modules: "
                               f"\n{message}")

        print('Tests successful')

        # Unset the Qt platform plugin variable after the test
        os.environ.pop("QT_QPA_PLATFORM_PLUGIN_PATH", None)

    # -----------------------------------------------------------------------
    # Packing-specific methods

    @connected_to_remote
    def download_environment(self, ssh):
        # TODO: compare environments properly
        return None
        with ssh.open_sftp() as sftp:
            latest_env = self.get_latest_env(sftp)
            if latest_env is None:
                return None

            print(f"The latest environment is found to be: {latest_env}")
            latest_filename = get_filename(name=KARABOGUI,
                                           version=latest_env,
                                           platform=self.platform)
            remote_packed_env = self._get_remote_path(latest_env, latest_filename)
            return self.download(remote_packed_env, sftp)

    @connected_to_remote
    def upload_environment(self, env, ssh):
        with ssh.open_sftp() as sftp:
            mkdir(self.remote_root_path, sftp)
            mkdir(self.remote_build_path, sftp)
            mkdir(self.remote_os_path, sftp)

            try:
                self.upload_env_pack(env, sftp)
                self.upload_env_file(env, sftp)
            except:
                # If any exceptions encountered and packing is unsuccessful,
                # delete the platform path.
                rmdir(self.remote_os_path, sftp)

    def get_latest_env(self, sftp):
        try:
            versions = sftp.listdir(self.remote_root_path)
            envs = sorted(versions, key=LooseVersion)
        except FileNotFoundError:
            print(f"The directory {self.remote_root_path} is not found.")
            return None

        # Return the latest environment, which is on the last list
        for env in reversed(envs):
            # If the environment is already existing (in case for rebuilding),
            # We delete the .
            if env == self.version:
                print(f"An environment for {env} already exists. "
                      f"We repack a new environment.")
                rmdir(self.remote_os_path, keep_dir=True, sftp=sftp)
                return None
            env_path = '/'.join([self.remote_root_path, env])
            # Check if there is an environment for the platform
            if self.platform in sftp.listdir(env_path):
                return env

    def download(self, remote_path, sftp):
        try:
            local_path = op.join(self.root_path, op.basename(remote_path))
            sftp.get(remote_path, local_path)
            return local_path
        except FileNotFoundError:
            raise RuntimeError(f"The file {remote_path} is not found.")

    def upload_env_pack(self, env, sftp):
        # Check if upload is needed if the latest remote env is different from
        # the current environment
        upload_needed = True
        latest_env = self.get_latest_env(sftp)
        if latest_env:
            print(f"The latest environment is found to be: {latest_env}")
            remote_env_file = self._get_remote_path(latest_env, ENVIRONMENT_YML)
            local_env_file = self.download(remote_env_file, sftp)
            exitcode, output = compare_env(env, local_env_file)
            upload_needed = exitcode != 0
            print(output)

        # Create symlink if upload is not needed.
        if not upload_needed and latest_env:
            if self.create_env_symlink(latest_env, sftp):
                # If it is successful, bail out. If not, we proceed packing
                # The environment
                return

        # Packing the environment
        pack_filename = get_filename(name=KARABOGUI, version=self.version,
                                     platform=self.platform)
        self.pack_environment(env, filename=pack_filename)

        # Uploading the packed environment
        remote_pack_path = '/'.join([self.remote_os_path, pack_filename])
        print(f"Uploading the packed environment to {remote_pack_path}..")
        sftp.put(pack_filename, remote_pack_path)

    def pack_environment(self, env, filename):
        # Pack and upload the environment
        print(f"Packing the {env} environment..")
        env = CondaEnv.from_name(env)
        # Include our script here
        with added_script(filename=INCLUDED_SCRIPT, environment=env):
            env.pack(op.join(filename))

    def create_env_symlink(self, latest_env, sftp):
        """Creates a symlink of the current environment version to the
           ``latest_env`` packed environment to avoid redundant uploads."""
        pack_filename = get_filename(name=KARABOGUI, version=self.version,
                                     platform=self.platform)
        remote_pack_path = '/'.join([self.remote_os_path, pack_filename])

        latest_filename = get_filename(name=KARABOGUI,
                                       version=latest_env,
                                       platform=self.platform)
        latest_pack_path = self._get_remote_path(latest_env, latest_filename)
        print(f"Creating symlink: source={latest_pack_path}, "
              f"dest={remote_pack_path}")

        # Check if packed enviroment is available in remote
        try:
            sftp.stat(latest_pack_path)
            sftp.symlink(latest_pack_path, remote_pack_path)
            success = True
        except FileNotFoundError:
            msg = (f"The latest packed environment {latest_pack_path} is "
                   f"not found.. Uploading a new packed environment "
                   f"instead.")
            print(msg)
            success = False

        return success

    def upload_env_file(self, env, sftp):
        # Check if the file is already existing, there's no need to download it
        # again
        remote_env_file = '/'.join([self.remote_os_path, ENVIRONMENT_YML])
        try:
            sftp.stat(remote_env_file)
        except FileNotFoundError:
            print(f"Uploading {remote_env_file}..")
            # Upload current environment file
            env_file = op.join(self.root_path, ENVIRONMENT_YML)
            conda_run(Commands.RUN, '-n', env,
                      'conda', 'env', 'export', '--no-builds', '>', env_file)
            sftp.put(env_file, remote_env_file)
        else:
            print(f"The environment file {remote_env_file} already exists.")

    def _get_remote_path(self, version, filename):
        return '/'.join([self.remote_root_path, version,
                         self.platform, filename])


PYTHON_SHEBANG = '#!python.exe' if on_win else '#!/usr/bin/env python'
BIN_DIR = 'Scripts' if on_win else 'bin'


@contextmanager
def added_script(filename, environment, executable=True):
    # Retrieve the script contents and append the shebang
    with open(filename) as script_file:
        script = script_file.read()
    complete_script = f"{PYTHON_SHEBANG}\n{script}"
    basename, _ = op.splitext(op.basename(filename))

    # Create a temp file as the modified file and make it executable
    file = tempfile.NamedTemporaryFile(mode='w', delete=False)
    try:
        file.write(complete_script)
        file.close()
        stat = os.stat(file.name)
        if executable:
            os.chmod(file.name, stat.st_mode | 0o111)  # make executable
        script_name = basename + ('-script.py' if on_win and executable else '')
        script_py = File(source=file.name,
                         target=op.join(BIN_DIR, script_name))
        environment.files.append(script_py)

        # Add the executable application of the script on Windows
        if on_win:
            cli_exe = pkg_resources.resource_filename("setuptools", "cli-64.exe")
            script_exe = File(source=cli_exe,
                              target=op.join(BIN_DIR, f"{basename}.exe"))
            environment.files.append(script_exe)
        yield
    finally:
        os.unlink(file.name)


def get_filename(name, version, platform):
    return f"{name}-{version}-{COMPRESSED_EXTENSIONS[platform]}"


def get_pack_args(sub_parser=None, description=None):
    if sub_parser is not None:
        # append this module as a sub parser of `parser`
        parser = sub_parser.add_parser('pack',
                                       help=description)
        parser.set_defaults(klass=Packer)
    else:
        parser = argparse.ArgumentParser(description=description)

    parser = append_build_args(parser)

    parser.add_argument(
        '-K', '--upload-env', action='store_true',
        help='Upload the environment on remote host')

    parser.add_argument(
        '-E', '--remote-env-dir', type=str,
        default='karaboEnvironments',
        help='Directory of the packed environments on remote server')

    return parser


def main(args):
    b = Packer(args)
    b.run()


DESCRIPTION = """
Karabo Environment Packing Manager

This script will manage the running of tests and uploading of the packed conda
environment and built recipe. 

This can be used in conjuction with the build script.
"""

if __name__ == '__main__':
    root_ap = get_pack_args(description=DESCRIPTION)
    args = root_ap.parse_args()
    main(args)
