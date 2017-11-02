.. _gui-widget-checklist:

==========================
Widget Developer Checklist
==========================

the following Classes needs to be created:

- The Scene MODEL of the widget
- The UNIT TESTS!
- The WIDGET itself
- The CONTAINER

if the Scene Model contains traits (i.e. needs to persist configuration data), the 
following elements need to be added:

- The Scene Model WRITER
- The Scene Model READER


MODEL
=====

- add the MODEL to the `src/pythonKarabo/karabo/common/scenemodel/**DISPLAYTYPE**.py`
  file.

if the MODEL contains traits, the developer will add:

- the Scene Model WRITER to 
  `src/pythonKarabo/karabo/common/scenemodel/**DISPLAYTYPE**.py`. 
- the Scene Model READER to
  `src/pythonKarabo/karabo/common/scenemodel/**DISPLAYTYPE**`.
  The READER's version should be the same as the value of 
  `SCENE_FILE_VERSION` in `src/pythonKarabo/karabo/common/scenemodel/const.py`.

If the Scene MODEL does not contain traits, the developer will add 
the MODEL name to the `names` tuple in the function 
`_build_empty_widget_readers_and_writers` in 
`src/pythonKarabo/karabo/common/scenemodel/**DISPLAYTYPE**.py`.

If a Scene MODEL contains only a `klass` trait, the developer will add
the Scene MODEL name to the `names` tuple in the function 
`_build_empty_display_editable_readers_and_writers` in
`src/pythonKarabo/karabo/common/scenemodel/**DISPLAYTYPE**.py`.

UNIT TESTS
==========

- Unless the rules of the universe are suddenly broken, the unit test in
  `src/pythonKarabo/karabo/common/scenemodel/tests/test_data_model_structure.py`
  will be broken. The developer will need to adapt the `EXPECTED_HASH` 
  in that file.

- If the Scene MODEL contains traits, add the UNIT TEST to the 
  `src/pythonKarabo/karabo/common/scenemodel/tests/test_widget_**DISPLAYTYPE**.py`
  file

- If the Scene MODEL does not contain traits, add the class name to the 
`test_all_empty_widgets` method in the 
`src/pythonKarabo/karabo/common/scenemodel/tests/test_widget_simple.py` file.

WIDGET
======

- add the WIDGET code to `src/pythonGui/karabo_gui/*TYPE_OF*widgets` directory in 
  a single file
- add the file to the relative `__init__.py` file of the directory

CONTAINER
=========

- add the CONTAINER code to the `src/pythonGui/karabo_gui/sceneview/widget/` directory in a single file.
- add import of the CONTAINER to the `src/pythonGui/karabo_gui/sceneview/widget/api.py`

.. note::

   This portion is scheduled to be changed soon.

Connecting it all together
==========================

- add import of the MODEL to the `src/pythonKarabo/karabo/common/scenemodel/api.py` file
- map the Scene MODEL to the CONTAINER in the `_SCENE_OBJ_FACTORIES` dictionary in the `src/pythonGui/karabo_gui/sceneview/builder.py` file
- map the WIDGET Name to the MODEL in the `WIDGET_FACTORIES` dictionary in the  `src/pythonGui/karabo_gui/sceneview/tools/const.py` file

