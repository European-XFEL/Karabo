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

Documentation
=============

All methods added to the public interface should be documented and covered by 
unit tests. All private methods should be documented such that another 
*developer* can understand their intention and logic. When refactoring, consider
adding documentation to existing code directly used or touched during your 
refactoring.

C++11
=====

C++11 usage is now (officially) supported for framework code. The following
guidelines are suggested:

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
  
- We currently do not encourage to use newly introduced numerical types, e.g. 
  `uint64_t` as the Karabo type system has not been fully prepared for them.

Python
======

Tools
-----

We program *PEP8* conform. Tools like *flake8* help in writing clean code.

Documentation
-------------

 * Use double backticks (\`\`name\`\`) to document variables inside a method

Python Classes
--------------

 * Per convention, deriving from a super class and call methods should be done
   like this:
 
       `super(className, self)`
   
 * Do not use names of builtins for variables (e.g. `object`)

