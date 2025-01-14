import argparse
import os
import os.path as op
import shutil
from contextlib import contextmanager
from platform import system as sys_name
from tempfile import gettempdir

import yaml

from .mirrors import Mirrors
from .utils import (
    chdir, command_run, conda_run_command, connected_to_remote,
    environment_exists, mkdir)

PLATFORMS = {"Windows": "win-64", "Darwin": "osx-64", "Linux": "linux-64"}
KARABOGUI = "karabogui"


class Builder:

    def __init__(self, args):
        self.args = args

        # Local paths
        r = command_run(["git", "rev-parse", "--show-toplevel"])
        self.root_path = op.normpath(r.strip())
        self.mirror_dir = op.join(gettempdir(), "mirror")

        # Platform
        self.platform = ""
        platform = PLATFORMS.get(sys_name())
        if platform is None:
            raise RuntimeError(f"Unsupported platform {sys_name()}")
        self.platform = platform

        # Recipe
        self.recipes = set()

        # Conda setup
        command_run(["conda", "config", "--set", "pip_interop_enabled",
                     "True"])

    # -----------------------------------------------------------------------
    # Main logic

    def run(self):
        actions = {
            self.args.clean,
            self.args.index_mirror,
            self.args.test,
            self.args.upload_to_mirror,
            not self.args.skip_build,
        }
        if all([not action for action in actions]):
            print("No action requested")

        for recipe in self.args.recipes.split(","):
            if recipe:
                self.recipes.add(recipe)

        # A wrongly triggered nightly build
        is_nightly = os.environ.get("SCHEDULED_JOB", "") == "nightly-build"
        if self.args.nightly and not is_nightly:
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
        return f"http://{self.args.channel}/karabo/channel"

    @property
    def mirror_conda_forge(self):
        return f"http://{self.args.channel}/karabo/channel/mirror/conda-forge"

    @staticmethod
    def get_env_name(recipe):
        return f"build_env_{recipe}"

    def recipe_path(self, recipe):
        return op.join(
            self.root_path, self.args.recipes_dir, recipe, "meta.yaml"
        )

    def devenv_path(self, recipe):
        return op.join(
            self.root_path,
            self.args.recipes_dir,
            recipe,
            "environment.devenv.yml",
        )

    def recipe_base_path(self, recipe):
        return op.join(
            self.root_path, self.args.recipes_dir, recipe, "meta_base.yaml"
        )

    # -----------------------------------------------------------------------
    # Setting

    def adapt_platform(self):
        """Performs platform specific tasks per recipe"""
        if self.platform == "osx-64":
            os.environ["LANG"] = "en_US.UTF-8"

    def clean(self):
        print("Cleaning conda..")
        command = ["conda", "build", "purge-all", "--quiet"]
        conda_run_command(cmd=command, env_name="base")
        command_run(["conda", "clean", "--all", "--force-pkgs-dirs", "--quiet",
                     "--yes"])
        for recipe in self.recipes:
            self.clean_environment(name=recipe)

    def clean_environment(self, *, name):
        """Cleans the environment of a package and purges old build dirs"""
        print("Cleaning", name)

        try:
            # Remove the environment for the recipe
            command_run(
                ["conda", "remove", "-n", name, "--all", "quiet", "--yes"])
        except Exception as e:
            # this might fail if the environment does not exist
            print(
                f"Tried removing `{name}` environment, ..."
                f"got exception {e}")

    # -----------------------------------------------------------------------
    # Installing and testing Karabo

    def install_karabo(self, recipe):
        # setup.py paths
        py_karabo_root = op.join(self.root_path, "src", "pythonKarabo")
        gui_root = op.join(self.root_path, "src", "pythonGui")

        # Install pythonKarabo in the `recipe` environment
        # FIXME: install native without environment variable
        os.environ["BUILD_KARABO_SUBMODULE"] = "NATIVE"
        command = ["pip", "install", "-e", py_karabo_root]
        conda_run_command(cmd=command, env_name=recipe)
        os.environ.pop("BUILD_KARABO_SUBMODULE")
        # Install pythonGui in the `recipe` environment
        command = ["pip", "install", "-e", gui_root]
        conda_run_command(cmd=command, env_name=recipe)

    def uninstall_karabo(self, recipe):
        # no need to uninstall if we are not building an env pack
        if self.args.skip_build:
            return
        # Prepare env pack by uninstalling karabo and karabogui
        for excluded_pkgs in ["karabo", "karabogui"]:
            command = ["pip", "uninstall", excluded_pkgs, "--yes"]
            conda_run_command(cmd=command, env_name=recipe)

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
        test_script = op.join(recipe_dir, "_run_test.py")
        if not op.exists(test_script):
            print(f"Test script '{test_script}' missing")

        # Check if there's an existing devenv environment
        if not environment_exists(recipe):
            self.create_devenv(recipe)

        with self.karabo_installed(recipe):
            output = command_run(["conda", "list"])
            print(output)

            conda_run_command(
                ["python", "-m", "pip", "install", "pytest-xvfb"],
                env_name=recipe)

            command = ["python", test_script]
            output = conda_run_command(cmd=command, env_name=recipe)
            if output:
                print(output)
            print("Tests successful")

    # -----------------------------------------------------------------------
    # Building recipe

    def prepare_build(self, recipe):
        # Build settings
        command = ["python", "-m", "pip", "install", "cogapp"]
        conda_run_command(cmd=command)
        osx_sysroot = os.environ.get("OSX_SYSROOT")
        if osx_sysroot:
            conf = {"CONDA_BUILD_SYSROOT": [osx_sysroot]}
            conf_path = op.join(
                self.root_path, "recipes", recipe, "conda_build_config.yaml")
            with open(conf_path, "w") as f:
                f.write(yaml.dump(conf))

    def build(self, recipe):
        """ "Builds a recipe"""
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
        os.environ["XFEL_CONDA_CHANNEL"] = self.args.channel
        command_run(
            ["conda", "run", "-n", "base", "conda", "devenv", "--file",
             self.devenv_path(recipe)])

    def build_recipe_from_base(self, recipe):
        # build the meta.yaml file from the meta_base file
        output_file = self.recipe_path(recipe)
        command = ["python", "-m", "cogapp", "-o", output_file,
                   self.recipe_base_path(recipe)]
        conda_run_command(cmd=command, env_name="base")
        # Print the recipe
        print(f'Building Recipe for "{recipe}":')
        with open(output_file) as recipe_file:
            for line in recipe_file:
                print(line, end="")

    def build_recipe(self, recipe):
        """ "Builds a recipe from meta.yaml file"""
        print(f"Building recipe for {recipe}")
        recipe_dir = op.dirname(self.recipe_path(recipe))
        command = [
            "conda", "build", recipe_dir,
            "-c", self.mirror_channel,
            "-c", self.mirror_conda_forge,
            "-c", "conda-forge",
            "--override-channels",
            "--no-anaconda-upload",
            "--quiet"
        ]
        conda_run_command(cmd=command, env_name="base")
        print("Recipe build successful")

    def install_build(self, recipe):
        """Installs a local package into a 'build' environmemnt"""
        if not self.args.upload_to_mirror:
            return
        # Create or use the existing environment
        env = self.get_env_name(recipe)
        if not environment_exists(env):
            print(f"Create environment {env}")
            command_run(["conda", "create", "-n", env, "--yes"])

        # locate the package
        print(f'install local package for "{recipe}" in env "{env}"')
        command = ["conda", "install", recipe,
                   "-y", "--quiet",
                   "-c", self.mirror_channel,
                   "-c", self.mirror_conda_forge,
                   "-c", "conda-forge",
                   "-c", "local"]
        conda_run_command(cmd=command, env_name=env)
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
                remote_file_path = "/".join([remote_dir, filename])
                local_file_path = op.join(dir_, filename)
                print(f"Uploading {local_file_path} to {remote_file_path}")
                sftp.put(local_file_path, remote_file_path)

    def upload_recipes(self, sftp):
        """Transfers packages using the sftp client"""
        print("uploading recipes")
        target_dir = self.platform
        if target_dir == "win-64":
            # XXX: Special treatment for windows as conda info return
            # difficult return value on CI
            conda_build_path = os.getenv("WIN_CONDA_ROOT").strip()
            assert conda_build_path, "Conda root must be set in CI variables"
        else:
            conda_build_path = command_run(["conda", "info", "--root"]).strip()

        print("Conda build path", conda_build_path)
        packages_path = op.join(conda_build_path, "conda-bld", target_dir)
        chdir(self.args.remote_channel_dir, sftp)
        mkdir(target_dir, sftp)
        # joining the path of the target directory will badly mangle paths
        # if this CI is run on windows
        chdir(target_dir, sftp)
        for entry in os.scandir(packages_path):
            # a package has a filename like packageName-versionTxt.tar.bz2
            if not entry.name.endswith("tar.bz2"):
                continue
            try:
                package_name, tail = entry.name.split("-", 1)
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
        """all local mirror content to upstream mirror"""
        # Collect the commands to be run
        cmds = []
        cmds.append("source ~/miniforge3/bin/activate")
        dirs = [
            self.args.remote_channel_dir,
            f"{self.args.remote_mirror_dir}/conda-forge",
        ]
        for dir_ in dirs:
            cmds.append(f"rm {dir_}/channeldata.json")
            cmds.append(f"rm {dir_}/index.html")
            for platform in PLATFORMS.values():
                plat_dir = f"{dir_}/{platform}"
                cmds.append(f"rm -f {plat_dir}/repodata.json*")
                cmds.append(f"rm -f {plat_dir}/repodata_from_packages.json*")
                cmds.append(f"rm -f {plat_dir}/current_repodata.json*")
            cmds.append(f"conda index {dir_}")
            print(f"Adding conda index for dir: {dir_}")
        if os.environ.get("MIRROR_HOSTNAME", None) == "exflctrl01":
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


DESCRIPTION = """
Conda Recipes Builder

This script will manage the building and uploading of one or more recipes
in a specific folder (see options).
The goal of this script is to facilitate the building, testing and management
of a conda recipes environment.
"""


def main():
    ap = argparse.ArgumentParser(description=DESCRIPTION)
    ap.add_argument("recipes", type=str, nargs="?", default="")

    ap.add_argument("--ci", action="store_true",
                    help="Run CI specific configurations")

    ap.add_argument("-c", "--channel", type=str,
                    help="host of the conda channel mirror")

    ap.add_argument("-s", "--skip-build", action="store_true",
                    help="Skip building")

    ap.add_argument("-f", "--clean", action="store_true",
                    help="Clean development environment")

    ap.add_argument("-T", "--test", action="store_true", help="Run tests")

    ap.add_argument("-U", "--upload-to-mirror", action="store_true",
                    help="Upload package and dependencies to mirror")

    ap.add_argument("-I", "--index-mirror", action="store_true",
                    help="Trigger remote index building")

    ap.add_argument("-N", "--nightly", action="store_true",
                    help="Check if this is a nightly build")

    ap.add_argument("-P", "--remote-channel-dir", type=str,
                    default="/data/karabo/channel",
                    help="Directory of the Packages channel on remote host. "
                         "Define when uploading or repopulating the index")

    ap.add_argument("-C", "--remote-mirror-dir", type=str,
                    default="/data/karabo/channel/mirror",
                    help="Directory of the mirror channels on remote host. "
                         "Define when uploading or repopulating the index")

    ap.add_argument("-D", "--recipes-dir", type=str, default="conda-recipes",
                    help="Base folder for recipes relative to the top level "
                         "directory of the git repository")

    args = ap.parse_args()
    b = Builder(args)
    b.run()
