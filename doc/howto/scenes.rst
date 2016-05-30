Documentation of the Scene file format
++++++++++++++++++++++++++++++++++++++

The ``Scene`` file format is based on XML(SVG).

Scene Object Classes
--------------------

All objects write the attribute `{ns_karabo}class`, which is the name of
the object's class (for dispatching the correct load method when reading).

Note that `{ns_karabo}` is shorthand for `{http://karabo.eu/scene}`.

Shape
=====

Inherits from ``ShapeAction`` -> ``Action`` -> ``Registry`` (also ``Loadable``)

Base class for ``Line``, ``Rectangle``, and ``Path``. Does not define ``save``
or ``load`` methods, but *does* define ``savepen`` and ``loadpen``.

Pen Attributes:

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

WorkflowItem
============

Attributes:

 - `{ns_karabo}text`: The device ID for the item.
   **must be looked up in the project**
 - `{ns_karabo}font`: The font to use for the item

Calls ``layout.loadPosition(element, sceneWidget)``, where ``sceneWidget`` is
the parent of the item being created. ``layout`` is any one of
``FixedLayout``, ``BoxLayout``, or ``GridLayout``

EditableNoApplyComponent
========================

Inherits from ``BaseComponent`` which does the saving and the loading.

EditAttributeComponent
======================

Inherits from ``BaseComponent`` which does the saving and the loading.

WorkflowGroupItem
=================

The same as ``WorkflowItem``, but `{ns_karabo}text` is a device group identifier

EditableApplyLaterComponent
===========================

Inherits from ``BaseComponent`` which does the saving and the loading.

Item
====

Does not define a ``load`` method. This is the common base class for
``WorkflowItem`` and ``WorkflowGroupItem``.

SceneLink
=========

Attributes:

 - `{ns_karabo}target`: The scene name which is linked to.

Calls ``layout.loadPosition(element, sceneWidget)``, where ``sceneWidget`` is
the parent of the item being created.

BoxLayout
=========

Inherits from ``Layout`` and ``QBoxLayout``

Attributes:

 - `{ns_karabo}direction`:

ChoiceComponent
===============

Inherits from ``BaseComponent`` which does the saving and the loading.

FixedLayout
===========

Inherits from ``Layout`` and ``QLayout``

Uses ``save`` method of parent class.

Implements ``add_children`` for saving and ``loadPosition`` for loading.

BaseComponent
=============

Inherits from ``Loadable`` and ``QObject``

The saving of a ``BaseComponent`` is done in ``save(self, element)`` which
calls ``self.widgetFactory.save(e)``. That means that some display widgets have
a ``save`` method implemented.

Attributes:

 - `{ns_karabo}widget`: Class name of the GUI widget
 - `{ns_karabo}keys`: List of associated property keys (box names)

Layout
======

Inherits from ``Loadable``

Attributes:

 - `{ns_karabo}x`:
 - `{ns_karabo}y`:
 - `{ns_karabo}width`:
 - `{ns_karabo}height`:

Rectangle
=========

Inherits from ``Shape``

The saving of the ``Rectangle`` is done in ``element(self)`` which calls
``Shape.savepen()``

Attributes:

 - `{ns_karabo}x`; Top-left X coordinate
 - `{ns_karabo}y`; Top-left Y coordinate
 - `{ns_karabo}width`; Width
 - `{ns_karabo}height`; Height

DisplayComponent
================

Inherits from ``BaseComponent`` which does the saving and the loading.

Line
====

Inherits from ``Shape``

The saving of the ``Line`` is done in ``element(self)`` which calls
``Shape.savepen()``

Attributes:

 - `{ns_karabo}x1`: Starting X coordinate
 - `{ns_karabo}x2`: Ending X coordinate
 - `{ns_karabo}y1`: Starting Y coordinate
 - `{ns_karabo}y2`: Ending Y coordinate

Path
====

Inherits from ``Shape``

The saving of the ``Path`` is done in ``element(self)`` which calls
``Shape.savepen()``

Attributes:

 - `{ns_karabo}d`: A string containing SVG data (handled by ``PathParser``)

GridLayout
==========

Inherits from ``Layout`` and ``QGridLayout``

The saving of this layout is done in the parent class
``Layout.element(self, selected=False)``. Actually, the ``save()`` method
for this class returns ``{}``

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


Scene SVG Object Classes
------------------------

These are the classes which are ``Loadable`` children and also define a
``xmltag`` class attribute. They don't appear to be handled any differently.
It's possible that the original meaning was lost.

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


Widget Object Classes (DisplayWidget, EditableWidget, VacuumWidget
------------------------------------------------------------------

These widgets are also saved to ``Scene`` files.

ValveWidget
===========

EditableFileIn
==============

Monitor
=======

TurboPumpWidget
===============

DisplayPlot
===========

FloatSpinBox
============

DisplayFileOut
==============

ShutOffValveWidget
==================

MaxiGaugeWidget
===============

DigitIcons
==========

EditableCheckBox
================

EditableChoiceElement
=====================

DisplayAlignedImage
===================

EditableComboBox
================

DisplayImage
============

SelectionIcons
==============

DisplayImageElement
===================

DisplayDirectory
================

HydraulicValveWidget
====================

DisplayCommand
==============

DisplayFileIn
=============

DisplayChoiceElement
====================

SingleBit
=========

EditableListElement
===================

DisplayTableElement
===================

EditableLineEdit
================

CryoCoolerWidget
================

DisplayComboBox
===============

PressureGaugeWidget
===================

EditableDirectory
=================

EditableList
============

TextIcons
=========

DoubleLineEdit
==============

EditableTableElement
====================

XYPlot
======

DisplayTrendline
================

IntLineEdit
===========

TemperatureProbeWidget
======================

Knob
====

XYVector
========

DisplayLineEdit
===============

DisplayStateColor
=================

PressureSwitchWidget
====================

EditableSpinBox
===============

EditableFileOut
===============

Slider
======

Bitfield
========

MotorWidget
===========

DisplayLabel
============

Evaluator
=========

MembranePumpWidget
==================

DisplayIconset
==============

RightAngleValveWidget
=====================

DisplayCheckBox
===============

Hexadecimal
===========
