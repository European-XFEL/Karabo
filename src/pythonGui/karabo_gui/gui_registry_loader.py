import importlib


def load_widgets():
    PACKAGES = ('karabo_gui.displaywidgets', 'karabo_gui.editablewidgets')
    for pkg in PACKAGES:
        module = importlib.import_module(pkg)
        for submodule in module.__all__:
            importlib.import_module('{}.{}'.format(pkg, submodule))


# XXX: This is crazy. It's being done for its side effects only.
# The purpose is to initialize the registry of classes. This needs
# to be done very early, so this module is imported from gui.init()
load_widgets()
del load_widgets
