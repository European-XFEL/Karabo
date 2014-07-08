from distutils.core import setup

import os


def getVersion():
    try:
        with open("VERSION", 'r') as file:
            return file.readline().rstrip()
    except IOError:
        return ""


setup(name="KaraboGUI",
    version=getVersion(),
    description="The GUI of the Karabo framework",
    author="Kerstin Weger, Burkhard Heisen, Martin Teichmann",
    packages=["suds", "karabo", "karaboGui", "karaboGui.dialogs",
              "karaboGui.icons", "karaboGui.treewidgetitems",
              "karaboGui.displaywidgets", "karaboGui.editablewidgets",
              "karaboGui.panels", "karaboGui.vacuumwidgets",
              "suds.xsd", "suds.umx", "suds.sax", "suds.bindings",
              "suds.mx", "suds.transport"],
    package_dir=dict(suds="../../extern/resources/suds/suds-jurko-0.5/suds",
                     karabo="../../src/pythonKarabo/karabo_package",
                     karaboGui="../../src/pythonGui"),
    scripts=['scripts/win_post_install.py'],
    package_data = {"karaboGui.icons": ["*.*"],
                    "karaboGui.displaywidgets": ["*.ui"],
                    "karaboGui.dialogs": ["*.ui"]},
)


