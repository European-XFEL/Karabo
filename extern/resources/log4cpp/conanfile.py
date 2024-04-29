from conan import ConanFile
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import collect_libs, copy, download, rm, rmdir, unzip
from conan.tools.gnu import Autotools, AutotoolsToolchain


class Log4cppConan(ConanFile):
    name = "log4cpp"
    version = "1.1.3"
    description = "A library of C++ classes for flexible logging."
    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False]}
    default_options = {"shared": True}
    url="https://sourceforge.net/projects/log4cpp/"
    license="https://sourceforge.net/p/log4cpp/codegit/ci/master/tree/COPYING"
    exports = ["replace_namespace.sh", "include/*"]

    def source(self):
        zip_name = "log4cpp-%s.tar.gz" % self.version
        url = "https://sourceforge.net/projects/log4cpp/files/latest/download"
        download(self, url, zip_name)
        unzip(self, zip_name, strip_root=True)
        copy(self, "*", "log4cpp", ".")
        rmdir(self, "log4cpp")
        rm(self, zip_name, self.source_folder)

    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()

        tc = AutotoolsToolchain(self)
        tc.extra_defines.append("KRB_LOG4CPP_HAVE_SSTREAM")
        tc.configure_args.append("--disable-doxygen")
        tc.configure_args.append("--disable-html-docs")

        env = tc.environment()
        tc.generate()

    def build(self):
        autotools = Autotools(self)
        autotools.configure()
        self.run("bash replace_namespace.sh")
        autotools.make()

    def package(self):
        autotools = Autotools(self)
        autotools.install()

    def package_info(self):
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.libs = collect_libs(self)
