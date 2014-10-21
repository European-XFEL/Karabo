How to write a widget
=====================

The macros in the project can also be used to write your own custom
widgets for the scene. There are two types of widgets: display
widgets, which only show values, and editable widgets, which can edit
a value. A widget may also be of both types at the same time, if it is
useful for displaying and editing.

A simple display widget
-----------------------

A widget is created by a factory class, which is not the widget
itself but creates the necessary widgets. It inherits from 
:class:`~widget.DisplayWidget`, which does all the necessary
registration so that the new widget will be shown in the context
menus.

The factory class contains a category, which states which kind of
data can be shown by this widget. The class also has an alias which is
the string shown in the context menu. To give an example:

::

    from widget import DisplayWidget

    class MyWidget(DisplayWidget):
        category = "Digit" # this widget will show numbers
        alias = "My cool widget" # this is shown in the context menu

Upon initialization, the factory gets two parameters: *box*, which
is a :class:`~schema.Box`, containing all the details of the
value to be shown, including the value itself. The other parameter
is *parent*. The initilization is supposed to create a Qt widget
by the name *widget*, with *parent* as its parent:

::

    def __init__(self, box, parent)
        super(MyWidget).__init__(self, box)
        self.widget = QWidget(parent)

One may certainly use any other widget, including self-written
widgets, as long as they inherit from :class:`~PyQt4.QtGui.QWidget`.

As the type of the value to be shown might change, a method
``typeChanged`` is called with the box as a parameter. It is
guaranteed that this method is called before any value is shown, so
this method may be used for initialization as well, avoiding code
doubling with ``__init__``.

Once a value for the widget arrives, ``valueChanged`` is called, with
*box*, *value* and *timestamp* as parameters. This is when finally
data is shown in the widget.

::
    
    def typeChanged(self, box):
        # do some initialization for a new value type

    def valueChanged(self, box, value, timestamp=None):
        # typically something like:
        self.widget.setValue(value)

With those methods implemented, a simple display widget is ready.

A simple editable widget
------------------------

Besides that an editable widget inherits from
:class:`~widget.EditableWidget`, some more methods need to be
implemented for an editable widget.

The factory should contain an attribute ``value`` which contains the
value currently set by the user. It is often common to define a
property if the value is actually stored in the underlying widget, as
in:

::

    @property
    def value(self):
        return self.widget.value()


Once the value is set,
:meth:`~widget.EditableWidget.onEditingFinished` should be called with
one parameter, the new value. This is even the case if the value was
set artificially, so a ``valueChanged`` method often ends in a call
to :meth:`~widget.EditableWidget.onEditingFinished`.

One more note on ``valueChanged``: its parameters seem to be
redundant, as you get the *box* and a *value*. But there is a difference:
the *box* contains the value on the device, while the *value* might be
set somewhere else and should be shown in this widget, too. So you
should show the value in the parameter *value*, not the one in the *box*.


Making the widget customizable
------------------------------

Sometimes the widget needs more information to be shown. To achieve
that on can add an action to the widget, which will be shown in its
context menu. This action may then be connected to an arbitrary
method.

::

    def __init__(self, box, parent):
        super(MyWidget).__init__(self, box)
        self.widget = SomeWidget(parent)
        action = QAction("Change something in My Widget...", self.widget)
        action.triggered.connect(self.onConfigureMyWidget)
        self.widget.addAction(action)

Most of the time, you will also have to save the changes to the scene.
This is done by writing two methods, ``load`` and ``save``, which get
one parameter: an :class:`~xml.etree.ElementTree.Element`. This is the
element in the XML file representing our widget. You may add
attributes or even sub-element at wish, but don't forget to set a
namespace, otherwise the XML won't be well-formatted anymore. Use the
karabo namespace or define your own, as you wish.

::

    from const import ns_karabo # this is at the top of the file

    ...
        def save(self, element):
            element.set(ns_karabo + "mydata", self.something)

        def load(self, element):
            self.something = element.get(ns_karabo + "mydata")
