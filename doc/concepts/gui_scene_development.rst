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
code for the scene lives in ``karabo_gui.sceneview``. It is built using Qt and
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

Version 2+ of the scene file format is basically the same as version 1, but
some elements have minor changes which are incompatible with the old reader
implementations. These changes should be in agreement with the guidelines
enumerated below.


Making Changes to the Scene File format
---------------------------------------

If you wish to add new data to the scene file format, or change the format of
data which is already there, you should take note of the following:

* If adding a new element, create a class which inherits ``BaseSceneObjectData``
  (a *model class*).

  * Increment the ``SCENE_FILE_VERSION`` constant in
    ``karabo.common.scenemodel.const``.
  * Create a reader function for the class. Register the reader with the
    ``register_scene_reader`` decorator. Make sure you pass
    ``version=<new version>`` (where ``<new version>`` is the new value of
    ``SCENE_FILE_VERSION``) to the decorator.
  * Create a writer function for the class. Register the writer with the
    ``register_scene_writer`` decorator.
  * Add unit tests which cover all the new code that you added. Try to cover
    edge cases that you can think of.

* If modifying an existing model class, you *likely* will need to add a new
  reader function.

  * As with adding data, increment the ``SCENE_FILE_VERSION`` constant first.
  * Make whatever changes are necessary to the model class.
  * Create a **new reader function** and register it with a version equal to the
    new value of ``SCENE_FILE_VERSION``.
  * Update the old reader function(s), **but only if NOT doing so would cause
    an exception when instantiating the model class**.

    * The overall aim is to construct the latest version of the model class from
      *any* version of the file data.

  * Update unit tests carefully.

.. note::

  Removing data from the file format is always safe. Old files which contain the
  data will continue to be readable, because the reader simply ignores the data.


Unit Tests
----------

The scene model code, by virtue of being independent from the view code, has
very extensive test coverage. You should strive to maintain this when making
changes. It is intended as a defensive measure against introducing breaking
changes to users. Unfortunately, it's not automatic, and it requires a bit of
discipline on the part of developers working on the scene.


The View
========

The job of the subpackage ``karabo_gui.sceneview`` is to create a visual
representation of the data in scene model objects *and* give a way to
manipulate that data.

.. note::

  The current scene view architecture (as of early 2017) is at the bleeding
  edge of a much larger refactor of the GUI code base. As such, it requires a
  bit more work than necessary when adding new widgets. It's doing this to
  accommodate the existing ``DisplayWidget`` classes, which are also used by
  the configuration panel. This should eventually change, but it's a long
  process.


Adding a New Widget
-------------------

If you haven't added the data for your widget to the scene model yet, you
should first do that before proceeding with the view portion. Once your new
widget has a data model class associated with it, you can make it appear in the
scene by doing the following:

* Create a ``DisplayWidget``/``EditableWidget`` class (or classes) which will
  be shown in the scene
* Create a ``BaseWidgetContainer`` class which acts as an intermediary between
  the ``DisplayWidget`` and the data model instance.

  * The ``karabo_gui.sceneview.widget`` subpackage has lots of examples if
    you're curious how this intermediary should work.
  * If your model class contains no data (ie: it's only being used for its type)
    then you might be able to use the machinery in
    ``karabo_gui.sceneview.widget.generic``.

* Add your container class to ``karabo_gui.sceneview.widget.api``
* Import your model class in ``karabo_gui.sceneview.tools.const`` and add it
  to the ``WIDGET_FACTORIES`` dictionary (mapping ``DisplayWidget`` subclass
  name -> model class).
* Import your container and model classes in ``karabo_gui.sceneview.builder``
  and add them to the ``_SCENE_OBJ_FACTORIES`` dictionary.
* Test in the GUI.

.. note::

  If your new scene object **does NOT** need to interact with device properties
  you should take a look at ``karabo_gui.sceneview.widgets.simple``. Adding
  things to the scene view isn't *always* complicated.


Adding a New Shape
------------------

Creating a new shape ``karabo_gui.sceneview.shapes`` is a bit easier, due to
the fact that shapes are not maintaining backwards compatibility with other
parts of the GUI code base. That said, you should still begin by creating a
data model class for your shape.

* Create a ``BaseShape`` class which will be shown in the scene
* Import your shape and model classes in ``karabo_gui.sceneview.builder``
  and add them to the ``_SHAPE_CLASSES`` dictionary.
* Test in the GUI
