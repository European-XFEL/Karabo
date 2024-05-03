from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.files import download, rm, unzip


class KaraboDaemonToolsConan(ConanFile):
    name = "daemontools-encore"
    version = "1.11-karabo3"
    description = "Karabo specific fork of daemontools"
    settings = "os", "arch", "compiler", "build_type"
    url = "https://github.com/tecki/daemontools-encore/"
    license = "https://github.com/tecki/daemontools-encore/blob/master/LICENSE"
    generators = "CMakeToolchain", "CMakeDeps"
    exports_sources = "CMakeLists.txt"

    def source(self):
        zip_name = "daemontools-encore-%s.zip" % self.version
        url = f"https://github.com/tecki/daemontools-encore/archive/refs/tags/{self.version}.zip"
        download(self, url, zip_name)
        unzip(self, zip_name, strip_root=True)
        rm(self, zip_name, self.source_folder)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.bindirs = ["bin"]
