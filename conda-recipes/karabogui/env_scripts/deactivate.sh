# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
# Restore previous value of OMP_NUM_THREADS if any

unset OMP_NUM_THREADS
if [[ -n "$_CONDA_SET_OMP_NUM_THREADS" ]]; then
    export OMP_NUM_THREADS=$_CONDA_SET_OMP_NUM_THREADS
    unset _CONDA_SET_OMP_NUM_THREADS
fi

if [[ -n "$_CONDA_UNSET_LANG" ]]; then
    unset LANG
    unset _CONDA_UNSET_LANG
fi
