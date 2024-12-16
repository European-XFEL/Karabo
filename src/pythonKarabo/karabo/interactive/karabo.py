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
import argparse
import contextlib
import copy
import csv
import glob
import multiprocessing
import os
import platform
import re
import subprocess
import sys
from importlib.metadata import entry_points

from karabo import __version__ as karabo_version
from karabo.packaging.device_template import configure_template


def run_local():
    try:
        karabo_dir = os.environ['KARABO']
        os.chdir(karabo_dir)
    except KeyError:
        print('$KARABO is not defined. Make sure you have sourced the '
              'activate script for the Karabo Framework which you would '
              'like to use.')
        sys.exit(1)


def run_cmd(cmd):
    try:
        output = subprocess.check_output(cmd, shell=True,
                                         stderr=subprocess.STDOUT)
        return output
    except subprocess.CalledProcessError as e:
        print("Problem with system call:\n{}"
              .format(e.output.decode("utf-8")))
        exit(e.returncode)


@contextlib.contextmanager
def pushd_popd():
    path = os.environ['KARABO']
    old_path = os.getcwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(old_path)


def resolve_project(package):
    """Resolve full gitlab project name.

    mainly due to the European XFEL git repository structure.
    If no `/` is present, the `/karaboDevices/` path is prepended.
    """
    if package.startswith("/"):
        return package
    elif '/' in package:
        return f"/{package}"
    else:
        return f"/karaboDevices/{package}"


def parse_commandline():
    parser = argparse.ArgumentParser(
        description=(f'Karabo Utility Script - version {karabo_version}'),
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    sps = parser.add_subparsers(metavar='')

    parser_new = sps.add_parser('new',
                                help='Creates a new device from template')
    parser_new.set_defaults(command=new)

    parser_new.add_argument('device',
                            type=str,
                            help='The name of the new device package')

    parser_new.add_argument('api',
                            type=str.lower,
                            metavar='api',
                            choices=['cpp', 'python', 'middlelayer'],
                            help='The API of the new device {cpp|python|'
                                 'middlelayer}')

    parser_new.add_argument('-f', '--force',
                            action='store_true',
                            help='Force creation of device, may override '
                                 'existing')

    parser_new.add_argument('-t', '--template',
                            type=str,
                            default='minimal',
                            help='The name of the set of template files to use'
                                 ' for scaffolding the new device project. '
                                 'Defaults to "minimal" (supported by all '
                                 'APIs).')

    parser_chk = sps.add_parser('checkout',
                                help='Checks out a device (sources) from the '
                                     'repository')
    parser_chk.set_defaults(command=checkout)

    parser_chk.add_argument('device',
                            type=str,
                            help='The name of the device package')

    parser_chk.add_argument('-b', '--branch',
                            type=str,
                            default='',
                            help='The branch/tag of the device package. '
                                 'If empty, use HEAD of the remote repo.')

    parser_ins = sps.add_parser('install',
                                help='Installs an existing device')

    parser_ins.add_argument('device',
                            type=str,
                            help='The name of the device package. '
                                 'if this device package does not '
                                 'include a `/` the `/karaboDevices/` '
                                 'sub-path will be prepended for '
                                 'backwards compatibility. '
                                 'Note: this name might not be '
                                 'case sensitive on the repository '
                                 'side depending on the protocol '
                                 'chosen.')

    parser_ins.add_argument('tag',
                            type=str,
                            help='The tag to install')

    parser_ins.add_argument('--copy',
                            type=str, default='True',
                            help='artifacts will be copied into the plugins '
                                 'folder unless --copy=False')

    parser_ins.add_argument('-f', '--force',
                            action='store_true',
                            help='Force installation of device and its '
                            'dependencies, may overwrite existing (this option'
                            ' is ignored when the -n option is also used)')

    parser_ins.add_argument('-n', '--no-clobber',
                            action='store_true',
                            help='Do not overwrite an existing device and its '
                            'dependencies (overrides a -f option)')

    parser_ins.set_defaults(command=install)

    parser_uins = sps.add_parser('uninstall',
                                 help='Uninstalls an existing device. '
                                 'Dependencies will not be uninstalled. '
                                 'NOTE: Not guaranteed to work for non '
                                 'standard device packages nor dependencies, '
                                 'or if installed with mismatch between upper'
                                 '/lower character cases wrt. package '
                                 'expectation.')
    parser_uins.set_defaults(command=uninstall)

    parser_uins.add_argument('device',
                             type=str,
                             help='The name of the device package')

    parser_dev = sps.add_parser('develop',
                                help='Activates develop mode for a given '
                                     'device')
    parser_dev.set_defaults(command=develop)

    parser_dev.add_argument('device',
                            type=str,
                            help='The name of the device package'
                                 'Note: this name is not case sensitive'
                                 ' on the repository side.')

    parser_dev.add_argument('-b', '--branch',
                            type=str,
                            default='',
                            help='The branch/tag of the device package. '
                                 'If empty, use HEAD of the remote repo.')

    parser_dev.add_argument('-f', '--force',
                            action='store_true',
                            help='Force installation of device\'s '
                            'dependencies, may overwrite existing (this option'
                            ' is ignored when the -n option is also used)')

    parser_dev.add_argument('-n', '--no-clobber',
                            action='store_true',
                            help='Do not overwrite device\'s '
                            'dependencies (overrides a -f option)')

    parser_udev = sps.add_parser('undevelop',
                                 help='Deactivates develop mode for a given '
                                      'device. '
                                      'NOTE: Not guaranteed to work for non '
                                      'standard device packages nor '
                                      'dependencies')
    parser_udev.set_defaults(command=undevelop)

    parser_udev.add_argument('device',
                             type=str,
                             help='The name of the device package')

    parser_list = sps.add_parser('list',
                                 help='List all installed devices')
    parser_list.set_defaults(command=list_devices)
    parser_list.add_argument('-i', '--internal', action='store_true',
                             help='Show internal plugins as well')
    parser_list.add_argument('-m', '--machine', action='store_true',
                             help='Machine-parseable output')

    parser.add_argument('-c', '--config',
                        type=str,
                        choices=['Debug', 'Release', 'Simulation'],
                        default='Release',
                        help='Build configuration {Debug|Release|Simulation}')

    parser.add_argument('-g', '--git',
                        type=str,
                        default='ssh://git@git.xfel.eu:10022',
                        help='URL to the git repository')

    parser.add_argument('-r', '--repo',
                        type=str,
                        default='http://exflctrl01.desy.de/karabo/'
                                'karaboDevices/',
                        help='URL to the binary repository')

    parser.add_argument('-j', '--jobs',
                        type=int,
                        default=multiprocessing.cpu_count(),
                        help='Specifies the number of make jobs (commands) '
                             'to run simultaneously.')

    parser.add_argument('-v', '--version', action='store_true',
                        help='prints the version of Karabo. '
                             'No command will be executed.')

    args = parser.parse_args()
    if len(sys.argv) <= 1:
        parser.print_help()
        parser.exit()
    if args.version:
        print(f"Karabo Version - {karabo_version}")
        return
    args.command(args)


def new(args):
    with pushd_popd():
        path = os.path.join('devices', args.device)
        class_name = args.device[0].capitalize() + args.device[1:]
        if os.path.isdir(path):
            if args.force:
                run_cmd(f'rm -rf {path}')
            else:
                print('Device {} already exists.'
                      .format(args.device))
                return
        tpath = os.path.join('templates', args.api, args.template)
        if not os.path.isdir(tpath):
            print(
                f'Template set "{args.template}" not available for API '
                f'"{args.api}".')
            troot = os.path.join('templates', args.api)
            avail_sets = [
                f'"{d}"' for d in os.listdir(troot)
                if os.path.isdir(os.path.join(troot, d))]
            print(f'    - available set(s): {", ".join(avail_sets)}.')
            return
        run_cmd(f'mkdir -p {path}')
        run_cmd('cp -rf {} {}'.format(os.path.join(tpath, '*'), path))
        # '.[gi]*' matches '.gitignore', '.install.sh' and '.gitlab-ci.yml',
        # but neither '.' nor '..', which would generate an error on the cp
        # command and interrupt the script.
        run_cmd('cp -fp {} {}'.format(os.path.join(tpath, '.[gi]*'), path))
        email = os.environ.get('USER', 'Unknown')
        configure_template(path, args.device, class_name, email,
                           " ".join([args.template, args.api]))
        os.chdir(path)
        # From git version 2.28 onwards we could use
        # 'git init --initial-branch=main' and skip the 'checkout -b' step
        run_cmd('git init')
        run_cmd('git checkout -b main')
        run_cmd('git remote add origin {}/karaboDevices/{}.git'
                .format(args.git, args.device))
        run_cmd('git add .')
        run_cmd('git commit -m "Initial commit"')
        print('New device package was added to: {}'
              .format(os.getcwd()))
        print('Use: "git push -u origin main" if you want start versioning '
              'remotely')


def checkout(args):
    with pushd_popd():
        path = os.path.join('devices', args.device.split('/')[-1])
        if os.path.isdir(path):
            print('INFO The device package already exists, skipped checkout')
        else:
            print(f'Downloading {args.device}... ', end='', flush=True)
            git_opt = f"-b {args.branch} " if args.branch else ''
            run_cmd(f"git clone {args.git}{resolve_project(args.device)}.git"
                    f" {git_opt}{path}")
            print('done.')
            print('Device package was added to: {}'
                  .format(os.path.abspath(path)))


def _get_lsb_release_info() -> dict[str, str]:
    resp: dict[str, str] = {}
    if os.path.exists("/etc/os-release"):
        # /etc/os-release contains distribution and version info for both
        # Debian and RHEL based Linux distributions. Use it as the primary
        # source for distribution and version info.
        with open("/etc/os-release") as f:
            for line in f:
                if line.upper().startswith("NAME="):
                    dist_name = line[5:].replace('"', '').split()[0].rstrip()
                    resp["LSB_RELEASE_DIST"] = dist_name
                elif line.upper().startswith("VERSION_ID="):
                    version = line[11:].replace('"', '').split(".")[0].rstrip()
                    resp["LSB_RELEASE_VERSION"] = version
    else:
        # Uses lsb_release command as a second option
        resp["LSB_RELEASE_DIST"] = run_cmd(
            'lsb_release -is').decode("utf-8").rstrip()
        resp["LSB_RELEASE_VERSION"] = run_cmd(
            'lsb_release -rs').decode("utf-8").split('.')[0].rstrip()
    return resp


def download(args):
    """
    attempts the download of a package

    :return: the path of the downloaded package, or None in case of failure.
    """
    if args.repo == '':
        return None
    with pushd_popd():
        karabo_tag = karabo_version
        lsb_release_info = _get_lsb_release_info()
        dist_name = lsb_release_info["LSB_RELEASE_DIST"]
        dist_ver = lsb_release_info["LSB_RELEASE_VERSION"]
        arch_name = os.uname().machine
        device = args.device
        tag = args.tag
        url = f"{args.repo}/{device}/tags/{tag}/"
        files = list_repo(url)
        if len(files) == 0:
            print(f"Binary for {device}-{tag} unavailable for download.")
            return None
        # first attempt to download a self extracting binary
        # that depends on the karabo framework version:
        # e.g. karabo Devices or karabo dependent libraries
        filename = (
            f"{device}-{tag}-{karabo_tag}-"
            f"{dist_name}-{dist_ver}-{arch_name}-"
            f"{args.config}.sh")
        if filename in files:
            dest = os.path.join('installed', args.device, filename)
            return _download(f"{url}{filename}", dest)
        # first attempt to download a self extracting binary
        # that depends on the Distribution:
        # e.g. DOOCS and other Libraries depending on libldap
        # since the library location is distribution dependent
        filename = (
            f"{device}-{tag}-"
            f"{dist_name}-{dist_ver}-{arch_name}.sh")
        if filename in files:
            dest = os.path.join('installed', args.device, filename)
            return _download(f"{url}{filename}", dest)
        # if this fails, attempt to download a self extracting
        # binary that is independent from the karabo framework
        # version: e.g. external libraries (hdf5, EPICS)
        # that is compatible with the libc version of the system
        glibc_major, glibc_minor = platform.libc_ver()[1].split(".")
        glibc_minor = int(glibc_minor)
        pattern = (f"{device}-{tag}-manylinux_"
                   f"{glibc_major}_([0-9]+)-{arch_name}.sh")
        minors = []
        for filename in files:
            groups = re.search(pattern, filename)
            if not groups:
                continue
            minor = int(groups[1])
            if minor > glibc_minor:
                continue
            minors.append(minor)
        if len(minors) == 0:
            # if this fails too, the binaries might just be missing
            print(f"Binary for {device}-{tag} unavailable for download.")
            return None
        minor = max(minors)
        filename = (f"{device}-{tag}-manylinux_"
                    f"{glibc_major}_{minor}-{arch_name}.sh")
        dest = os.path.join('installed', args.device, filename)
        return _download(f"{url}{filename}", dest)


def _download(url, dest):
    """Downloads a file from `url` and saves it as `dest`

    :return: None on failure and `dest` on success"""
    cmd = f"curl {url} -f -o {dest}"
    try:
        subprocess.check_output(cmd, shell=True, stderr=subprocess.DEVNULL)
        return dest
    except subprocess.CalledProcessError:
        return None


def list_repo(url):
    """returns the files contained in a folder at the URL

    assumes the repository webserver will serve the list of files
    in the usual apache standard page

    :return: a list of files without the URL prefix.
             an empty list is returned for empty folders or errors
    """
    cmd = (f"curl {url} | grep href "
           "| sed 's/.*href=\"//' | sed 's/\".*//' | grep '\\.sh$'")
    try:
        out = subprocess.check_output(
            cmd, shell=True, stderr=subprocess.DEVNULL)
        return out.decode().split('\n')[:-1]
    except subprocess.CalledProcessError:
        print(f"URL '{url}' not found")
        return []


def install(args):
    """Install a karabo plugin or package.

    it will recursively call itself to install dependencies"""
    with pushd_popd():
        copyFlag = args.copy
        path = os.path.join('installed', args.device.split('/')[-1])
        if not clean_dir(path, args):
            return
        os.makedirs(path, exist_ok=True)
        print(f'Downloading source for {args.device}... ',
              end='', flush=True)
        run_cmd(f"git clone {args.git}{resolve_project(args.device)}.git"
                f" --depth 1 -b {args.tag} --single-branch {path}")
        # remove tags folder. this avoids that a commit tagged with multiple
        # tags will generate a device with a tag other than `args.tag`.
        tag_dir = os.path.join(path, ".git", "refs", "tags")
        for tag_file in os.listdir(tag_dir):
            if os.path.basename(tag_file) == args.tag:
                # The requested tag is not part of tags folder.
                # This should not happen
                print(f"unexpected git configuration. {args.tag}"
                      " present in .git/refs/tag folder")
                break
            os.remove(os.path.join(tag_dir, tag_file))
        print('done.')
        os.chdir(path)
        install_dependencies(args)
        if os.path.exists('Makefile'):
            # download needs to happen after the processing of DEPENDS
            tgt = os.path.join(os.environ['KARABO'], "plugins")
            package = download(args)
            if package:
                # NOTE: for backwards compatibility the $KARABO/plugins
                # directory is passed to the prefix option.
                # If the package does not need to be installed the plugins
                # folder, the burden of doing the "right" thing falls
                # on the downloaded binary script, e.g. ignore the
                # prefix option.
                print('Installing {} binaries, please wait...'
                      ''.format(args.device), end='', flush=True)
                cmd = os.path.join(os.environ['KARABO'], package)
                run_cmd(f'bash {cmd} --prefix={tgt} ')
                print('done.')
            else:
                # some dependencies might still use this env. var.
                # to download a binary if the naming scheme
                # is different than the one expected by the `download` function
                os.environ['KARABO_BINARY_REPOSITORY'] = args.repo
                print(f'Compiling {args.device}, please wait... ',
                      end='', flush=True)
                run_cmd(f'make CONF={args.config} -j{args.jobs}')
                print('done.')
                if os.path.isdir('dist') and str2bool(copyFlag):
                    src = os.path.join('dist', args.config, '*', '*.so')
                    run_cmd(f'cp -f {src} {tgt}')
        elif os.path.exists('setup.py') or os.path.exists('pyproject.toml'):
            run_cmd('pip install --upgrade .')
        else:
            print('package {} has no clear'
                  ' installation path'.format(args.device))
        print("Installation succeeded.")


def str2bool(v):
    return v.lower() in ("yes", "true", "t", "1")


def clean_dir(path, args):
    """
    Removes a path

    :return: True if path has been removed, False otherwise
    """
    if os.path.isdir(path):
        tag = run_cmd(f'cd {path}; git tag').decode("utf-8").rstrip()
        if not hasattr(args, 'tag'):
            # we are called by develop, the depth is more than 1
            tag = run_cmd('cd {}; git rev-parse --abbrev-ref HEAD'
                          ''.format(path)).decode("utf-8").rstrip()
            new_tag = args.branch
        else:
            new_tag = args.tag
        sha1 = run_cmd(f'cd {path}; git show -s --format=%H').\
            decode("utf-8").rstrip()

        def check_tag():
            """
            Get SHA1 of new_tag from remote

            Note: "real" tags are marked with a trailing '^{}',
                   eg "refs/tags/3.1.1^{}"
            """
            new_sha1 = ""
            # protect against hand-crafted credentials,
            # which might raise an interactive prompt at this point
            # if we do not have access to git under those credentials.
            remote_uri = run_cmd('cd {}; git ls-remote --get-url '
                                 'origin'.format(path)).decode("utf-8")
            expected_url = f"{args.git}{resolve_project(args.device)}.git"
            if args.no_clobber and expected_url.strip() != remote_uri.strip():
                print(f"Device at {path} has a different remote than "
                      "the one expected: abort! ")
                sys.exit(1)

            cmd = 'cd {}; git ls-remote origin {tag} {tag}^{{}}'
            remote_refs = run_cmd(cmd.format(path,
                                  tag=new_tag)).decode("utf-8")

            for line in remote_refs.splitlines():
                # A line for a "real" tag will look like:
                # "91c91b5ed3ad86869f9c05e15692966579fcaa90\trefs/tags/3.1.1^{}"  # noqa
                if new_tag+'^{}' in line:
                    new_sha1 = line.split('\t')[0]
                    break  # Tag found -> break
                elif new_tag in line:
                    new_sha1 = line.split('\t')[0]
            return new_sha1

        if args.no_clobber:  # abort if different version installed
            if tag == '':  # not a tag
                print('{}-{} already installed: abort!'
                      ''.format(args.device, sha1))
                sys.exit(1)
            elif sha1 != check_tag():
                print('{}-{} already installed: abort!'
                      ''.format(args.device, tag))
                sys.exit(1)
            else:  # same version -> skip
                print('{}-{} already installed: skipping'
                      ''.format(args.device, tag))
                return False
        elif args.force:  # always overwrite
            run_cmd(f'rm -rf {path}')
        elif sha1 == check_tag():  # tag already installed
            # TODO: add integrity check (git status should be ok)
            print('{}-{} already installed: skipping'
                  ''.format(args.device, tag))
            return False
        else:  # interactive
            # Prompt for user's confirmation
            if tag != '':
                ver = tag
            else:  # not a tag
                ver = sha1
            overwrite = input(
                '{}-{} already installed: do you want to '
                'replace it with {}-{}? [y/N]'.format(
                    args.device, ver, args.device, new_tag))

            if overwrite.lower() != "y":
                print(f'Abort {args.device} installation')
                sys.exit(1)
            run_cmd(f'rm -rf {path}')

    return True


def parse_configuration_file(filename):
    devices = []
    with open(filename) as csvfile:
        rows = csv.reader(csvfile, delimiter=',')
        for row in rows:
            if len(row) < 2 or len(row) > 3 or '#' in row[0]:
                continue
            row[0] = row[0].strip()
            row[1] = row[1].strip()
            if len(row) == 2:
                row.append('False')
            else:
                row[2] = row[2].strip()
            devices.append(row)
    return devices


def list_devices(args):
    APIS = [
        ('Bound Devices', 'karabo.bound_device'),
        ('Middlelayer Devices', 'karabo.middlelayer_device'),
        ('C++ Devices', 'cpp')
    ]
    collected = {}
    for name, api in APIS:
        if api == 'cpp':
            # C++ is 'special'
            cpp_dir = os.path.join(os.environ['KARABO'], 'plugins')
            for path in glob.glob(os.path.join(cpp_dir, '*.so')):
                path = path[len(cpp_dir) + 1:]
                collected.setdefault(name, []).append(path)
        else:
            # Python devices
            for ep in entry_points(group=api):
                if args.internal or ep.dist.name != 'karabo':
                    collected.setdefault(name, []).append(ep)

    def _show_py_devices(name, entries):
        # Three columns with left, center, and right alignment
        TEMPLATE = '{:<26}{:^26}{:>26}'
        print(f'{name}:')
        print(TEMPLATE.format('Class', 'Package', 'Version'))
        print('=' * 79)
        for ep in entries:
            print(TEMPLATE.format(ep.name, ep.dist.name,
                                  ep.dist.version))
        print()

    def _show_py_machine(entries):
        print('\n'.join(ep.name for ep in entries))

    def _show_cpp_devices(name, entries):
        print(name + ':\n' + '=' * 79)
        for ent in entries:
            print(ent)
        print()

    def _show_cpp_machine(entries):
        print('\n'.join(entries))

    for name, api in APIS:
        entries = collected.get(name, [])
        if not entries:
            continue  # No devices of this type were found
        if api == 'cpp':
            if args.machine:
                _show_cpp_machine(entries)
            else:
                _show_cpp_devices(name, entries)
        else:
            if args.machine:
                _show_py_machine(entries)
            else:
                _show_py_devices(name, entries)


def uninstall(args):
    path = os.path.join('installed', args.device)
    if os.path.isdir(path):
        run_cmd(f'rm -rf {path}')
    so_path = f'plugins/lib{args.device}.so'
    if os.path.isfile(so_path):
        if not os.path.islink(so_path):
            ret = run_cmd(f'rm {so_path}')
        else:
            print('Failed to uninstall {}. {} is not a regular file.'.
                  format(args.device, so_path))
            sys.exit(1)
    else:
        # -q (quiet) to suppress sucess output - that is returned by run_cmd
        ret = run_cmd(f'pip uninstall -q -y {args.device}')

    # run_cmd returns output - empty output means success for 'rm' and 'pip'
    if not ret:
        print(f'{args.device} was successfully uninstalled')
    else:
        print('Problems uninstalling {}, see "karabo -h" for potential '
              'reasons.'.format(args.device))
        sys.exit(1)


def develop(args):
    with pushd_popd():
        path = os.path.join('devices', args.device)
        if not clean_dir(path, args):
            return
        checkout(args)
        os.chdir(path)
        install_dependencies(args, True)
        if os.path.exists('Makefile'):
            print(f'Compiling {args.device}, please wait... ',
                  end='', flush=True)
            run_cmd(f'make CONF={args.config} -j{args.jobs}')
            print('done.')
            os.chdir(os.path.join('..', '..', 'plugins'))
            lib = os.path.join('..', path, 'dist', args.config, '*', '*.so')
            run_cmd(f'ln -sf {lib}')
        elif os.path.exists('setup.py') or os.path.exists('pyproject.toml'):
            run_cmd('pip install -e .')
        else:
            print('package {} has no clear'
                  ' installation path'.format(args.device))
        print("Develop installation succeeded.")


def install_dependencies(args, is_develop=False):
    """Installs dependencies as specified in the DEPENDS file.
    NOTE: This function must be run in the directory of the DEPENDS file!
    """
    if not os.path.exists('DEPENDS'):
        return
    devices = parse_configuration_file('DEPENDS')
    dep_names = ', '.join(f'{e[0]} ({e[1]})' for e in devices)
    print('Found dependencies:', dep_names)
    if args.config == 'Simulation':
        print('Skipped automatic installation as building in Simulation '
              'configuration. Consider manual installation if needed.')
        return
    print('Automatically installing now.')
    for item in devices:
        args_copy = copy.deepcopy(args)
        args_copy.device = item[0]
        args_copy.copy = item[2]
        if is_develop:
            args_copy.branch = item[1]
            develop(args_copy)
        else:
            args_copy.tag = item[1]
            install(args_copy)


def undevelop(args):
    so_path = f'plugins/lib{args.device}.so'
    if os.path.islink(so_path):
        run_cmd(f'rm -f {so_path}')
    elif os.path.exists(so_path):
        print('Failed to undo development mode installation for {}. {} is '
              'not a link.'.format(args.device, so_path))
        sys.exit(1)
    else:
        run_cmd(f'pip uninstall -y {args.device}')
    print(f'Development mode installation for {args.device} undone')


def main():
    run_local()
    parse_commandline()


if __name__ == "__main__":
    main()
