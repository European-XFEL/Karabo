.. _toolsRedmine:

***************
Tools - Redmine
***************

We are using a commercial clone of redmine, called EasyRedmine. It is available here:

`<https://in.xfel.eu/redmine>`_


Issue Relations
===============

'Related issues' allow developers to link issues to each other in order to remove duplicates or simplify their workflow.
It is possible to link issues based on various relations. Current relations are:

* related to - Just adds a link to the other issue.

* duplicates - Links issues so that closing one, will close the other (e.g. closing A will close B)
    For example, if issue B duplicates A:
    - closing B will leave A open
    - closing A will automatically close B

* duplicated by - Reciprocal of duplicates.
    For example, if issue A is duplicated by issue B:
    - closing B will leave A open
    - closing A will automatically close B

* blocks - Links issues so that closing one can be blocked by an issue which is still open
    If issue B blocks A,
    A can't be closed unless B is.
    blocked by - Reciprocal of blocks.

* precedes - Links issues to define an "order", where A needs to be completed x days before B can be started on
    If B follows A, you can't give B
    a starting date equal or less
    than the ending date of A.
    follows - Reciprocal of precedes.
    If issue B follows A (ex A ends the 21/04 and B begins the 22/04)
    and you add +2 day at the ending date of A,
    the starting and ending dates of B will be +2 too.

* copied from - Links issues to identify whether it was copied, and from which issue it was copied from.

* copied to - Reciprocal of copied from.

