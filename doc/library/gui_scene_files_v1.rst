..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

.. _scene-file-version-one:

====================
Scene File Version 1
====================

This is the (mostly) complete documentation of the ``Scene`` file format
(version 1). The ``Scene`` file format is based on SVG and in fact, scene files
can be opened in any program which can edit SVG files.


Scene SVG Object Classes
------------------------

These are the classes which are ``Loadable`` children and also define a
``xmltag`` class attribute. They don't appear to be handled any differently.
It's possible that the original meaning was lost. See below for more
information about these classes.

Rectangle
=========

Tag: `{http://www.w3.org/2000/svg}rect`

Line
====

Tag: `{http://www.w3.org/2000/svg}line`

FixedLayout
===========

Tag: `{http://www.w3.org/2000/svg}g`

Path
====

Tag: `{http://www.w3.org/2000/svg}path`


Scene Object Classes
--------------------

All objects write the attribute `{ns_karabo}class`, which is the name of
the object's class (for dispatching the correct load method when reading).

Note that `{ns_karabo}` is shorthand for `{http://karabo.eu/scene}` and
`{ns_svg}` is shorthand for `{http://www.w3.org/2000/svg}`.

**All object position are all saved absolute - not layout relative.**



Layout
======

Inherits from ``Loadable``.

This is the base class for other layouts and should never be instantiated
as a ``Layout``.

Saving occurs in the ``element`` method. An element with the tag `{ns_svg}g`
is created and the following attributes are added:

Attributes:

 - `{ns_karabo}x`: Top-left X coordinate
 - `{ns_karabo}y`: Top-left Y coordinate
 - `{ns_karabo}width`: The width of the layout
 - `{ns_karabo}height`: The height of the layout
 - `{ns_karabo}class`: The classname of the layout

Then the ``save`` method (of a derived class) is called to add any additional
attributes.

Finally, the ``add_children`` method is called to add subelements for all of
the children of this layout.

BoxLayout
=========

Inherits from ``Layout`` and ``QBoxLayout``

Implements the ``save`` method, as well as ``load`` and ``loadPosition``.

Attributes:

 - `{ns_karabo}direction`: An integer representing a ``QBoxLayout::Direction``

FixedLayout
===========

Inherits from ``Layout`` and ``QLayout``

Implements the ``save`` method (but adds no attributes) and the ``add_children``
method for saving.

Implements the ``load`` and ``loadPosition`` methods for loading.

GridLayout
==========

Inherits from ``Layout`` and ``QGridLayout``

Implements the ``save`` method (but adds no attributes) and the ``add_children``
method for saving. The attributes `{ns_karabo}row`, `{ns_karabo}col`,
`{ns_karabo}rowspan`, and `{ns_karabo}colspan` are added to non-Shape children.

Implements the ``load`` and ``loadPosition`` methods for loading.

Label
=====

Inherits from ``QLabel`` and ``Loadable``

The saving of the label is done in ``save(self, ele)``

Attributes:

 - `{ns_karabo}text`: The text which is displayed
 - `{ns_karabo}font`: The font used to display the text
 - `{ns_karabo}foreground`: A CSS-compatible color name
 - `{ns_karabo}background`: A CSS-compatible color name
 - `{ns_karabo}frameWidth`: An integer representing the line width of the frame
   around the text (the ``QFrame`` ``lineWidth`` property).

Shape
=====

Inherits from ``ShapeAction`` -> ``Action`` -> ``Registry`` (also ``Loadable``)

Base class for ``Line``, ``Rectangle``, and ``Path``. Does not define ``save``
or ``load`` methods, but *does* define ``savepen`` and ``loadpen``.

Attributes (from ``savepen`` / ``loadpen`` ):

 - `stroke`: An HTML color `#hex` value, or `"none"`
 - `stroke-opacity`: A floating point number between 0 and 1. Default 1
 - `stroke-linecap`: `butt`, `square`, or `round`. Default `butt`
 - `stroke-dashoffset`: A floating point number. Default 0
 - `stroke-width`: A floating point number. Default 1
 - `stroke-dasharray`: A comma-separated list of dash lengths. Default `"none"`
 - `stroke-style`: An integer which maps to a ``QPen`` style. Default
   ``Qt.SolidLine``
 - `stroke-linejoin`: `miter`, `round`, or `bevel`. Default `miter`
 - `stroke-miterlimit`: A floating point number. Default 4
 - `fill`: An HTML color `#hex` value, or `"none"`
 - `fill-opacity`: A floating point number between 0 and 1. Default 1

All pen attribute dimensions supply a unit from the list:
`px`, `pt`, `pc`, `mm`, `cm`, `in`. This value is then converted to the
corresponding number of pixels (based on a 90 DPI display). The absence of a
unit suffix indicates `px` (pixels).

Rectangle
=========

Inherits from ``Shape``

The saving of the ``Rectangle`` is done in the ``element`` method which calls
``Shape.savepen``

Creates an element with the tag `{ns_svg}rect`. Adds the following
attributes and calls ``Shape.savepen`` on the element.

Attributes:

 - `x`: Top-left X coordinate
 - `y`: Top-left Y coordinate
 - `width`: Width
 - `height`: Height

Line
====

Inherits from ``Shape``

The saving of the ``Line`` is done in the ``element`` method which calls
``Shape.savepen``

Creates an element with the tag `{ns_svg}line`. Adds the following
attributes and calls ``Shape.savepen`` on the element.

Attributes:

 - `x1`: Starting X coordinate
 - `x2`: Ending X coordinate
 - `y1`: Starting Y coordinate
 - `y2`: Ending Y coordinate

Path
====

Inherits from ``Shape``

The saving of the ``Path`` is done in the ``element`` method which calls
``Shape.savepen``

Creates an element with the tag `{ns_svg}path`. Adds the following
attributes and calls ``Shape.savepen`` on the element.

Attributes:

 - `d`: A string containing SVG data (handled by ``PathParser``)

BaseComponent
=============

Inherits from ``Loadable`` and ``QObject``

The saving of a ``BaseComponent`` is done in ``save(self, element)`` which
calls ``self.widgetFactory.save(e)``. That means that some display widgets have
a ``save`` method implemented.

Attributes:

 - `{ns_karabo}widget`: Class name of the GUI widget
 - `{ns_karabo}keys`: List of associated property keys (box names)

DisplayComponent
================

Inherits from ``BaseComponent`` which does the saving and the loading.

EditableNoApplyComponent
========================

Inherits from ``BaseComponent`` which does the saving and the loading.

EditableApplyLaterComponent
===========================

Inherits from ``BaseComponent`` which does the saving and the loading.

EditAttributeComponent
======================

Inherits from ``BaseComponent`` which does the saving and the loading.

ChoiceComponent
===============

Inherits from ``BaseComponent`` which does the saving and the loading.

Item
====

Does not define a ``load`` method. This is the common base class for
``WorkflowItem`` and ``WorkflowGroupItem``.

WorkflowItem
============

Attributes:

 - `{ns_karabo}text`: The device ID for the item.
   **must be looked up in the project**
 - `{ns_karabo}font`: The font to use for the item

Calls ``layout.loadPosition(element, sceneWidget)``, where ``sceneWidget`` is
the parent of the item being created. ``layout`` is any one of
``FixedLayout``, ``BoxLayout``, or ``GridLayout``

WorkflowGroupItem
=================

The same as ``WorkflowItem``, but `{ns_karabo}text` is a device group identifier

SceneLink
=========

Attributes:

 - `{ns_karabo}target`: The scene name which is linked to.

Calls ``layout.loadPosition(element, sceneWidget)``, where ``sceneWidget`` is
the parent of the item being created.


Widget Object Classes (DisplayWidget, EditableWidget, VacuumWidget)
-------------------------------------------------------------------

These widgets are also saved to ``Scene`` files.

EditableCheckBox
================

Inherits from ``EditableWidget``

Alias: `Toggle Field`

No ``save`` or ``load`` methods.

EditableChoiceElement
=====================

Inherits from ``EditableWidget``

Alias: `Choice Element`

No ``save`` or ``load`` methods.

EditableComboBox
================

Inherits from ``EditableWidget``

Alias: `Selection Field`

No ``save`` or ``load`` methods.

SingleBit
=========

Inherits from ``EditableWidget``

Alias: `Single Bit`

Attributes:

 - `{ns_karabo}bit`: An integer denoting a bit index

EditableLineEdit
================

Inherits from ``EditableWidget``

Alias: `Text Field`

No ``save`` or ``load`` methods.

EditableDirectory
=================

Inherits from ``EditableWidget``

Alias: `Directory`

No ``save`` or ``load`` methods.

EditableFileOut
===============

Inherits from ``EditableWidget``

Alias: `File Out`

Does not define ``save`` or ``load`` methods.

``EditableFileOut`` and ``DisplayFileOut`` should be combined in one class and
which inherits from ``EditableWidget`` and ``DisplayWidget``.

EditableFileIn
==============

Inherits from ``EditableWidget``

Alias: `File In`

No ``save`` or ``load`` methods.

Slider
======

Inherits from ``QwtWidget`` which is an ``EditableWidget``

Alias: `Slider`

Does not define ``save`` or ``load`` methods.

Knob
====

Inherits from ``QwtWidget`` which is an ``EditableWidget``

Alias: `Knob`

Does not define ``save`` or ``load`` methods.

FloatSpinBox
============

Inherits from ``EditableWidget`` and ``DisplayWidget``

Alias: `Spin Box`

Attributes:

 - `{ns_karabo}step`: A floating point number denoting the widget's step size

EditableSpinBox
===============

Inherits from ``EditableWidget`` and ``DisplayWidget``.

Alias: `Integer Spin Box`

Does not define ``save`` or ``load`` methods.

EditableTableElement
====================

Inherits from ``EditableWidget`` and ``DisplayWidget``.

Alias: 'Table Element`

Attributes:

 - `{ns_karabo}columnSchema`: Schema which defines the table

DisplayTableElement
===================

Inherits from ``EditableTableElement``

Alias: `Display Table Element`

The ``save`` or ``load`` methods are inherited from ``EditableTableElement``

Bitfield
========

Inherits from ``EditableWidget`` and ``DisplayWidget``.

Alias: `Bit Field`

Does not define ``save`` or ``load`` methods.

``self.widget`` is the self defined widget ``BitfieldWidget`` implemented in the
same file.

DoubleLineEdit
==============

Inherits from ``NumberLineEdit`` which inherits from ``EditableWidget`` and
``DisplayWidget``.

Alias: `Float Field`

Does not define ``save`` or ``load`` methods.

IntLineEdit
===========

Inherits from ``NumberLineEdit`` which inherits from ``EditableWidget`` and
``DisplayWidget``.

Alias: `Integer Field`

Does not define ``save`` or ``load`` methods.

EditableList
============

Inherits from ``EditableWidget`` and ``DisplayWidget``

Alias: `List`

No ``save`` or ``load`` methods.


EditableListElement
===================

Inherits from ``EditableWidget`` and ``DisplayWidget``

Alias: `List Element Field`

No ``save`` or ``load`` methods.



DisplayLabel
============

Inherits from ``DisplayWidget``

Alias: `Value Field`

Does not define ``save`` or ``load`` methods.

This widget is used for the current value on device.

Evaluator
=========

Inherits from ``DisplayWidget``

Alias: `Evaluate Expression`

Attributes:

 - `{ns_karabo}expression`: The expression which gets evaluated.

DisplayIconset
==============

Inherits from ``DisplayWidget``

Alias: `Iconset`

Attributes:

 - `{ns_karabo}url`: Filename of the iconset
 - `{ns_karabo}filename`: If `{ns_karabo}url` is not set then this attribute is
   used for the URL

DisplayCheckBox
===============

Inherits from ``DisplayWidget``.

Alias: `Toggle Field`

Does not define ``save`` or ``load`` methods.

XYVector
========

Inherits from ``DisplayWidget``

Alias: `"XY-Plot`

Contains a list of subelements with the tag `{ns_karabo}box`. The attributes
for these elements are defined below:

Attributes:

 - `device`: The device ID
 - `path`: property name and ``curve`` data

Same elements and attributes are saved as in ``XYVector``, ``DisplayTrendline``.

Several vectors of the same size are plotted against each other.

DisplayPlot
===========

Inherits from ``DisplayWidget``

Alias: `Plot`

No ``save`` or ``load`` methods.

**Note**: Adjacent to this code is a ``PlotItem`` class which contains a large
number of ``NameError`` opportunities. It looks like it's not used anywhere in
the Karabo GUI code and might be a good candidate for removal.

XYPlot
======

Inherits from ``DisplayWidget``

Alias: `XY-Plot`

Two values are plotted against each other.

Does not define ``save`` or ``load`` methods, which actually should be changed.

DisplayTrendline
================

Inherits from ``DisplayWidget``

Alias: `Trendline`

Contains a list of subelements with the tag `{ns_karabo}box`. The attributes
for these elements are defined below:

Attributes:

 - `device`: The device ID
 - `path`: property name and ``curve`` data

Same elements and attributes are saved as in ``XYVector``, ``DisplayTrendline``.

A vector is plotted.

DisplayLineEdit
===============

Inherits from ``DisplayWidget``

Alias: `Text Field`

Does not define ``save`` or ``load`` methods.

DisplayStateColor
=================

Inherits from ``DisplayWidget``

Alias: `State Color Field`

Attributes:

 - `{ns_karabo}staticText`; The text shown on the widget

Sub Elements: Use the tag `{ns_karabo}sc` and the data is the name of the state

 - `red`: Color component for red
 - `green`: Color component for green
 - `blue`: Color component for blue
 - `alpha`: Color component for alpha channel

Monitor
=======

Inherits from ``DisplayWidget``

Alias: `Monitor`

Attributes:

 - `filename`: A string containing a file path (*can be absent*)
 - `interval`: A floating point number of seconds

DisplayFileOut
==============

Inherits from ``DisplayWidget``

Alias: `File Out`

No ``save`` or ``load`` methods.

DisplayFileIn
=============

Inherits from ``DisplayWidget``

Alias: `File In`

Does not define ``save`` or `` load`` methods.

``EditableFileIn`` and ``DisplayFileIn`` should be combined in one class and
which inherits from ``EditableWidget`` and ``DisplayWidget``.

DisplayAlignedImage
===================

Inherits from ``DisplayWidget``

Alias: `Aligned Image View`

No ``save`` or ``load`` methods.

DisplayImage
============

Inherits from ``DisplayWidget``

Alias: `Image View`

No ``save`` or ``load`` methods.

SelectionIcons
==============

Inherits from ``Icons`` which inherits from ``DisplayWidget``

Alias: `Icons`

Identical to ``DigitIcons``, except that the child element tags are
`{ns_karabo}option` and `image` is the only valid attribute.

TextIcons
=========

Inherits from ``Icons`` which inherits from ``DisplayWidget``

Alias: `Icons`

Identical to ``DigitIcons``, except that the child element tags are
`{ns_karabo}re` and `image` is the only valid attribute.

DigitIcons
==========

Inherits from ``Icons`` which inherits from ``DisplayWidget``

Alias: `Icons`

An element containing a ``DigitIcons`` instance contains zero or more
subelements with the tag `{ns_karabo}value`. The format of those elements
follows:

Data: The ``value`` attribute of the given ``Item`` (a text label??)

Attributes:

 - `equal`: A string containing the value `true` or `false` (*can be absent*)
 - `image`: A URL for an icon (*can be absent*)

DisplayImageElement
===================

Inherits from ``DisplayWidget``

Alias: `Image Element`

No ``save`` or ``load`` methods.

DisplayDirectory
================

Inherits from ``DisplayWidget``

Alias: `Directory`

No ``save`` or ``load`` methods.

DisplayCommand
==============

Inherits from ``DisplayWidget``

Alias: `Command`

An element containing a ``DisplayCommand`` instance contains zero or more
subelements with the tag `{ns_karabo}action`. The format of those elements
follows:

Data: Empty

Attributes:

 - `key`: A string containing a ``Box`` path
 - `image`: A URL for an icon

DisplayChoiceElement
====================

Inherits from ``DisplayWidget``

Alias: `Choice Element`

No ``save`` or ``load`` methods.

DisplayComboBox
===============

Inherits from ``DisplayWidget``

Alias: `Selection Field`

No ``save`` or ``load`` methods.



MembranePumpWidget
==================

Inherits from ``VacuumWidget``

Alias: `Membrane Pump`

Should be removed - use Iconsets for vacuum widgets instead.

RightAngleValveWidget
=====================

Inherits from ``VacuumWidget``

Alias: `Right angle valve`

Should be removed - use Iconsets for vacuum widgets instead.

Hexadecimal
===========

Inherits from ``EditableWidget`` and ``DisplayWidget``.

Alias: `Hexadecimal`

Does not define ``save`` or ``load`` methods.

MotorWidget
===========

Inherits from ``VacuumWidget``

Alias: `Motor`

Should be removed - use Iconsets for vacuum widgets instead.

ValveWidget
===========

Inherits from ``VacuumWidget``

Alias: `Valve`

Should be removed - use Iconsets for vacuum widgets instead.

PressureSwitchWidget
====================

Inherits from ``VacuumWidget``

Alias: `Pressure switch`

Should be removed - use Iconsets for vacuum widgets instead.

TemperatureProbeWidget
======================

Inherits from ``VacuumWidget``

Alias: `Temperature probe`

Should be removed - use Iconsets for vacuum widgets instead.

PressureGaugeWidget
===================

Inherits from ``VacuumWidget``

Alias: `Pressure gauge`

Should be removed - use Iconsets for vacuum widgets instead.

TurboPumpWidget
===============

Inherits from ``VacuumWidget``

Alias: `Turbo pump`

Should be removed - use Iconsets for vacuum widgets instead.

ShutOffValveWidget
==================

Inherits from ``VacuumWidget``

Alias: `Shut off valve`

Should be removed - use Iconsets for vacuum widgets instead.

MaxiGaugeWidget
===============

Inherits from ``VacuumWidget``

Alias: `Maxi gauge`

Should be removed - use Iconsets for vacuum widgets instead.

HydraulicValveWidget
====================

Inherits from ``VacuumWidget``

Alias: `Hydraulic valve`

Should be removed - use Iconsets for vacuum widgets instead.

CryoCoolerWidget
================

Inherits from ``VacuumWidget``

Alias: `Cryo-cooler`

Should be removed - use Iconsets for vacuum widgets instead.
