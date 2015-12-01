from functools import partial
import os.path as op
import re
import subprocess


class SvnParseError(ValueError):
    pass


def _get_development_version(path):
    """ Return the current development version.
    """
    commit_count = '0'
    generators = [partial(f, path) for f in (svn_version, git_version)]
    for gen in generators:
        vcs_version, commit_count = gen()
        if vcs_version == 'Unknown':
            continue

    return commit_count


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


def get_karabo_version():
    """ Return the current Karabo version.

    The karabo version is generated at package build time using functions from
    this module. Therefore, we must import it from inside this function.
    """
    from karabo._version import version

    return version


def get_package_version(path):
    """ Read the SVN or Git revision for a directory.

    Returns a PEP 440 compatible version string.
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
        cmd = 'git describe --tags'.split(' ')
        out = subprocess.check_output(cmd, cwd=path).decode('ascii')
    except subprocess.CalledProcessError:
        out = b''

    expr = r'.*?\-(?P<count>\d+)-g(?P<hash>[a-fA-F0-9]+)'
    match = re.match(expr, out)
    if match is None:
        git_revision, git_count = 'Unknown', '0'
    else:
        git_revision, git_count = match.group('hash'), match.group('count')

    return git_revision, git_count


def svn_version(path):
    """ Return the SVN revision as a string and a commit count.
    """
    cmd = ['svn', 'info', path]
    try:
        out = subprocess.check_output(cmd).decode('ascii')
        svn_branch = _parse_svn_part(out, 'branch')
        svn_count = _parse_svn_part(out, 'revision')
    except (subprocess.CalledProcessError, SvnParseError):
        return 'Unknown', '0'

    return '{}@r{}'.format(svn_branch, svn_count), svn_count
