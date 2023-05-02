Building Karabo Conda Package
-----------------------------

In order to build the *karaboGUI* package for a specified version, one needs to
 either tag or checkout to a already tagged version. After the checkout, just 
 execute the `build_conda_recipe.sh` script. It will update the recipe 
 information based on the `karabogui` environment and spawn the building 
 process. If everything is correct, your package will be located in 
 `<miniconda_path>/conda_bld/<platform>/`.
