# AGENTS.md

This file is located at the root of the repository.

Before making any changes, read and follow the instructions in this file:
`./AGENTS.md`

Any use of AI tools or agents when working on or interacting with Karabo must comply with our
[guidelines for using AI tools](https://karabo.readthedocs.io/en/latest/policy/).

> [!important]
> **Primary directive**: Read the policy before making or proposing any changes.

When acting on this repository, apply the principles:

- Consider whether the change is really necessary.
- Make surgical and focused changes.
- Follow existing coding style and patterns.
- Write tests that exercise the change.

## Development environments

This repository uses separate development environments for the **Karabo
Framework** and **karaboGui**. Use the environment corresponding to the
component you are working on.

### Karabo Framework

The repository contains the Framework environment in the root `karabo/`
directory.

Activate it with:

    bash -c 'source "karabo/activate"'

### karaboGui

For `karaboGui` development, activate the dedicated Conda environment:

    bash -c 'conda activate karabogui'

### Environment requirement

If the required environment cannot be activated, **stop and report the problem**.
Do not continue with another environment or attempt to work around the failure
without instructions.

## Testing

Run Karabo-backed tests **outside the file sandbox**.

The Framework activation script writes files under the user's home directory,
which may not be accessible from the file sandbox.
