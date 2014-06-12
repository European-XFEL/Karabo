from distutils.core import setup

import os

setup(name="KaraboGUI",
    version="1.2",
    description="The GUI of the Karabo framework",
    author="Kerstin Weger, Burkhard Heisen, Martin Teichmann",
    packages=["suds", "karabo", "karaboGui", "karaboGui.dialogs",
              "karaboGui.icons", "karaboGui.treewidgetitems",
              "karaboGui.displaywidgets", "karaboGui.editablewidgets",
              "karaboGui.panels", "karaboGui.vacuumwidgets",
              "suds.xsd", "suds.umx", "suds.sax", "suds.bindings",
              "suds.mx", "suds.transport"],
    package_dir=dict(suds="../../extern/resources/suds/suds-0.4.1/suds",
                     karabo="../../src/pythonKarabo/karabo_package",
                     karaboGui="../../src/pythonGui"),
    scripts=['scripts/win_post_install.py'],
    package_data = {"karaboGui.icons": ["*.*"],
                    "karaboGui.displaywidgets": ["*.ui"],
                    "karaboGui.dialogs": ["*.ui"]},
)
