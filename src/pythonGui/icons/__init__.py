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

    global applyGrey
    applyGrey = QIcon(apply.pixmap(32, QIcon.Disabled, QIcon.On))


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
delete = Icon("edit-remove-32x32.png")
enum = Icon("type-list.png")
string = Icon("type-string.png")
path = Icon("type-string.png")
int = Icon("type-integer.png")
float = Icon("type-float.png")
boolean = Icon("type-boolean.png")
undefined = Icon("type-undefined.png")
apply = Icon("apply-32x32.png")
applyGrey = apply
applyConflict = Icon("apply-conflict-32x32.png")
yes = Icon("yes.png")
no = Icon("no.png")
host = Icon("computer-32x32.png")
deviceClass = Icon("device-class.png")
deviceInstance = Icon("device-instance.png")
deviceInstanceError = Icon("device-instance-error.png")
deviceOffline = Icon("offline-32x32.png")
deviceOfflineNoServer = Icon("offline-no-server.png")
deviceOfflineNoPlugin = Icon("offline-no-plugin.png")
signal = Icon("type-schema.png")
slot = Icon("type-pair.png")
image = Icon("image-gray-32x32.png")
folder = Icon("folder-32x32.png")
revert = Icon("edit-undo.png")
open = Icon("fileopen-32x32.png")
saveAs = Icon("filesaveas-32x32.png")
file = Icon("file.svg")
device_dead = Icon("device-dead.svg")
device_requested = Icon("device-requested.svg")
device_schema = Icon("device-schema.svg")
