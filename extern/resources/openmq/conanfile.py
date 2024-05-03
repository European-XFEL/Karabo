from conan import ConanFile
from conan.tools.files import download, rm, unzip, patch


PATCH_CONTENT = \
"""
--- MessageQueue/mq/lib/props/broker/install.properties
+++ MessageQueue/mq/lib/props/broker/install.properties
@@ -25,3 +25,7 @@
 # Any properties changed at installation will be lost when the
 # product is installed or upgraded
 # 
+imq.autocreate.destination.maxTotalMsgBytes=-1
+imq.autocreate.destination.maxNumMsgs=-1
+imq.autocreate.destination.maxNumProducers=-1
+imq.autocreate.destination.maxBytesPerMsg=-1
"""


class KaraboOpenMQConan(ConanFile):
    name = "openmq"
    version = "5.1.3"
    description = "Karabo extern install of OpenMQ"
    settings = "os", "arch", "compiler", "build_type"
    exports_sources = "openmq5_1_3_linux.zip", "etc/*", "bin/*"

    def source(self):
        zip_name = "openmq%s_linux.zip" % self.version.replace('.','_')
        unzip(self, zip_name, strip_root=True)
        patch(self, patch_string=PATCH_CONTENT, strip=1)
        rm(self, zip_name, self.source_folder)

    def build(self):
        pass

    def package(self):
        self.copy("*", "mq", "mq")
        self.copy("*", "etc", "etc")
        self.copy("*", "bin", "bin")
        self.run(f"mkdir {self.package_folder}/MessageQueue")
        self.run(f"mv {self.package_folder}/etc/* {self.package_folder}/mq/etc")
        self.run(f"mv {self.package_folder}/mq {self.package_folder}/MessageQueue")
        self.run(f"chmod +x {self.package_folder}/MessageQueue/mq/bin/*")
        self.run(f"rmdir {self.package_folder}/etc")
