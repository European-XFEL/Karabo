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

Everyone is asked to follow these guidelines:

Gitlab
======

Git Workflow
-------------

All new features and fixes should be developed against the *master*/*main*
branch. Afterwards, fixes may be backported to a release branch that usually
follows the format `<MAJOR>.<MINOR>.X-hotfix`.

- Checkout the targeted branch (usually master/main) and `git pull`.
- Switch to a development branch, e.g. `git checkout -b <branchname>`
- Edit code, `add`, `commit` and finally `push` that branch.
- Create  a merge request in GitLab using the url that the output of `push`
  provides. Do not forget to provide a meaningful description
  and a test for the fix or the new feature.
- The merge request title should be prefixed with type and scope following
  [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) where
  the scope usually is the API that the merge requests updates:
  `feat(C++)`, `fix(Bound)`, `refactor(MDL)`, `perf(GUI)`...
  - If it is a backport, please indicate that in the title.
- Kindly address review comments by pushing updates or commenting why that
  would not be a good idea.
- Keep an eye on the tests that are run by the GitLab's Continuous Integration
  (CI). The CI first checks for formatting compliance (linting) - you may ease
  your life by enabling git pre-commit checks, see below.
- Once a reviewer comments with LGTM ("Looks good to me") and no comments of
  another reviewer are open, the author of the merge request shall merge the
  merge request.
  - Please edit the merge commit message prepared by GitLab my removing the
    line "Merge branch 'XXXX' into 'YYYY'".
    The first line should be a copy of the merge request title.


Formatting compliance and Pre-commit
-------------------------------------

In order to have your commits accepted by the CI's linting jobs, your modified
Python source files must be compliant with `flake8` and `isort` and your
modified C++ source files must be compliant with the `clang-format` settings in
the `.clang-format` file at the root of the repository.

The CI linting job (.pre-commit-config.yaml) currently uses `clang-format 17.0.6`.
Instructions on how to setup a development system based
on Visual Studio Code can be found at
[doc/tools/vscode.rst](https://karabo.pages.xfel.eu/Framework/tools/vscode.html) in this repository.

To detect non-compliances even before pushing to GitLab, developers are
encouraged to setup the `pre-commit` functionality. To do so, Karabo has to be
built and its environment activated. Then install via

    pip install pre-commit
    pre-commit install

The first step installs `pre-commit` in the activated Karabo environment and
thus has to be redone whenever Karabo is built again from scratch.
The latter step creates the necessary hook for git (`.git/hooks/pre-commit`).
The `pre-commit` configuration can be found at `.pre-commit-config.yaml`.

Whenever one commits, the staged files will now be checked. If any
non-compliance is detected, the commit is aborted. The offending files are
corrected, but these changes are not yet staged for commit. That has to be
done by hand before the next try to commit.


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

WIP, or Draft, as prefix of a merge request title
means work in progress and should be used to indicate MRs that you would
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

Since Karabo 2.20, C++17 usage is supported for framework code.
Any depending packages should follow and upgrade to C++17 since it cannot be
guaranteed that C++17 features are avoided in framework header files.

Additionally, the following guidelines are suggested:

- Feel free to use new features where they make sense. E.g. use auto to shorten
  iterator syntax in loops, e.g.
  `std::map<MyComplexType, MyMoreComplexType<double> >::const_iterator it = foo.begin() -> auto it = foo.begin()`.

- Don’t use `auto` to indicate straight forward types, e.g. `auto i = 4;`

- Existing code does not need to be refactored for C++11 feature usage alone.
  E.g. if you happen to refactor something anyway, feel free to replace iterators
  with `auto` if it aids readability. You do not specifically have to refactor
  otherwise working code though.

- Except if in well contained parts of the code, do **not** use
  `std::shared_ptr`, but continue to use `boost::shared_ptr`.

- In general, if a `boost` and a `std`-library feature coexist
  (smart pointers, mutexes, bind, etc.), continue to use the boost implementation
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

Coding Style
-------------

- Code should be *isorted* and style-checked using *flake8*. An isort configuration file
  is provided in `src/pythonKarabo`.
- Double quotes ("") should be used around strings instead of single quotes('') and triple-double
  quotes (""") should be used instead of triple-single quotes (''').
- We prefer hanging indentation.
- `super()` should always be called without explicitly passing the class object.
  For example, call ```super().foo(1, 2)``` instead of ```super(Foo, self).foo(1, 2)```


Naming conventions:

- Classes should be named in PascalCase and methods and instance variables
  should be in snake_case, except when overriding a third-party
  method/variable.
- Always spell out the variable without truncating, e.g. `server` instead of `srv`.
- Do not use names of builtins for variables (e.g. `object` and`list`)


Documentation
-------------
- Always try to add a docstring for new code, including tests, unless the name
  of the class/method/function is self-explanatory. Overridden methods should
  have a docstring to mention how they behave differently from the parent
  method.
- Use double backticks (\`\`name\`\`) to document variables inside a method


Testing
-------
- Every new feature must be supplied with a unit-test and also with an integration
  test, if necessary.


Karabo GUI
==========

See the contribution guidelines to Karabo GUI [here](src/pythonGui/CONTRIBUTING.md)
