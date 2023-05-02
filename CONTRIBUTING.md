Contributing Code to Karabo Framework
=====================================

External contributors, i.e. anyone not contractually associated to
the European XFEL GmbH, are asked to sign a Contributor License
Agreement (CLA):

- people contributing as individuals should sign the Individual CLA
- people contributing on behalf of an organization should sign
  the Entity CLA.

The CLAs can be found in the `Contributor_License_Agreement.md` and
`Entity_Contributor_License_Agreement.md` documents located in
the root folder of this repository.
Please send signed CLAs to opensource [at] xfel.eu. We'll get in
touch with you then.
We ask for your understanding that because Karabo is critical to perform
outstanding science at the European XFEL, we cannot accept external
contributions without a CLA in place. Importantly, with signing the CLA
you acknowledge that

* European XFEL retains all copyrights of Karabo,
* European XFEL may relicense Karabo under other appropriate open source licenses
  which the Free Software Foundation classifies as Free Software licenses.

However, you are welcome to already
suggest modifications you'd like to contribute by opening a merge/pull
request before you send the CLA.

Internal contributors, i.e. anyone contractually associated to the
European XFEL, can find guidelines on internal contributions in
the Karabo Framework Redmine Wiki:
https://in.xfel.eu/redmine/projects/karabo-library/wiki/Developing_the_framework_using_git

Additionally, everyone is asked to follow these guidelines:

Gitlab
======

Large(r) merge requests:
------------------------

There is a recurrent tradeoff between „complete“ merge requests,
e.g. MRs fully implementing and defining a larger addition to the framework,
and smaller, often easier to review, merge requests. The latter
would mean to break the additions into smaller chunks, frequently
not fully functional on their own. A solution is to merge to separate
feature branches:

1. Create a new (longer lived) feature branch for your larger project

2. Break down your larger project, so that smaller MRs can be created for
   individual parts

3. Create MRs onto your feature branch. While in general your feature branch
   should compile for other to check, the policy can be relaxed with respect
   to merging to master if you clearly indicate this: e.g. you are refactoring
   something which breaks the python bindings, but would like to split C++ and
   binding refactoring into separate MRs.

4. When your feature branch is complete and consistent, create a single MR to merge
   the feature branch into master, and indicate that is has been reviewed
   in smaller increments already.

What does WIP/Draft mean?
-------------------------

WIP, or Draft, means work in progress and should be used to indicate MRs that you would
like to share with a selected group of people you work together with and get
their opinion. You should explicitly let the people know that they should
collaborate with you on the WIP MR. Everyone else can ignore WIP MRs,
if they have not been explicitly asked for review.

Documentation
=============

All methods added to the public interface should be documented and covered by
unit tests. All private methods should be documented such that another
*developer* can understand their intention and logic. When refactoring, consider
adding documentation to existing code directly used or touched during your
refactoring.

C++ Standard
============

C++14 usage is supported for framework code. However, to support legacy packages,
the C++ headers of Karabo currently do not use any C++14 features and allow
a package (e.g. a device) to be compiled using the C++11 standard.
It is however discouraged to build Karabo C++ Packages using the C++11 standard.

Additionally, the following guidelines are suggested:

- Feel free to use new features where they make sense. E.g. use auto to shorten
  iterator syntax in loops, e.g.
  `std::map<MyComplexType, MyMoreComplexType<double> >::const_iterator it = foo.begin() -> auto it = foo.begin()`.

- Don’t use `auto` to indicate straight forward types, e.g. `auto i = 4;`

- Existing code does not need to be refactored for C++11 feature usage alone.
  E.g. if you happen to refactor something anyway, feel free to replace iterators
  with `auto` if it aids readability. You do not specifically have to refactor
  otherwise working code though.

- Do **not** use `std::shared_ptr`, we will continue to use `boost::shared_ptr`!

- In general, if a `boost` and a `std`-library feature coexist
  (smart pointers, mutices, bind, etc.), continue to use the boost implementation
  as we have done previously, especially if there is a risk that your new code
  needs to interact with existing code.

- When using more „advanced“ features, like late return type declaration
  (`->decltype(foo)`), variadic templates or reference forwarding, add a short
  comment to these lines to aid people less experienced with C++11 features in
  the review.

- We currently do not encourage to use C++11-introduced numerical types, e.g.
  `uint64_t` as the Karabo type system has not been fully prepared for them.


Python
======

General - Tools
---------------

- First and foremost, programming should be done *PEP8* conform.
  Additionally, if there is no *PEP8* definition and for further examples, the
  `google style guide <https://google.github.io/styleguide/pyguide.html>`_
  can be looked up.
- Per convention, `strings` should be created with `"`.
- Per convention, `lists` and `dicts` should should be created with [], {}, respectively.
  Tools like *flake8* can help in writing clean code.
- Import ordering is done using *isort -m4*. A corresponding *isort* configuration file
  is provided in `src/pythonKarabo` and `src/pythonGui`
- Every new feature must be supplied with a unit-test and if necessary, with an integration
  test.

Documentation
-------------

- Use double backticks (\`\`name\`\`) to document variables inside a method


Python Classes
--------------

- Per convention, deriving from a super class and call methods should be done
  like this (>= Python 3.6):

       `super()`

- Do not use names of builtins for variables (e.g. `object`)


Karabo GUI
==========

- By design, we sort out functionality to controllers based on *traits* package for type checking,
  user interface library independent testing and event signals.
- New style features of Qt > 5.9 should not be used until further notice.
- The `Karabo GUI` must run on different OS: MacOS, Windows and Unix system.
  Hence, the library *pathlib* should be used whenever os path functionality has to be used.
- The Qt version of the `Karabo GUI` is provided by the community, e.g. we use packages like *qtpy* and the conda qt-feedstock
  to run. However, only the *PyQt* package is tested and supported. *PySide* is best-effort.
- Plot features should be implemented using *PyQtGraph*.
