import os

from conans import ConanFile
from conans.tools import collect_libs, untargz
from conan.tools.cmake import CMake, CMakeToolchain


class KaraboOpenMQConan(ConanFile):
    name = "openmqc"
    version = "5.1.4.1"
    description = "Karabo extern install of OpenMQ client"
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeToolchain", "CMakeDeps"
    exports = ["openmqc-5.1.4.1.tar", "CMakeLists.txt"]

    requires = [ "nss/3.72" ]

    def source(self):
        zip_name = "openmqc-%s.tar" % self.version
        untargz(zip_name, strip_root=True)
        os.unlink(zip_name)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.libs = collect_libs(self)

        # packages that link against openmqc should also link
        # against same nss/nspr libraries as openmqc
        self.cpp_info.requires = [
            "nss::nss",
        ]
