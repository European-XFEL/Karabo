import re

_BASE = '\d+\.\d+\.\d+'
_RELEASE = '(a|b|rc)\d+'
_DEV = '\.dev\d+'

_BASE_VERSION_REGEX = re.compile('^({})({})?'.format(_BASE, _RELEASE))
_FULL_VERSION_REGEX = re.compile('^({})({}({})?)?'.format(
    _BASE, _RELEASE, _DEV))


def _apply_regex(regex, version):
    result = regex.match(version)
    if result:
        return result.group()
    else:
        result = ''
    return result


def extract_base_version(version: str) -> str:
    """
    Extracts the base version of a setuptools_scm version string.
    """
    return _apply_regex(_BASE_VERSION_REGEX, version)


def extract_full_version(version: str) -> str:
    """
    Extracts the full version of a setuptools_scm version string.
    """
    return _apply_regex(_FULL_VERSION_REGEX, version)
