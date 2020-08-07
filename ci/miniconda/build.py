import argparse
from collections import defaultdict
from functools import partial
import json
import os
import os.path as op
from platform import system as sys_name
import shutil
import subprocess
from tempfile import gettempdir

from conda.cli.python_api import Commands, run_command
from conda.exceptions import PackagesNotFoundError
from paramiko import AutoAddPolicy, SSHClient
import yaml

CHANNEL_MAP = {'pkgs/main': 'https://repo.anaconda.com/pkgs/main/'}
PLATFORMS = {
    "Windows": "win-64",
    "Darwin": "osx-64",
    "Linux": "linux-64"
}


class _MirrorChannel:
    """Stores all the information a mirror should have, such as its name,
    original repository, and target mirror name"""

    def __init__(self, name, mirror_root):
        self.name = name
        self.norm_name = name.replace('/', '-')
        self.original_repo = CHANNEL_MAP.get(name, name)
        # not a path join. this is a URL
        self.mirror_channel = "/".join([mirror_root, 'mirror', self.norm_name])
        # Dictionary containing packages for each platform of this mirror
        self._packages = {}

    def get_packages(self, platform):
        if platform not in self._packages:
            packages = self._obtain_packages_on_mirror(platform)
            self._packages[platform] = packages
        return self._packages[platform]

    def _obtain_packages_on_mirror(self, platform):
        """Returns the packages that are already on our mirror channel"""
        try:
            packages_on_mirror = conda_run(
                Commands.SEARCH,
                '--override-channels', '-c',
                self.mirror_channel, f'*[subdir={platform}]',
                '--json')

            # All these packages are already on the target mirror,
            # so it is not necessary to upload them
            to_exclude = defaultdict(list)
            print(f'Optimize upload for {self.norm_name}-{platform}')
            for name, packages in json.loads(packages_on_mirror).items():
                for package in packages:
                    to_exclude[package['name']].append((package['version'],
                                                        package['build']))
            return to_exclude
        except PackagesNotFoundError as e:
            print(f"no packages from {self.mirror_channel} for {platform}")
            return {}


class Mirrors:
    """Holds the information of all mirrros and the packages we will need to
    download for each mirror and platform"""

    def __init__(self, reference_environment, mirror_channel):
        self.mirror_channel = mirror_channel
        self._mirrors = {}
        self._needed_packages = defaultdict(partial(defaultdict, list))
        self.reference_environment = reference_environment

    def _generate_needed_packages(self):
        """Returns the packages we will need to download. This excludes packages
        already present in our mirror channels"""
        output = conda_run(
            Commands.LIST, '-n', self.reference_environment, '--json')

        for pkg in json.loads(output):
            if pkg['base_url'].startswith('file://'):
                continue

            # skip local files
            channel_name = pkg['channel']
            if channel_name in ['karabo/channel', 'pypi', '']:
                continue

            channel = self._mirrors.setdefault(
                channel_name,
                _MirrorChannel(
                    channel_name, mirror_root=self.mirror_channel))

            name = pkg['name']
            version = pkg['version']
            build = pkg['build_string']
            platform = pkg['platform']

            to_exclude = channel.get_packages(platform)

            if name in to_exclude and (version, build) in to_exclude[name]:
                # this package is already uploaded
                print(f'Skipping upload of {name} to {channel_name}')
                continue

            self._needed_packages[channel_name][platform].append(
                (name, version, build))

    def populate(self, target_dir):
        """Populate the mirror in the given target directory"""
        self._generate_needed_packages()
        for channel_name, platforms in self._needed_packages.items():
            mirror = self._mirrors[channel_name]
            # Create a configuration file for each platform's packages
            print("needing", channel_name, platforms)
            for pkg_platform, needed_packages in platforms.items():
                to_write = 'blacklist:\n' \
                           '    - name: "*"\n' \
                           'whitelist:\n'

                # Whitelist all packages that are not yet mirrored
                for name, version, build in needed_packages:
                    to_write += f'    - name: {name}\n' \
                        f'      version: \'{version}\'\n' \
                        f'      build: \'{build}\'\n'
                conf_filename = f'{mirror.norm_name}.yaml'
                conf_file = op.join(target_dir, conf_filename)
                with open(conf_file, 'w') as f:
                    f.write(to_write)

                print(f"""
Creating mirror {mirror.name} - {pkg_platform} with the following configuration

{to_write}""")
                self._populate_mirror(
                    mirror, pkg_platform, conf_file, target_dir)

    def _populate_mirror(self, mirror, platform, conf_file, target_dir):
        """Calls a conda-mirror process in order to download all the packages
        we need"""

        target_mirror_directory = op.join(target_dir, mirror.norm_name)

        # Create the mirror directory for later
        if not op.isdir(target_mirror_directory):
            os.makedirs(target_mirror_directory)

        print(f'Mirroring channel {mirror.name}...')
        conda_run(
            Commands.RUN,
            '-n', 'base',
            'conda', 'mirror',
            '--upstream-channel', mirror.original_repo,
            '--target-directory', target_mirror_directory,
            '--platform', platform,
            '--config', str(conf_file))
        os.remove(conf_file)


class Builder:
    """Main Build Manager for the Karabo Control System

    will take an argument namespace in input, it will build and
    optionally upload a recipe and its dependencies.
    """

    def __init__(self, args):
        # we get the root of the git repository here
        r = subprocess.check_output(
                ['git', 'rev-parse', '--show-toplevel'])
        self.root_path = op.normpath(r.decode().strip())
        self.recipes = set()
        self.platform = ''
        self.mirror_dir = op.join(gettempdir(), 'mirror')
        self.args = args
        self.mirror_channel = f'http://{args.channel}/karabo/channel'
        self.recipes_dir = self.args.recipes_dir
        platform = PLATFORMS.get(sys_name())
        if platform is None:
            raise RuntimeError(f'Unsupported platform {sys_name()}')
        self.platform = platform

    def clean(self, recipe):
        """Cleans the environment of a package and purges old build dirs"""
        print("Cleaning", recipe)
        conda_run(
            Commands.RUN,
            '-n', 'base',
            'conda', 'build', 'purge-all')
        try:
            env = get_env_name(recipe)
            conda_run(
                Commands.REMOVE,
                '-n', env, '--all')
        except RuntimeError:
            # this might fail if the environement does not exist
            pass

    def prepare_mirror_dir(self):
        if self.args.upload_to_mirror:
            # clean the mirror directory
            if op.isdir(self.mirror_dir):
                shutil.rmtree(self.mirror_dir)
            os.makedirs(self.mirror_dir)

    def adapt_platform(self, recipe):
        """Performs platform specific tasks per recipe"""
        if self.platform == 'osx-64':
            os.environ['LANG'] = 'en_US.UTF-8'
            osx_sysroot = os.environ.get('OSX_SYSROOT')
            if osx_sysroot:
                conf = {'CONDA_BUILD_SYSROOT': [osx_sysroot]}
                conf_path = op.join(
                    self.root_path, 'recipes',
                    recipe, 'conda_build_config.yaml')
                with open(conf_path, 'w') as f:
                    f.write(yaml.dump(conf))
        elif self.platform == 'linux-64':
            # This is a CI specific setting
            proxy_server = os.environ.get('PROXY_SERVER')
            if proxy_server:
                os.environ['http_proxy'] = f'http://{proxy_server}/'
                os.environ['https_proxy'] = f'https://{proxy_server}/'
            os.environ['DISPLAY'] = ':99.0'
            subprocess.check_output([
                'start-stop-daemon', '--start', '-b', '-x', '/usr/bin/Xvfb',
                '--', ':99.0', '-screen', '0', '1024x768x24', '-ac'])

    def build_recipe(self, recipe):
        """"Builds a recipe"""
        recipe_path = op.join(
            self.root_path, self.args.recipes_dir, recipe,
            'meta.yaml')
        devenv_path = op.join(
            self.root_path, self.args.recipes_dir, recipe,
            'environment.devenv.yml')
        recipe_base_path = op.join(
            self.root_path, self.args.recipes_dir, recipe,
            'meta_base.yaml')
        if all([op.exists(recipe_base_path),
                op.exists(devenv_path)]):
            return self.build_recipe_from_base(
                recipe, recipe_base_path, devenv_path, recipe_path)
        if op.exists(recipe_path):
            return self._build_recipe(recipe, recipe_path)

    def build_recipe_from_base(self,
                               recipe, recipe_base_path,
                               devenv_path, recipe_path):
        """"Builds a recipe file from dev-env and base recipe"""
        # the devenv environment is called like the recipe,
        # therefore, we clean it
        if recipe != 'karabogui':
            raise RuntimeError(f'Recipe {recipe} not implemented')

        if self.args.clean:
            conda_run(Commands.REMOVE, '-n', recipe, '--all')
        os.environ['XFEL_CONDA_CHANNEL'] = self.args.channel
        conda_run(
            Commands.RUN, '-n', 'base',
            'conda', 'devenv', '--file', devenv_path)

        # setup.py paths
        py_karabo_root = op.join(
            self.root_path, 'src', 'pythonKarabo')
        gui_root = op.join(
            self.root_path, 'src', 'pythonGui')
        # FIXME: install native without environment variable
        os.environ['BUILD_KARABO_GUI'] = '1'
        # Install the code in the environment `karabogui`
        conda_run(
            Commands.RUN, '-n', recipe, '--cwd', py_karabo_root,
            'python', 'setup.py', 'install')
        os.environ.pop('BUILD_KARABO_GUI')

        conda_run(
            Commands.RUN, '-n', recipe, '--cwd', gui_root,
            'python', 'setup.py', 'install')

        if self.args.test:
            modules_to_test = ['karabogui', 'karabo.native', 'karabo.common']
            self.test_environment(recipe, modules_to_test)
            return

        if self.args.skip_build:
            return

        # build the meta.yaml file from the meta_base file
        conda_run(
            Commands.RUN, '-n', 'base',
            'python', '-m', 'cogapp', '-o', recipe_path, recipe_base_path)
        # Print the recipe
        print(f'Building Recipe for "{recipe}":')
        with open(recipe_path) as recipe_file:
            for line in recipe_file:
                print(line, end='')
        print(f'Building recipe for {recipe}')
        conda_run(
            Commands.RUN,
            '-n', 'base',
            'conda', 'build', op.dirname(recipe_path),
            '-c', self.mirror_channel,
            '-c', 'conda-forge',
            '-c', 'defaults',
            '--no-anaconda-upload')
        # create a mirror from the recipe
        self.create_mirror(recipe)

    def _build_recipe(self, recipe, recipe_path):
        """"Builds a recipe from meta.yaml file"""
        if self.args.skip_build:
            return
        recipe_dir = op.dirname(recipe_path)
        print(f'Building recipe for {recipe}')
        conda_run(
            Commands.RUN,
            '-n', 'base',
            'conda', 'build', recipe_dir, '-c', self.mirror_channel,
            '-c', 'conda-forge',
            '-c', 'defaults',
            '--no-anaconda-upload')
        print("Recipe build successful")
        self.install(recipe)
        env = get_env_name(recipe)
        self.create_mirror(env)

    def run(self):
        for recipe in self.args.recipes.split(','):
            self.recipes.add(recipe)
        if len(self.recipes) == 0:
            print("Nothing to do!")
            return
        if (self.args.nightly and
                os.environ.get("SCHEDULED_JOB", "") == "nightly-build"):
            print("Not a nightly-build scheduled job!")
            return
        if self.args.clean:
            for recipe in self.recipes:
                self.clean(recipe)
        self.prepare_mirror_dir()
        for recipe in self.recipes:
            # run platform specific settings per recipe
            self.adapt_platform(recipe)
            # Build the recipe
            self.build_recipe(recipe)
        # upload to mirror
        self.upload()
        # index mirror
        self.index()

    def install(self, recipe):
        """Installs a local package into a 'build' environmemnt"""
        if not self.args.upload_to_mirror:
            return
        # create the environment
        env = get_env_name(recipe)
        print(f'create environment {env}')
        conda_run(
            Commands.CREATE,
            '-n', env)
        # locate the package
        print(f'install local package for "{recipe}" in env "{env}"')
        conda_run(
            Commands.RUN,
            '-n', env,
            'conda', 'install', '-y', '-c', 'local', recipe)

    def create_mirror(self, env):
        if not self.args.upload_to_mirror:
            return
        print(f"Identifying packages to mirror for {env}")
        mirrors = Mirrors(env, self.mirror_channel)
        mirrors.populate(self.mirror_dir)

    def upload(self):
        """Uploads all local mirror content to upstream mirror"""
        if not self.args.upload_to_mirror:
            return
        with SSHClient() as ssh_client:
            ssh_client.set_missing_host_key_policy(AutoAddPolicy())
            hostname, user, password = get_host_from_env()
            ssh_client.connect(hostname=hostname,
                               username=user, password=password)
            with ssh_client.open_sftp() as sftp:
                # upload the mirror content
                self.walk_mirror_dirs(sftp)
                # upload the recipes for this platform
                self.upload_recipes(sftp, self.platform)
                # FIXME: the recipes have strict dependencies
                # noarch packages would not work.
                # self.upload_recipes(sftp, 'noarch')

    def upload_recipes(self, sftp, plat_dir):
        """Transfers packages using the sftp client"""
        print("uploading packages")
        conda_build_path = conda_run(Commands.INFO, '--root').strip()
        packages_path = op.join(conda_build_path, 'conda-bld', plat_dir)
        sftp.chdir(self.args.remote_channel_dir)
        target_dir = plat_dir
        try:
            sftp.stat(target_dir)
        except FileNotFoundError:
            print(f"Creating missing directory {target_dir}")
            sftp.mkdir(target_dir)
        # joining the path of the target directory will badly mangle paths
        # if this CI is run on windows
        sftp.chdir(target_dir)
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

    def walk_mirror_dirs(self, sftp):
        """Transfers mirror files using the sftp client"""
        print("uploading mirror packages")
        sftp.chdir(self.args.remote_mirror_dir)
        for dir_, subdirs, files in os.walk(self.mirror_dir):
            target_dir = op.relpath(dir_, self.mirror_dir)
            # the remote server will have a unix-like path.
            # this call converts windows directories structure
            # and is a nop in unix like systems.
            dir_parts = [self.args.remote_mirror_dir]
            dir_parts.extend(op.split(target_dir))
            remote_dir = "/".join(dir_parts)
            try:
                sftp.stat(remote_dir)
            except FileNotFoundError:
                print(f"Creating missing directory {remote_dir}")
                sftp.mkdir(remote_dir)

            for filename in files:
                # the remote server will have a unix path.
                # running a `op.join` on windows will mangle the paths
                remote_file_path = '/'.join([remote_dir, filename])
                local_file_path = op.join(dir_, filename)
                print(f'Uploading {local_file_path} to {remote_file_path}')
                sftp.put(local_file_path, remote_file_path)

    def index(self):
        """ all local mirror content to upstream mirror"""
        if not self.args.index_mirror:
            return
        with SSHClient() as ssh_client:
            ssh_client.set_missing_host_key_policy(AutoAddPolicy())
            hostname, user, password = get_host_from_env()
            ssh_client.connect(hostname=hostname,
                               username=user, password=password)
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
            command = "; ".join(cmds)
            stds = ssh_client.exec_command(command)
            _, stdout, stderr = stds
            stdout.channel.recv_exit_status()
            print(f"STDOUT")
            for line in stdout.readlines():
                print(line)
            print(f"STDERR")
            for line in stderr.readlines():
                print(line)

    def test_environment(self, env, modules_to_test):
        errors = []
        for module_name in modules_to_test:
            cmd = [Commands.RUN, '-n', env, 'nosetests', '-v', module_name]
            try:
                conda_run(*cmd)
            except RuntimeError:
                errors.append(module_name)

        if errors:
            raise RuntimeError(f"Failures in modules {','.join(errors)}")

        print('Tests successful')


def conda_run(command, *args, **kwargs):
    stdout, stderr, ret_code = run_command(command, *args, **kwargs)
    if ret_code != 0:
        msg = f'Command {command} [{args}] '\
            f'{kwargs} returned {ret_code}\n{stderr}'
        raise RuntimeError(msg)
    return stdout


def get_env_name(recipe):
    return f"build_env_{recipe}"


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

    root_ap = argparse.ArgumentParser(
        description=DESCRIPTION)

    recipes_kwargs = {
        'type': str,
        'nargs': '?',
        'default': 'ON_FEATURE_BRANCH',
        'help': 'Target Environment, The reserver keywords "ON_FEATURE_BRANCH"'
        ' and "ON_MERGE" will detect from the git history what to do. '
        'In case of ON_MERGE the reference commit is the previous commit, '
        'in case of ON_FEATURE_BRANCH the reference commit is the earliest '
        'common commit with the master branch.'
    }

    root_ap.add_argument('recipes', **recipes_kwargs)

    root_ap.add_argument(
        '-c', '--channel', type=str, default='exflserv05.desy.de',
        help='internal conda channel')

    root_ap.add_argument(
        '-s', '--skip-build', action='store_true',
        help='Skip building')

    root_ap.add_argument(
        '-f', '--clean', action='store_true',
        help='clean developement environment')

    root_ap.add_argument(
        '-T', '--test', action='store_true',
        help='run tests')

    root_ap.add_argument(
        '-U', '--upload-to-mirror', action='store_true',
        help='upload package and dependencies to mirror')

    root_ap.add_argument(
        '-I', '--index-mirror', action='store_true',
        help='trigger remote index building')

    root_ap.add_argument(
        '-N', '--nightly', action='store_true',
        help='check if this is a nightly build')

    root_ap.add_argument(
        '-P', '--remote-channel-dir', type=str,
        default='/var/html/www/karabo/channel',
        help='directory of the Packages channel on remote host. '
        'Define this when uploading or repopulating the index')

    root_ap.add_argument(
        '-C', '--remote-mirror-dir', type=str,
        default='/var/html/www/karabo/channel/mirror',
        help='directory of the mirror channels on remote host '
        'Define this when uploading or repopulating the index')

    root_ap.add_argument(
        '-D', '--recipes-dir', type=str, default='recipes',
        help='base folder for recipes relative to the top level '
        'directory of the git repository')

    args = root_ap.parse_args()
    main(args)
