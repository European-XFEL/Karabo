import argparse
import subprocess
import sys
import os
import contextlib
import multiprocessing
import csv

from karabo.packaging.device_template import configure_template

DEV_NULL = open(os.devnull, 'w')
INVOKE_DIR = os.getcwd()


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
        global DEV_NULL
        output = subprocess.check_output(cmd, shell=True, stderr=DEV_NULL)
        return output
    except subprocess.CalledProcessError as e:
        print("Problem with system call: {}".format(e))
        exit()


@contextlib.contextmanager
def pushd_popd():
    path = os.environ['KARABO']
    old_path = os.getcwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(old_path)


def parse_commandline():
    parser = argparse.ArgumentParser(
        description=('Karabo Utility Script'),
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

    parser_chk = sps.add_parser('checkout',
                                help='Checks out a device (sources) from the'
                                     'repository')
    parser_chk.set_defaults(command=checkout)

    parser_chk.add_argument('device',
                            type=str,
                            help='The name of the device package')

    parser_chk.add_argument('-d', '--develop',
                            action='store_true',
                            help='Installs device in develop mode')

    parser_ins = sps.add_parser('install',
                                help='Installs an existing device')

    parser_ins.add_argument('device',
                            type=str,
                            help='The name of the device package')

    parser_ins.add_argument('tag',
                            type=str,
                            help='The tag to install')

    parser_ins.add_argument("-c", "--copy",
                            type=str, default='True',
                            help='artifacts will be copied into the plugins folder unless --copy=False')

    parser_ins.set_defaults(command=install)

    parser_uins = sps.add_parser('uninstall',
                                 help='Uninstalls an existing device')
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
                            help='The name of the device package')

    parser_udev = sps.add_parser('undevelop',
                                 help='Deactivates develop mode for a given'
                                      'device')
    parser_udev.set_defaults(command=undevelop)

    parser_udev.add_argument('device',
                             type=str,
                             help='The name of the device package')

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
                        default='',
                        help='URL to the binary repository, example '
                             'http://exflserv05.desy.de/karabo/'
                             'karaboDevices/')

    parser.add_argument('-j', '--jobs',
                        type=int,
                        default=multiprocessing.cpu_count(),
                        help='Specifies the number of make jobs (commands) '
                             'to run simultaneously.')

    args = parser.parse_args()
    if len(sys.argv) <= 1:
        parser.print_help()
        parser.exit()
    args.command(args)


def new(args):
    with pushd_popd():
        path = os.path.join('devices', args.device)
        class_name = args.device[0].capitalize() + args.device[1:]
        if os.path.isdir(path):
            if args.force:
                run_cmd('rm -rf {}'.format(path))
            else:
                print('Device {} already exists.'
                      .format(args.device))
                return
        run_cmd('mkdir -p {}'.format(path))
        tpath = os.path.join('templates', args.api, 'minimal')
        run_cmd('cp -rf {} {}'.format(os.path.join(tpath, '*'), path))
        run_cmd('cp -f {} {}'.format(os.path.join(tpath, '.gitignore'), path))
        email = os.environ.get('USER', 'Unknown')
        configure_template(path, args.device, class_name, email)
        os.chdir(path)
        run_cmd('git init')
        run_cmd('git remote add origin {}/karaboDevices/{}.git'
                .format(args.git, args.device))
        run_cmd('git add .')
        run_cmd('git commit -m "Initial commit"')
        print('New device package was added to: {}'
              .format(os.getcwd()))
        print('Use: "git push -u origin master" if you want start versioning '
              'remotely')


def checkout(args):
    with pushd_popd():
        path = os.path.join('devices', args.device)
        if os.path.isdir(path):
            print('INFO The device package already exists, skipped checkout')
        else:
            print('Downloading {}... '.format(args.device), end='', flush=True)
            run_cmd('git clone {}/karaboDevices/{}.git {}'.format(args.git,
                                                                  args.device,
                                                                  path))
            print('done.')
            print('Device package was added to: {}'
                  .format(os.path.abspath(path)))
    if hasattr(args, 'develop') and args.develop:
        develop(args)


def download(args):
    """
    attempts the download of a package

    :return: the path of the package or None in case of failure
    """
    if args.repo == '':
        return None
    with pushd_popd():
        karabo_tag = run_cmd('cat VERSION').decode("utf-8").rstrip()
        dist_name = run_cmd('lsb_release -is').decode("utf-8").rstrip()
        dist_ver = run_cmd('lsb_release -rs').decode("utf-8").split('.')[0].rstrip()
        arch_name = os.uname().machine
        filename = '{device}-{tag}-{ktag}-{dist_name}-' \
                   '{dist_ver}-{arch}-{config}.sh'.format(device=args.device,
                                                          tag=args.tag,
                                                          ktag=karabo_tag,
                                                          dist_name=dist_name,
                                                          dist_ver=dist_ver,
                                                          arch=arch_name,
                                                          config=args.config)
        dest = os.path.join('installed', args.device, filename)
        cmd = 'curl -f {repo}/{device}/tags/{tag}/{filename} ' \
              '-o {dest}'.format(device=args.device, tag=args.tag,
                                 repo=args.repo, filename=filename, dest=dest)
        try:
            global DEV_NULL
            subprocess.check_output(cmd, shell=True, stderr=DEV_NULL)
            return dest
        except subprocess.CalledProcessError as e:
            print("Problem dowloading {device}-{tag}: {e}"
                  "".format(device=args.device, tag=args.tag, e=e))
        return None


def install(args):
    with pushd_popd():
        copyFlag = args.copy
        path = os.path.join('installed', args.device)
        if os.path.isdir(path):
            run_cmd('rm -rf {}'.format(path))
        os.makedirs(path, exist_ok=True)
        print('Downloading source for {}... '.format(args.device), end='', flush=True)
        run_cmd('git clone {}/karaboDevices/{}.git --depth 1 -b {}\
                --single-branch {}'
                .format(args.git, args.device, args.tag, path))
        print('done.')
        os.chdir(path)
        if os.path.isfile('DEPENDS'):
            install_dependencies(args)
        if os.path.isfile('Makefile'):
            # download needs to happen after the processing of DEPENDS
            package = download(args)
            tgt = os.path.join(os.environ['KARABO'], 'plugins')
            if package:
                print('Installing binaries, please wait... ',
                      end='', flush=True)
                cmd = os.path.join(os.environ['KARABO'], package)
                run_cmd('bash {} --prefix={} '.format(cmd, tgt))
                print('done.')
            else:
                print('Compiling, please wait... ', end='', flush=True)
                run_cmd('make CONF={} -j{}'.format(args.config, args.jobs))
                print('done.')
                if os.path.isdir('dist') and str2bool(copyFlag):
                    src = os.path.join('dist', args.config, '*', '*.so')
                    run_cmd('cp -f {} {}'.format(src, tgt))
        else:
            run_cmd('pip install --upgrade .')
        print("Installation succeeded.")

def str2bool(v):
    return v.lower() in ("yes", "true", "t", "1")

def parse_configuration_file(filename):
    devices = []
    with open(filename, 'r') as csvfile:
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


def uninstall(args):
    so_path = 'plugins/lib{}.so'.format(args.device)
    if os.path.isfile(so_path):
        run_cmd('rm -f {}'.format(so_path))
    else:
        run_cmd('pip uninstall -y {}'.format(args.device))
    print('{} was successfully uninstalled'.format(args.device))


def develop(args):
    with pushd_popd():
        path = os.path.join('devices', args.device)
        if not os.path.isdir(path):
            checkout(args)
        os.chdir(path)
        if os.path.isfile('DEPENDS'):
            install_dependencies(args)
        if os.path.isfile('Makefile'):
            print('Compiling, please wait... ', end='', flush=True)
            run_cmd('make CONF={} -j{}'.format(args.config, args.jobs))
            print('done.')
            os.chdir(os.path.join('..', '..', 'plugins'))
            lib = os.path.join('..', path, 'dist', args.config, '*', '*.so')
            run_cmd('ln -sf {}'.format(lib))
        else:
            run_cmd('pip install -e .')
        print("Develop installation succeeded.")


def install_dependencies(args):
    """Installs dependencies as specified in the DEPENDS file.
    NOTE: This function must be run in the directory of the DEPENDS file!
    """
    devices = parse_configuration_file('DEPENDS')
    dep_names = ', '.join('{} ({})'.format(e[0], e[1]) for e in devices)
    print('Found dependencies:', dep_names)
    if args.config == 'Simulation':
        print('Skipped automatic installation as building in Simulation '
              'configuration. Consider manual installation if needed.')
        return
    print('Automatically installing now.')
    for item in devices:
        args.device = item[0]
        args.tag = item[1]
        args.copy = item[2]
        install(args)


def undevelop(args):
    uninstall(args)


def main():
    run_local()
    parse_commandline()


if __name__ == "__main__":
    main()
