from contextlib import contextmanager
from lxml import etree
from pathlib import Path
import pkg_resources
import re
import requests

_TAG_REGEX = '^\d+\.\d+\.\d+$'
_PKG_NAME = 'GUI_Extensions'
_WHEEL_TEMPLATE = 'GUI_Extensions-{}-py3-none-any.whl'
_REMOTE_SVR = 'http://exflserv05.desy.de/karabo/karaboExtensions/tags/'

UNDEFINED_VERSION = 'Undefined'


def get_current_version():
    """Gets the current version of the package"""
    try:
        package = pkg_resources.get_distribution('GUI-Extensions')
        return package.version
    except pkg_resources.DistributionNotFound:
        return UNDEFINED_VERSION


def _retrieve_remote_html():
    """Retrieves the remote tag url html and decodes it"""
    result = requests.get(_REMOTE_SVR)
    html = result.content.decode()

    return html


def get_latest_version():
    """Gets the latest version of the package"""
    html = _retrieve_remote_html()

    if not html:
        return UNDEFINED_VERSION

    table = etree.HTML(html).find('body/table')
    if table is not None:
        return UNDEFINED_VERSION

    # Match entries of the type N.N.N
    tag_regex = re.compile(_TAG_REGEX)

    # All the tags are in an href tag
    for href in reversed(table.xpath('//a/@href')):
        tag = href.strip('/')
        if tag_regex.match(tag):
            return tag

    return UNDEFINED_VERSION


@contextmanager
def download_file_for_tag(tag):
    """Downloads the wheel for the given tag and writes it to a file,
    removing it when the context is finished."""
    wheel_file = _WHEEL_TEMPLATE.format(tag)
    wheel_path = '{}{}/{}'.format(_REMOTE_SVR,
                                  tag,
                                  wheel_file)

    with requests.get(wheel_path, stream=True) as wheel_request:
        if not wheel_request.status_code == requests.codes.ok:
            yield None
            return

        temp_file = Path(wheel_file)
        temp_file.write_bytes(wheel_request.content)

        yield temp_file

        # Remove downloaded wheel
        temp_file.unlink()
