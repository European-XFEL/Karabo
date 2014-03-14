from PyQt4.QtGui import QIcon
import os.path

class Icon(object):
    """A lazy-loading class for QIcons

    QIcons can only be read once a QApplication is created. This
    class assures that icons are only loaded when needed."""
    def __init__(self, name):
        self.name = name
        self.icon = None


    def __get__(self, instance, owner):
        if self.icon is None:
            self.icon = QIcon(os.path.join(os.path.dirname(__file__),
                                           self.name))
        return self.icon


def init():
    """This function actually loads the hitherto lazily loaded icons."""
    d = globals()
    for k, v in d.iteritems():
        if isinstance(v, Icon):
            d[k] = v.__get__(None, None)


entireWindow = Icon("entire.svg")
selectAll = Icon("select-all.svg")
text = Icon("text-32x32.png")
line = Icon("line-32x32.png")
rect = Icon("rect-32x32.png")
group = Icon("group-32x32.png")
groupGrid = Icon("group-grid.svg")
groupVertical = Icon("group-vertical.svg")
groupHorizontal = Icon("group-horizontal.svg")
ungroup = Icon("ungroup-32x32.png")
editCut = Icon("edit-cut-32x32.png")
editCopy = Icon("edit-copy-32x32.png")
editPaste = Icon("edit-paste-32x32.png")
bringToFront = Icon("bring-to-front1-32x32.png")
sendToBack = Icon("bring-to-back1-32x32.png")
lock = Icon("lock-32x32.png")
remote = Icon("remote.png")
exit = Icon("exit-32x32.png")
delete = Icon("edit-remove.svg")
