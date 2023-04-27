..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

********************
Developing the Scene
********************

As it is an important part of the Karabo GUI, we expect that the Scene will
require new widgets/elements and other changes as time goes by. The following
should be considered guidelines for accomplishing that in a way which doesn't
cause undue pain to existing users of the GUI.


Basic Architecture
==================

The scene code is generally split into two parts: model and view. The model
code lives in ``karabo.common.scenemodel`` and is responsible for the
representation of scene data and reading/writing those data to/from files. It
is built using the `Traits library <http://docs.enthought.com/traits/>`_, which
you should familiarize yourself with if you plan to work on the scene. The view
code for the scene lives in ``karabogui.sceneview``. It is built using Qt and
makes use of the model objects.

.. note::

  This separation of model and view has an important benefit: You don't *need*
  a GUI to work with scenes. Having a well defined and **independent** data
  model means that power users can script the creation/modification of scene
  files.


Data Model
==========

The job of ``karabo.common.scenemodel`` is to describe the data of a scene.
GUI code does not belong here. For that, see `The View`_ below.


Scene File Versioning
---------------------

Every scene file, starting with version 2, contains a ``krb:version`` attribute
in the root SVG element which gives the version of the file. If the attribute
is missing, a file is assumed to be version 1.

The general philosophy of versioning in the scene file format is that old data
must *always* be readable. If a file is then modified and saved, it will be
written using the *latest version* of the file format. To accomplish this, the
scene data model code will necessarily accumulate reader code over time, but
will only ever know how to write out the current version of the file format.


The Scene File Format
---------------------

Version 1 of the scene file format is described in detail here:
:ref:`scene-file-version-one`. In general, the file format is SVG and
individual elements in a scene file are recognized by their corresponding
reader functions in the Karabo library.

Version 2 of the scene file format is basically the same as version 1, but
some elements have minor changes which are incompatible with the old reader
implementations. These changes should be in agreement with the guidelines
enumerated below.

Version 3 (and later) does not yet exist at the time of this writing. The
condition for it to exist would be if an existing element (or elements) need(s)
a new reader. As soon as the format of any element changes enough that it can't
be handled cleanly by the current reader, then a new reader (for that element)
and new file version is required.


Making Changes to the Scene File format
---------------------------------------

If you wish to add new data to the scene file format, or change the format of
data which is already there, you should take note of the following:

* If adding a new element, create a class which inherits ``BaseSceneObjectData``
  (a *model class*).

  * Create a reader function for the class. Register the reader with the
    ``register_scene_reader`` decorator. Make sure you pass
    ``version=<current version>`` (where ``<current version>`` is the value of
    ``SCENE_FILE_VERSION``) to the decorator. Don't pass ``SCENE_FILE_VERSION``
    because this value will change over time and you want to pin your reader to
    a single version.
  * Create a writer function for the class. Register the writer with the
    ``register_scene_writer`` decorator.
  * **Add unit tests which cover all the new code that you added**. Try to cover
    edge cases that you can think of.
  * Starting Karabo 2.10, the scene reader and writer functions would only
    expect one parameter, which is the XML element (``element``). The reader and
    writer entrypoint function (``read_func``/``write_func``) is not passed
    anymore. The functions ``read_element`` and ``write_element`` can be used
    if there is a need to handle children elements.

* If *lightly* modifying an existing model class, you can make small changes to
  the reader function

  * **Leave the** ``SCENE_FILE_VERSION`` **constant alone**.
  * Make whatever changes are necessary to the model class.
  * Update the other reader function(s) for the model if needed
  * Update unit tests carefully.

* If modifying an existing model class in a way which requires a new reader
  function:

  * Increment the ``SCENE_FILE_VERSION`` constant first.
  * Make whatever changes are necessary to the model class.
  * Create a **new reader function** and register it with a version equal to the
    new value of ``SCENE_FILE_VERSION``. Remember not to use
    ``SCENE_FILE_VERSION`` here. Use its value.
  * Update the old reader function(s), **but only if NOT doing so would cause
    an exception when instantiating the model class**.

    * The overall aim is to construct the latest version of the model class from
      *any* version of the file data.

  * Update unit tests carefully.

.. note::

  Removing data from the file format is always safe. Old files which contain the
  data will continue to be readable, because the reader can simply ignore the
  data.

.. note::

  Similarly, adding new widgets to the file format is also safe, as long as the
  addition is orthogonal to existing data in the format. As of version 2.2(-ish)
  the ``UnknownWidgetDataModel`` catches new widgets which do not have a reader
  registered.


Unit Tests
----------

The scene model code, by virtue of being independent from the view code, has
very extensive test coverage. You should strive to maintain this when making
changes. It is intended as a defensive measure against introducing breaking
changes to users. Unfortunately, it's not automatic, and it requires a bit of
discipline on the part of developers working on the scene.

**This is very important**. Good unit test coverage of the scene file model
code is the main defense against user hostile bugs encountered when loading
scenes.


The View
========

The job of the subpackage ``karabogui.sceneview`` is to create a visual
representation of the data in scene model objects *and* give a way to
manipulate that data.


Adding a New Widget
-------------------

If you haven't added the data for your widget to the scene model yet, you
should first do that before proceeding with the view portion. Once your new
widget has a data model class associated with it, you can make it appear in the
scene by doing the following:

* Create a ``BaseBindingController`` class (or classes) which will be shown in
  the scene.
* Make sure your controller class has a ``model`` trait which is an ``Instance``
  of whatever your scene model class is.
* Register your controller class with the ``register_binding_controller``
  decorator.
* Add unit tests for your controller class.
* Test in the GUI.


.. note::

  A Developer's Checklist is documented in :ref:`gui-widget-checklist`

.. note::

  If your new scene object **does NOT** need to interact with device properties
  you should take a look at ``karabogui.sceneview.widget``. Adding things to
  the scene view isn't *always* dealing with properties.

Adding a New Shape
------------------

Creating a new shape ``karabogui.sceneview.shapes`` is a bit easier, due to
the fact that shapes are not maintaining backwards compatibility with other
parts of the GUI code base. That said, you should still begin by creating a
data model class for your shape.

* Create a ``BaseShape`` class which will be shown in the scene
* Import your shape and model classes in ``karabogui.sceneview.builder``
  and add them to the ``_SHAPE_CLASSES`` dictionary.
* Test in the GUI
