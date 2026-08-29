# AGENTS.md

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

If the required environment cannot be activated, **stop and report the problem
**. Do not continue with another environment or attempt to work around the
failure without instructions.

## Testing

Run Karabo-backed tests **outside the file sandbox**.

The Framework activation script writes files under the user's home directory,
which may not be accessible from the file sandbox.
