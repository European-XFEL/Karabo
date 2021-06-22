Next Steps to remove Netbeans as a build system
===============================================

1. [X] Add a `CMakelists.txt` for the `karathon` folder.

2. [X] Add a `CMakelists.txt` for the `cppLongTests` folder.

3. [X] Add device template based on `cmake`.

4. [ ] add the cmake to the `auto_build_all` script. Introduce a side script, `cmake_auto_build_all` for cmake and start deprecation period for Netbeans based builds.

5. [ ] implement code coverage for CMake based builds.

6. [ ] move the helper scripts out of `build/netbeans/*` - this marks the conclusion of the Netbeans deprecation period.

7. [ ] remove templates for Netbeans based devices.

8. [ ] remove the `build/netbeans/` directories

Steps 6, 7 and 8 should be an all or nothing feature in a Karabo release - there should be no Karabo release with some but not all artifacts produced by those steps.

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

1. [ ] refactor the conda dependency to upload a single environment file (conda-pack)

   1. [ ] create a separate conda recipe for Karathon and add support for updating the Karathon dependencies conda environment when the recipe changes (similar to what is done currently for the Karabo lib). Also move the Karabo lib environment check to the appropriate level (currently in the root CMakeLists.txt).

   2. [ ] provide CI jobs for updating the conda environments with the dependencies for the Karabo and Karathon libs whenever their conda recipes change. The build jobs later in the CI pipeline should use the artifacts produced by these new jobs instead of always building the conda environment.

   The two substeps above don't imply two separate Conda environments in the developer's machine - one for libkarabo dependencies and one for libkarathon dependencies. That would be cumbersome and error prone, as the paths for the two environments would have to be set for CMAKE_PREFIX_PATH. In the developer's machine there should be a single Conda environment which has both dependency packages installed - actually it will only have to install the libkarathon dependencies package as, by transitivity, that will install the libkarabo dependencies package.

2. [ ] make the `auto_build_all` fetch the remote single environment file instead of building the dependencies

3.  [ ] get rid of the code from the `Framework` repository

Steps to fully use Conda environments
=====================================

11. [ ] Implement portion of the framework as single components (e.g. hash serialization, etc.)

12. [ ] build the framework importing the components as they were external modules

13. [ ] Separate the components out of the `Framework`

14. [ ] Allow subcomponents to be installed separately
