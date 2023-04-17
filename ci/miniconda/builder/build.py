import argparse
from contextlib import contextmanager
import os
import os.path as op
from platform import system as sys_name
import shutil
from tempfile import gettempdir

from conda.cli.python_api import Commands
from conda import CondaMultiError
import yaml

from .mirrors import Mirrors
from .utils import (
    chdir, command_run, conda_run, connected_to_remote, get_conda_prefix,
    mkdir)

PLATFORMS = {
    "Windows": "win-64",
    "Darwin": "osx-64",
    "Linux": "linux-64"
}

KARABOGUI = "karabogui"

XDG_RUNTIME_DIR = "/tmp/runtime-root"
XVFB_ARGS = "-screen 0 1280x1024x24"
XVFB_DISPLAY = ":0"
XAUTHORITY_PATH = op.join(XDG_RUNTIME_DIR, ".Xauthority")


class Builder:

    def __init__(self, args):
        self.args = args

        # Local paths
        r = command_run(['git', 'rev-parse', '--show-toplevel'])
        self.root_path = op.normpath(r.decode().strip())
        self.mirror_dir = op.join(gettempdir(), 'mirror')

        # Platform
        self.platform = ''
        platform = PLATFORMS.get(sys_name())
        if platform is None:
            raise RuntimeError(f'Unsupported platform {sys_name()}')
        self.platform = platform

        # Recipe
        self.recipes = set()

        # Conda setup
        conda_run(Commands.RUN, '-n', 'base',
                  'conda', 'config', '--set', 'pip_interop_enabled', 'True')

    # -----------------------------------------------------------------------
    # Main logic

    def run(self):
        actions = {
            self.args.clean,
            self.args.index_mirror,
            self.args.test,
            self.args.upload_to_mirror,
            not self.args.skip_build
        }
        if all([not action for action in actions]):
            print("No action requested")

        for recipe in self.args.recipes.split(','):
            if recipe:
                self.recipes.add(recipe)

        # A wrongly triggered nightly build
        if (self.args.nightly and
                os.environ.get("SCHEDULED_JOB", "") != "nightly-build"):
            print("Not a nightly-build scheduled job!")
            return

        if len(self.recipes) == 0:
            print("Recipe list empty. Nothing to do!")
            return

        if self.args.clean:
            self.clean()
        if self.args.ci:
            self.adapt_platform()


        for recipe in self.recipes:
            # Proceed only if recipe supports testing
            if recipe == KARABOGUI and self.args.test:
                self.test(recipe)
            if not self.args.skip_build:
                self.build(recipe)

        if self.args.upload_to_mirror:
            self.prepare_mirror_dir()
            for recipe in self.recipes:
                self.create_mirror(recipe)
            self.upload()

        if self.args.index_mirror:
            self.index()

    # -----------------------------------------------------------------------
    # Properties

    @property
    def mirror_channel(self):
        return f'http://{self.args.channel}/karabo/channel'

    @property
    def mirror_conda_forge(self):
        return f'http://{self.args.channel}/karabo/channel/mirror/conda-forge'

    @staticmethod
    def get_env_name(recipe):
        return f"build_env_{recipe}"

    def recipe_path(self, recipe):
        return op.join(self.root_path, self.args.recipes_dir, recipe,
                       'meta.yaml')

    def devenv_path(self, recipe):
        return op.join(self.root_path, self.args.recipes_dir, recipe,
                       'environment.devenv.yml')

    def recipe_base_path(self, recipe):
        return op.join(self.root_path, self.args.recipes_dir, recipe,
                       'meta_base.yaml')

    # -----------------------------------------------------------------------
    # Setting

    def adapt_platform(self):
        """Performs platform specific tasks per recipe"""
        if self.platform == 'osx-64':
            os.environ['LANG'] = 'en_US.UTF-8'
        elif self.platform == 'linux-64':
            # Setup XVFB
            os.environ['DISPLAY'] = XVFB_DISPLAY
            os.environ['XDG_RUNTIME_DIR'] = XDG_RUNTIME_DIR
            os.environ['XAUTHORITY'] = XAUTHORITY_PATH
            command_run([
                'start-stop-daemon', '--start', '-b', '-x', '/usr/bin/Xvfb',
                '--', XVFB_DISPLAY, '-screen', '0', '1024x768x24', '-extension', 'GLX'])

    def clean(self):
        print("Cleaning conda..")
        conda_run(
            Commands.RUN, '-n', 'base',
            'conda', 'build', 'purge-all', '--quiet')
        conda_run(
            Commands.RUN, '-n', 'base',
            'conda', 'clean', '--all', '--quiet', '--yes'
        )
        for recipe in self.recipes:
            self.clean_environment(name=recipe)

    def clean_environment(self, *, name):
        """Cleans the environment of a package and purges old build dirs"""
        print("Cleaning", name)

        try:
            # Remove the environment for the recipe
            conda_run(Commands.REMOVE, '-n', name, '--all', '--quiet', '--yes')
        except RuntimeError:
            # this might fail if the environment does not exist
            print(f"Tried removing `{name}` environment, "
                  f"but it does not exist")
        except CondaMultiError as e:
            # this might fail if there are files that have been removed before
            print(f"There are errors with {name}:")
            print(e)
            print(f"Will proceed as intended.")

    # -----------------------------------------------------------------------
    # Installing and testing Karabo

    def install_karabo(self, recipe):
        # setup.py paths
        py_karabo_root = op.join(
            self.root_path, 'src', 'pythonKarabo')
        gui_root = op.join(
            self.root_path, 'src', 'pythonGui')

        # Install pythonKarabo in the `recipe` environment
        # FIXME: install native without environment variable
        os.environ['BUILD_KARABO_SUBMODULE'] = 'NATIVE'
        conda_run(
            Commands.RUN, '-n', recipe, '--cwd', py_karabo_root,
            'python', 'setup.py', 'install')
        os.environ.pop('BUILD_KARABO_SUBMODULE')
        # Install pythonGui in the `recipe` environment
        conda_run(
            Commands.RUN, '-n', recipe, '--cwd', gui_root,
            'python', 'setup.py', 'install')

    def uninstall_karabo(self, recipe):
        # no need to uninstall if we are not building an env pack
        if self.args.skip_build:
            return
        # Prepare env pack by uninstalling karabo and karabogui
        for excluded_pkgs in ["karabo", "karabogui"]:
            conda_run(Commands.RUN, '-n', recipe,
                      'pip', 'uninstall', excluded_pkgs, '--yes')

    @contextmanager
    def karabo_installed(self, recipe):
        try:
            self.install_karabo(recipe)
            yield
        finally:
            self.uninstall_karabo(recipe)

    def test(self, recipe):
        # Proceed only if recipe supports testing
        if not (recipe == KARABOGUI and self.args.test):
            return
        recipe_dir = op.dirname(self.recipe_path(recipe))
        test_script = op.join(recipe_dir, f"_run_test.py")
        if not op.exists(test_script):
            print(f"Test script '{test_script}' missing")

        # Check if there's an existing devenv environment
        if get_conda_prefix(recipe) is None:
            self.create_devenv(recipe)

        with self.karabo_installed(recipe):
            cmd = [Commands.LIST]
            output = conda_run(*cmd)
            print(output)
            cmd = [
                Commands.RUN, '-n', recipe, 'python',
                test_script]
            output = conda_run(*cmd)
            print(output)
            print('Tests successful')

    # -----------------------------------------------------------------------
    # Building recipe

    def prepare_build(self, recipe):
        # Build settings
        osx_sysroot = os.environ.get('OSX_SYSROOT')
        if osx_sysroot:
            conf = {'CONDA_BUILD_SYSROOT': [osx_sysroot]}
            conf_path = op.join(
                self.root_path, 'recipes',
                recipe, 'conda_build_config.yaml')
            with open(conf_path, 'w') as f:
                f.write(yaml.dump(conf))

    def build(self, recipe):
        """"Builds a recipe"""
        self.prepare_build(recipe)

        if all([op.exists(self.recipe_base_path(recipe)),
                op.exists(self.devenv_path(recipe))]):
            self.create_devenv(recipe)
            self.build_recipe_from_base(recipe)

        if op.exists(self.recipe_path(recipe)):
            self.build_recipe(recipe)

    def create_devenv(self, recipe):
        # Clean again just safety
        self.clean_environment(name=recipe)
        os.environ['XFEL_CONDA_CHANNEL'] = self.args.channel
        conda_run(
            Commands.RUN, '-n', 'base',
            'conda', 'devenv', '--file', self.devenv_path(recipe))

    def build_recipe_from_base(self, recipe):
        # build the meta.yaml file from the meta_base file
        output_file = self.recipe_path(recipe)
        conda_run(
            Commands.RUN, '-n', 'base',
            'python', '-m', 'cogapp', '-o', output_file,
            self.recipe_base_path(recipe))
        # Print the recipe
        print(f'Building Recipe for "{recipe}":')
        with open(output_file) as recipe_file:
            for line in recipe_file:
                print(line, end='')

    def build_recipe(self, recipe):
        """"Builds a recipe from meta.yaml file"""
        print(f'Building recipe for {recipe}')
        recipe_dir = op.dirname(self.recipe_path(recipe))
        conda_run(
            Commands.RUN,
            '-n', 'base',
            'conda', 'build', recipe_dir,
            '-c', 'local',
            '-c', self.mirror_channel,
            '-c', self.mirror_conda_forge,
            '-c', 'conda-forge',
            '-c', 'defaults',
            '--override-channels',
            '--no-anaconda-upload',
            '--quiet')
        print("Recipe build successful")

    def install_build(self, recipe):
        """Installs a local package into a 'build' environmemnt"""
        if not self.args.upload_to_mirror:
            return
        # Create or use the existing environment
        env = self.get_env_name(recipe)
        if get_conda_prefix(env) is None:
            print(f'Create environment {env}')
            conda_run(Commands.CREATE, '-n', env)
        # locate the package
        print(f'install local package for "{recipe}" in env "{env}"')
        conda_run(
            Commands.RUN, '-n', env,
            'conda', 'install', recipe, '-y', '--quiet',
            '-c', 'local',
            '-c', self.mirror_channel,
            '-c', self.mirror_conda_forge,
            '-c', 'conda-forge',
            '-c', 'defaults')
        return env

    # -----------------------------------------------------------------------
    # Mirroring

    def prepare_mirror_dir(self):
        if self.args.upload_to_mirror:
            # clean the mirror directory
            if op.isdir(self.mirror_dir):
                shutil.rmtree(self.mirror_dir)
            os.makedirs(self.mirror_dir)

    def create_mirror(self, recipe):
        env = self.install_build(recipe)
        print(f"Identifying packages to mirror for {env}")
        mirrors = Mirrors(env, self.mirror_channel)
        mirrors.populate(self.mirror_dir)

    # -----------------------------------------------------------------------
    # Uploading

    @connected_to_remote
    def upload(self, ssh=None):
        with ssh.open_sftp() as sftp:
            self.upload_mirrors(sftp)
            # upload the recipes for this platform
            self.upload_recipes(sftp)

    def upload_mirrors(self, sftp):
        """Transfers mirror files using the sftp client"""
        print("Uploading mirror packages")
        chdir(self.args.remote_mirror_dir, sftp)
        for dir_, subdirs, files in os.walk(self.mirror_dir):
            target_dir = op.relpath(dir_, self.mirror_dir)
            # the remote server will have a unix-like path.
            # this call converts windows directories structure
            # and is a nop in unix like systems.
            dir_parts = [self.args.remote_mirror_dir]
            dir_parts.extend(op.split(target_dir))
            remote_dir = "/".join(dir_parts)
            mkdir(remote_dir, sftp)

            for filename in files:
                # the remote server will have a unix path.
                # running a `op.join` on windows will mangle the paths
                remote_file_path = '/'.join([remote_dir, filename])
                local_file_path = op.join(dir_, filename)
                print(f'Uploading {local_file_path} to {remote_file_path}')
                sftp.put(local_file_path, remote_file_path)

    def upload_recipes(self, sftp):
        """Transfers packages using the sftp client"""
        print("uploading packages")
        target_dir = self.platform
        conda_build_path = conda_run(Commands.INFO, '--root').strip()
        packages_path = op.join(conda_build_path, 'conda-bld', target_dir)
        chdir(self.args.remote_channel_dir, sftp)
        mkdir(target_dir, sftp)
        # joining the path of the target directory will badly mangle paths
        # if this CI is run on windows
        chdir(target_dir, sftp)
        for entry in os.scandir(packages_path):
            # a package has a filename like packageName-versionTxt.tar.bz2
            if not entry.name.endswith('tar.bz2'):
                continue
            try:
                package_name, tail = entry.name.split('-', 1)
                if package_name not in self.recipes:
                    continue
                # upload
                print(f"uploading {entry.path}")
                sftp.put(entry.path, entry.name)
            except ValueError:
                # for some reason there is a unformatted package file
                continue

    # -----------------------------------------------------------------------
    # Indexing

    @connected_to_remote
    def index(self, ssh):
        """ all local mirror content to upstream mirror"""
        # Collect the commands to be run
        cmds = []
        cmds.append("source ~/miniconda3/bin/activate")
        dirs = [
            self.args.remote_channel_dir,
            f'{self.args.remote_mirror_dir}/anaconda',
            f'{self.args.remote_mirror_dir}/conda-forge',
            f'{self.args.remote_mirror_dir}/pkgs-main'
        ]
        for dir_ in dirs:
            cmds.append(f"rm {dir_}/channeldata.json")
            cmds.append(f"rm {dir_}/index.html")
            for platform in PLATFORMS.values():
                plat_dir = f"{dir_}/{platform}"
                cmds.append(
                    f"rm -f {plat_dir}/repodata.json*")
                cmds.append(
                    f"rm -f {plat_dir}/repodata_from_packages.json*")
                cmds.append(
                    f"rm -f {plat_dir}/current_repodata.json*")
            cmds.append(
                f"conda index {dir_} --check-md5 --no-progress")

        if os.environ.get('MIRROR_HOSTNAME', None) == "exflctrl01":
            # a version of this script is available in the conda-recipes
            # repository
            cmds.append("python ~/rebuild_index.py")
        # end workaround
        command = "; ".join(cmds)

        # Execute the command
        stds = ssh.exec_command(command)
        _, stdout, stderr = stds

        # Print stdout
        stdout.channel.recv_exit_status()
        print(f"STDOUT")
        for line in stdout.readlines():
            print(line)

        # Print stderr
        print(f"STDERR")
        for line in stderr.readlines():
            print(line)


def append_build_args(parser):
    parser.add_argument('recipes', type=str, nargs='?', default="")

    parser.add_argument(
        '--ci', action='store_true',
        help='Run CI specific configurations')

    parser.add_argument(
        '-c', '--channel', type=str,
        help='host of the conda channel mirror')

    parser.add_argument(
        '-s', '--skip-build', action='store_true',
        help='Skip building')

    parser.add_argument(
        '-f', '--clean', action='store_true',
        help='Clean developement environment')

    parser.add_argument(
        '-T', '--test', action='store_true',
        help='Run tests')

    parser.add_argument(
        '-U', '--upload-to-mirror', action='store_true',
        help='Upload package and dependencies to mirror')

    parser.add_argument(
        '-I', '--index-mirror', action='store_true',
        help='Trigger remote index building')

    parser.add_argument(
        '-N', '--nightly', action='store_true',
        help='Check if this is a nightly build')

    parser.add_argument(
        '-P', '--remote-channel-dir', type=str,
        default='/data/karabo/channel',
        help='Directory of the Packages channel on remote host. '
        'Define this when uploading or repopulating the index')

    parser.add_argument(
        '-C', '--remote-mirror-dir', type=str,
        default='/data/karabo/channel/mirror',
        help='Directory of the mirror channels on remote host. '
        'Define this when uploading or repopulating the index')

    parser.add_argument(
        '-D', '--recipes-dir', type=str, default='conda-recipes',
        help='Base folder for recipes relative to the top level '
        'directory of the git repository')

    return parser


def get_build_args(sub_parser=None, description=None):
    if sub_parser is not None:
        # append this module as a sub parser of `parser`
        parser = sub_parser.add_parser('build',
                                       help=description)
        parser.set_defaults(klass=Builder)
    else:
        parser = argparse.ArgumentParser(description=description)
    return append_build_args(parser)


def main(args):
    b = Builder(args)
    b.run()


DESCRIPTION = """
Conda Recipes Builder

This script will manage the building and uploading of one or more recipes
in a specific folder (see options).
The goal of this script is to facilitate the building, testing and management
of a conda recipes environment.
"""


if __name__ == '__main__':
    root_ap = get_build_args(description=DESCRIPTION)
    args = root_ap.parse_args()
    main(args)
