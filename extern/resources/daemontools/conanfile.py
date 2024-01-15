import os

from conans import ConanFile
from conans.tools import download, unzip
from conan.tools.cmake import CMake, CMakeToolchain


class KaraboDaemonToolsConan(ConanFile):
    name = "daemontools-encore"
    version = "1.11-karabo3"
    description = "Karabo specific fork of daemontools"
    settings = "os", "arch", "compiler", "build_type"
    url = "https://github.com/tecki/daemontools-encore/"
    license = "https://github.com/tecki/daemontools-encore/blob/master/LICENSE"
    generators = "CMakeToolchain", "CMakeDeps"
    exports = ["CMakeLists.txt"]

    def source(self):
        zip_name = "daemontools-encore-%s.zip" % self.version
        url = f"https://github.com/tecki/daemontools-encore/archive/refs/tags/{self.version}.zip"
        download(url, zip_name)
        unzip(zip_name, strip_root=True)
        os.unlink(zip_name)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.bindirs = ["bin"]
