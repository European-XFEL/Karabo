..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

.. _gui-widget-checklist:

==========================
Widget Developer Checklist
==========================

the following Classes needs to be created:

- The Scene MODEL of the widget
- The UNIT TESTS!
- The WIDGET CONTROLLER

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

WIDGET CONTROLLER
=================

- Add the WIDGET CONTROLLER code to the `src/pythonGui/karabogui/controllers/[display|edit]`
  directory.
- Note that it is a requirement (enforced by register_binding_controller) that
  controller classes define a `model` trait which binds them to the scene MODEL
  class which they use. The unit tests will break if you forget this.
- Add unit tests for your controller. Look at tests for existing controllers if
  you are curious how that's accomplished.

Connecting it all together
==========================

- add import of the MODEL to the `src/pythonKarabo/karabo/common/scenemodel/api.py` file
- Make sure the controller class is decorated with `register_binding_controller`
