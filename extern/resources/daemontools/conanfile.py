import os

from conans import ConanFile
from conans.tools import download, unzip


class KaraboDaemonToolsConan(ConanFile):
    name = "daemontools-encore"
    version = "1.11-karabo3"
    description = "Karabo specific fork of daemontools"
    settings = "os", "arch", "compiler", "build_type"
    url = "https://github.com/tecki/daemontools-encore/"
    license="https://github.com/tecki/daemontools-encore/blob/master/LICENSE"

    def source(self):
        zip_name = "daemontools-encore-%s.zip" % self.version
        url = f"https://github.com/tecki/daemontools-encore/archive/refs/tags/{self.version}.zip"
        download(url, zip_name)
        unzip(zip_name, strip_root=True)
        os.unlink(zip_name)

    def build(self):
        self.run("bash makemake")
        self.run("make programs")
        self.run(f"mkdir bin; cp $(cut -d: -f6 BIN) bin")

    def package(self):
        self.copy("*", "bin", "bin")
