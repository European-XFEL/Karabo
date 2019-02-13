from contextlib import contextmanager
from lxml import etree
import os
from pathlib import Path
import pkg_resources
from PyQt4.QtCore import QProcess
import re
import requests


class ExtensionsUpdater:

    _TAG_REGEX = '^\d+\.\d+\.\d+$'
    _PKG_NAME = 'GUI_Extensions'
    _WHEEL_TEMPLATE = 'GUI_Extensions-{}-py3-none-any.whl'
    _REMOTE_SVR = 'http://exflserv05.desy.de/karabo/karaboExtensions/tags/'

    UNDEFINED_VERSION = 'Undefined'

    def __init__(self, parent=None):
        self._parent = parent

    def get_current_version(self):
        """Gets the current version of the package"""
        try:
            package = pkg_resources.get_distribution('GUI-Extensions')
            return package.version
        except pkg_resources.DistributionNotFound:
            return self.UNDEFINED_VERSION

    def _retrieve_remote_html(self):
        result = requests.get(self._REMOTE_SVR)
        html = result.content.decode()

        return html

    def get_latest_version(self):
        """Gets the latest version of the package"""
        html = self._retrieve_remote_html()

        if not html:
            return self.UNDEFINED_VERSION

        table = etree.HTML(html).find('body/table')
        if not table:
            return self.UNDEFINED_VERSION

        # Match entries of the type N.N.N
        tag_regex = re.compile(self._TAG_REGEX)

        for href in reversed(table.xpath('//a/@href')):
            tag = href.strip('/')
            if tag_regex.match(tag):
                return tag

        return self.UNDEFINED_VERSION

    @contextmanager
    def _retrieve_wheel_file(self, tag):
        wheel_file = self._WHEEL_TEMPLATE.format(tag)
        wheel_path = '{}{}/{}'.format(self._REMOTE_SVR,
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
            os.remove(temp_file)

    def update_to_latest(self, tag):
        """Updates the package to the latest tag"""
        with self._retrieve_wheel_file(tag) as temp_file:
            if not temp_file:
                return

            # Install it using a system call
            process = QProcess(self._parent)
            process.start('pip install --upgrade {}'.format(temp_file))
            process.waitForFinished()
