import sys
sys.karabo_gui = True
import gui

sys.excepthook = gui.excepthook
loop = gui.init(sys.argv)
sys.exit(loop.run_forever())
