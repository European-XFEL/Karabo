import os

from conans import ConanFile, AutoToolsBuildEnvironment
from conans.tools import download, untargz, patch, rmdir
from conan.tools.files import copy


class Log4cppConan(ConanFile):
    name = "log4cpp"
    version = "1.1.3"
    description = "A library of C++ classes for flexible logging."
    generators = "cmake"
    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False]}
    default_options = "shared=True"
    url="https://sourceforge.net/projects/log4cpp/"
    license="https://sourceforge.net/p/log4cpp/codegit/ci/master/tree/COPYING"
    exports = ["replace_namespace.sh", "include/*"]

    def source(self):
        zip_name = "log4cpp-%s.tar.gz" % self.version
        url = "https://sourceforge.net/projects/log4cpp/files/latest/download"
        download(url, zip_name)
        untargz(zip_name, strip_root=True)
        copy(self, "*", "log4cpp", ".")
        rmdir("log4cpp")
        os.unlink(zip_name)

    def build(self):
        autotools = AutoToolsBuildEnvironment(self)
        autotools.defines.append("KRB_LOG4CPP_HAVE_SSTREAM")
        autotools.configure(args=["--disable-doxygen", "--disable-html-docs"])
        self.run("bash replace_namespace.sh")
        autotools.make()
        autotools.install()

    def package_info(self):
        self.cpp_info.libs = ['log4cpp']
        if self.settings.os == "Linux":
            self.cpp_info.libs.append("pthread")
