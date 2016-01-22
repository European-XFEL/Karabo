from PyQt4.QtGui import QDialog, QFileDialog

import weakref


class Weak(object):
    """ this declares a member variable of a class to be weak

    use as follows:

        class Spam(object):
            ham = Weak()

            def __init__(self, ham):
                self.ham = ham # use like a normal variable

    to define a weak variable ham.  """

    def __get__(self, instance, owner):
        return instance.__dict__[self]()


    def __set__(self, instance, value):
        instance.__dict__[self] = weakref.ref(value)


    def __delete__(self, instance):
        del instance.__dict__[self]


class SignalBlocker(object):
    """ Block signals from a QWidget in a with statement """
    def __init__(self, object):
        self.object = object


    def __enter__(self):
        self.state = self.object.blockSignals(True)


    def __exit__(self, a, b, c):
        self.object.blockSignals(self.state)


def getSaveFileName(title, dir="", description="", suffix="", filter=None, selectFile=""):
    dialog = QFileDialog(None, title, dir, description)
    dialog.selectFile(selectFile)
    dialog.setDefaultSuffix(suffix)
    dialog.setFileMode(QFileDialog.AnyFile)
    dialog.setAcceptMode(QFileDialog.AcceptSave)
    if filter is not None:
        dialog.setNameFilter(filter)

    if dialog.exec_() == QDialog.Rejected:
        return
    if len(dialog.selectedFiles()) == 1:
        return dialog.selectedFiles()[0]
