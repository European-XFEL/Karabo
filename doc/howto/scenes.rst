Scene Object Classes
--------------------

Shape
=====

WorkflowItem
============

EditableNoApplyComponent
========================

EditAttributeComponent
======================

WorkflowGroupItem
=================

EditableApplyLaterComponent
===========================

Item
====

SceneLink
=========

BoxLayout
=========

Inherits from `Layout` and `QBoxLayout`

Additional tags:

 - direction

ChoiceComponent
===============

FixedLayout
===========

Inherits from `Layout` and `QLayout`

Uses `save` method of parent class.

Implements `add_children` for saving and `loadPosition` for loading.

BaseComponent
=============

Inherits from `Loadable` and `QObject`

The saving of a `BaseComponent` is done in `def save(self, element)` which
calls `self.widgetFactory.save(e)`. That means that some display widgets have a
`save` method implemented.

Additional tags:

 - widget (class name of the GUI widget)
 - keys (list of associated property keys)

Layout
======

Inherits from `Loadable`

Additional tags describe the geometry of the layout:

 - x
 - y
 - width
 - height
 - class

Rectangle
=========

Inherits from `Shape`

The saving of the line is done in `def element(self)` which calls `Shape.savepen()`

Additional tags:

 - x
 - y
 - width
 - height

DisplayComponent
================

Inherits from `BaseComponent` which does the saving and the loading.

Line
====

Inherits from `Shape`

The saving of the line is done in `def element(self)` which calls `Shape.savepen()`

Additional tags:

 - x1
 - x2
 - y1
 - y2

Path
====

Inherits from `Shape`

The saving of the path is done in `def element(self)` which calls `Shape.savepen()`

Additional tags:

 - d

GridLayout
==========

Inherits from `Layout` and `QGridLayout`

The saving of this layout is done in the parent class `Layout.element(self, selected=False)`

Label
=====

Inherits from `QLabel` and `Loadable`

The saving of the label is done in `def save(self, ele)`

Additional tags:

 - text
 - font
 - foreground
 - background
 - frameWidth


Scene SVG Object Classes
------------------------

Rectangle
=========

Tag: {http://www.w3.org/2000/svg}rect

Line
====

Tag: {http://www.w3.org/2000/svg}line

FixedLayout
===========

Tag: {http://www.w3.org/2000/svg}g

Path
====

Tag: {http://www.w3.org/2000/svg}path
