# Restore previous value of OMP_NUM_THREADS if any

unset OMP_NUM_THREADS
if [[ -n "$_CONDA_SET_OMP_NUM_THREADS" ]]; then
    export OMP_NUM_THREADS=$_CONDA_SET_OMP_NUM_THREADS
    unset _CONDA_SET_OMP_NUM_THREADS
fi
