import sys
sys.karabo_gui = True
import gui

sys.excepthook = gui.excepthook
app = gui.init(sys.argv)
sys.exit(app.exec_())
