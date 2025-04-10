from functools import partial
from os import environ
import os.path as op
import re
import subprocess


class SvnParseError(ValueError):
    pass


def _get_development_version(path):
    """ Return the current development version.
    """
    commit_count = '0'
    generators = [partial(f, path)
                  for f in (svn_version, jsvn_version, git_version)]
    for gen in generators:
        vcs_version, commit_count = gen()
        if vcs_version != 'Unknown':
            break

    return commit_count


def _get_karabo_framework_path():
    """ Get the filesystem location of the running Karabo framework.
    """
    karabo = environ.get('KARABO')
    if karabo is None:
        msg = ("$KARABO env variable not defined! Source the activate script "
               "for the desired Karabo framework to address this issue.")
        raise RuntimeError(msg)

    return karabo


def _parse_svn_part(info_str, part_name):
    """ Extract various information from the output of `svn info`

    part_name = 'revision': Gives the current SVN revision number
    part_name = 'branch': Gives the SVN branch name
    """
    svn_parts = {
        'revision': r'Revision\: (\d+)',
        'branch': r'URL\: .+/(.+)',
    }
    match = re.search(svn_parts[part_name], info_str)
    if match is not None:
        return match.groups()[0]
    raise SvnParseError('SVN {} not found!'.format(part_name))


def get_karabo_framework_version():
    """ Return the current Karabo version contained in the VERSION file.
    """
    karabo_path = _get_karabo_framework_path()
    version_path = op.join(karabo_path, 'VERSION')

    with open(version_path, 'rt') as fp:
        return fp.read().strip()


def get_karabo_version():
    """ Return the current Karabo Python package version.
    """
    from karabo._version import full_version
    return full_version


def get_package_version(path):
    """ Get the package version for a device's repository.

    Returns a PEP 440 compatible version string which is a combination of the
    active Karabo framework's version and the commit count of the device's
    repository.
    """
    path = op.normpath(path)
    karabo_version = get_karabo_version()
    dev_version = _get_development_version(path)
    extra = '.dev{}'.format(dev_version) if dev_version != '0' else ''

    return karabo_version + extra


def git_version(path):
    """ Return the git revision as a string and a commit count
    """
    try:
        cmd = 'git describe'.split(' ')
        out = subprocess.check_output(cmd, cwd=path).decode('ascii')
    except (subprocess.CalledProcessError, OSError):
        out = ''

    expr = r'.*?\-(?P<count>\d+)-g(?P<hash>[a-fA-F0-9]+)'
    match = re.match(expr, out)
    if match is None:
        git_revision, git_count = 'Unknown', '0'
    else:
        git_revision, git_count = match.group('hash'), match.group('count')

    return git_revision, git_count


def svn_version(path, svn_cmd='svn'):
    """ Return the SVN revision as a string and a commit count.
    """
    cmd = [svn_cmd, 'info', path]
    try:
        out = subprocess.check_output(cmd).decode('ascii')
        svn_branch = _parse_svn_part(out, 'branch')
        svn_count = _parse_svn_part(out, 'revision')
    except (subprocess.CalledProcessError, OSError, SvnParseError):
        return 'Unknown', '0'

    return '{}@r{}'.format(svn_branch, svn_count), svn_count


# Define this where it can be imported.
jsvn_version = partial(svn_version, svn_cmd='jsvn')
