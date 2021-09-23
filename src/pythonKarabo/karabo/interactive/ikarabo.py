from IPython import start_ipython
from traitlets.config.loader import Config

SCRIPT = """
# This provides the namespace
from karabo.middlelayer.cli import *
from karabo.macros.cli import *

# This initializes everything
from karabo.middlelayer_api import ikarabo
ikarabo.start_ikarabo()
del ikarabo
"""


def main():
    c = Config()
    c.TerminalIPythonApp.display_banner = False
    c.InteractiveShellApp.exec_lines = SCRIPT.split("\n")
    start_ipython(config=c)


if __name__ == '__main__':
    main()
