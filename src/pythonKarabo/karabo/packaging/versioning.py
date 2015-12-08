from functools import partial
import os.path as op
import re
import subprocess


class OutputParseError(ValueError):
    pass


def _get_development_version(path):
    """ Return the current development
    """
    checkers = [partial(f, path)
                for f in (_svn_dev_revision, _git_dev_revision)]
    for check in checkers:
        try:
            return check()
        except (subprocess.CalledProcessError, OutputParseError):
            continue

    return ''


def _git_dev_revision(path):
    """ Return the git commit count for the current branch on the given path.
    """
    cmd = 'git rev-list --count HEAD'.split(' ')
    count = subprocess.check_output(cmd, cwd=path).decode('utf8')
    return 'dev' + count.strip()


def _parse_svn_part(info_str, part_name):
    """ Extract various information from the output of `svn info`

    part_name = 'revision': Gives the current SVN revision number
    part_name = 'branch': Gives the SVN branch name
    """
    SVN_PARTS = {
        'revision': r'Revision\: (\d+)',
        'branch': r'URL\: .+/(.+)',
    }
    match = re.search(SVN_PARTS[part_name], info_str)
    if match is not None:
        return match.groups()[0]
    raise OutputParseError('SVN {} not found!'.format(part_name))


def _svn_dev_revision(path):
    """ Determine version info in an SVN repository.
    """
    cmd = ['svn', 'info', path]
    output = subprocess.check_output(cmd).decode('utf8')
    version = 'dev' + _parse_svn_part(output, 'revision')

    return version


def get_karabo_version():
    """ Return the current Karabo version.
    """
    # FIXME: Add version generation to Karabo's setup.py, then import the
    # version number here.
    return '1.5.0'


def get_package_version(path):
    """ Read the SVN or Git revision for a directory.

    Returns a PEP 440 compatible version string.
    """
    path = op.normpath(path)
    karb_version = get_karabo_version()
    dev_version = _get_development_version(path)

    return karb_version + ('.{}'.format(dev_version) if dev_version else '')
