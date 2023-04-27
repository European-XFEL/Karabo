..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

Contributing Code to Karabo Framework
=====================================

There is a guide available on the Karabo Framework Redmine Wiki:
https://in.xfel.eu/redmine/projects/karabo-library/wiki/Developing_the_framework_using_git

Additionally please follow these Guideline:

Gitlab
======

Large(r) merge requests:
------------------------

There is a recurrent problem of tradeoff between „complete“ merge requests,
e.g. fully implementing and defining a larger addition to the framework,
and smaller, often easier to review merge requests,
which would mean breaking the additions into smaller chunks, frequently
not fully functional on their own. As a solutution please merge to separate
feature branches:

1. Create a new (longer lived) feature branch for your larger project

2. Break down your larger project, so that smaller MRs can be created for
   individual parts

3. Create MRs onto your feature branch. While in general your feature branch
   should compile for other to check, the policy can be relaxed with respect
   to merging to master if you clearly indicate this: e.g. you are refactoring
   something which breaks the python bindings, but would like to split C++ and
   binding refactoring into separate MRs.

4. When you feature branch is complete and consistent, create a MR to merge
   the feature branch into master.

What does WIP mean?
-------------------

WIP means work in progress and should be used to indicate MRs that you would
like to share with a selected group of people you work together with and get
their opinion. You should explicitly let the people know that they should
collaborate with you on the WIP MR. Everyone else can ignore WIP MRs,
if they have not been explicitly asked for review.


