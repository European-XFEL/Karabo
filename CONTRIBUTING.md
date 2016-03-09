Contributing Code to Karabo Framework
-------------------------------------

Before beginning:

* Install git if your computer doesn't have it.
* Clone this repository:
  * SSH: `$ git clone ssh://git@git.xfel.eu:10022/Karabo/Framework.git`
  * HTTPS: `$ git clone https://in.xfel.eu/gitlab/Karabo/Framework.git`

When Developing features or bux fixes:

* Check out the branch where your changes will eventually land
* Update to the most recent version of that branch: `$ git pull`
* Create a feature branch: `$ git branch <branchname>`
* Check that branch out: `$ git checkout <branchname>`
* Make changes to the code
* Commit those changes with: `$ git add <changed and/or new files>; git commit`
* Push your feature branch to GitLab: `$ git push origin <branchname>`
* Open a Merge Request on GitLab
* Your code will then be reviewed. This might include more commits and pushes
  from you.
* After review, the branch will be merged on GitLab
* GOTO 1
