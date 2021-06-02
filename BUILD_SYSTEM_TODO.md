Next Steps to remove Netbeans as a build system
===============================================

1. [ ] Add a `CMakelists.txt` for the `karathon` folder.

2. [ ] Add a `CMakelists.txt` for the `cppLongTests` folder.

3. [ ] add the cmake to the `auto_build_all` script

4. [ ] move the helper scripts out of `build/netbeans/*`

5. [ ] refactor the device template to use `cmake`

6. [ ] implement the code coverage

7. [ ] remove the `build/netbeans/` directories

Future steps on subsequent milestones
-------------------------------------

These steps are beyond the 1st stage and are just a rough blueprint.
We should worry how to cross these bridges when we get there.

- [ ] Re-asses if something is missing in the previous milestone

Optional Steps
==============

- [ ] Test Ninjia instead of Make as a building system

Steps to remove dependency code from the repository
===================================================

8. [ ] refactor the conda dependency to upload a single environement file (conda-pack)

9. [ ] make the `auto_build_all` fetch the remote single environment file instead of building the dependencies

10. [ ] get rid of the code from the `Framework` repository

Steps to fully use Conda environments
=====================================

11. [ ] Implement portion of the framework as single components (e.g. hash serialization, etc.)

12. [ ] build the framework importing the components as they were external modules

13. [ ] Separate the components out of the `Framework`

14. [ ] Allow subcomponents to be installed separately
