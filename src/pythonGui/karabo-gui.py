import gui
import sys

if __name__ == "__main__":
    sys.excepthook = gui.excepthook
    app = gui.init(sys.argv)
    sys.exit(app.exec_())
