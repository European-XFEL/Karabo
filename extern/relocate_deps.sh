
get_abs_path() {
    local parent_dir=$(dirname "$1")
    local _basename=$(basename "$1")
    case $_basename in
    ..)
        cd "$1"
        local abs_path="$(pwd -P)"
        ;;
    *)
        cd "$parent_dir"
        local abs_path="$(pwd -P)"/"$_basename"
    esac
    cd - >/dev/null
    echo "$abs_path"
}

relocate_python() {
    
    # Makefile has a 'prefix' which needs to be fixed
    local sed_program='s%^prefix=.*$%prefix='${INSTALL_PREFIX}'%'
    sed -i $sed_program $($INSTALL_PREFIX/bin/python -c "import sysconfig; print(sysconfig.get_makefile_filename())")

    # Entry point shebangs are all wrong
    rewrite_python_shebangs
}

rewrite_build_machine_paths() {
    # The bin directory
    $INSTALL_PREFIX/bin/python3 ./builder_path_replace.py --fix-links $INSTALL_PREFIX/bin __KARABO_CI_PATH__ $INSTALL_PREFIX
    # The lib directory
    $INSTALL_PREFIX/bin/python3 ./builder_path_replace.py $INSTALL_PREFIX/lib __KARABO_CI_PATH__ $INSTALL_PREFIX
}

rewrite_python_shebangs() {
    local new_shebang_line="#!/usr/bin/env python3"
    local sed_program='1 s%^.*$%'$new_shebang_line'%g'

    local entry_points=(2to3 2to3-3.8 cygdb cython
	easy_install easy_install-3.8 f2py3 flake8 get_objgraph.py
    idle3 idle3.8 ipcluster ipcluster3 ipcluster_watcher
	ipcontroller ipcontroller3 ipengine ipengine3 iptest iptest3 ipython
	ipython3 jupyter jupyter-kernelspec jupyter-migrate
	jupyter-nbextension jupyter-notebook jupyter-qtconsole
	jupyter-serverextension jupyter-trust nosetests nosetests-3.8 pip pip3
	pip3.8 pnuke prsync pscp pslurp pydoc3 pydoc3.8
	pygmentize pyvenv pyvenv-3.8 pyflakes py.test py.test-3.8 pybabel
	pycodestyle rpath-fixer rpath-missing rpath-show rst2html.py
	rst2latex.py rst2man.py rst2odt_prepstyles.py rst2odt.py
	rst2pseudoxml.py rst2s5.py rst2xetex.py rst2xml.py rstpep2html.py sift
	sphinx-apidoc sphinx-autogen sphinx-build sphinx-quickstart
	unpickle.py wheel
    )

    local count=0
    while [ "x${entry_points[count]}" != "x" ]
    do
        local script_path=$INSTALL_PREFIX/bin/${entry_points[count]}
        [ -f $script_path ] && sed -i "$sed_program" $script_path
        count=$(($count + 1))
    done
}

rewrite_rpaths() {
    # These are the specific Python packages which need to be relocated
    local target_packages=(Cython h5py lxml numpy PIL
        psutil scipy tornado traits zmq
    )

    # Relocate the Python packages
    local count=0
    while [ "x${target_packages[count]}" != "x" ]
    do
        local package=${target_packages[count]}
        local package_dir=$(LD_LIBRARY_PATH=$INSTALL_PREFIX/lib $INSTALL_PREFIX/bin/python3 -c "import os,$package as pkg;print(os.path.dirname(pkg.__file__))")
        run_rpath_fixer "-f -g '*.so' -l $INSTALL_PREFIX/lib -d $package_dir"
        count=$(($count + 1))
    done

    # These are specific libs which need to be relocated
    local target_libs=(libtiffxx.so libhdf5_hl.so libfreetype.so libxslt.so libexslt.so libxml2mod.so libzmq.so)

    # Relocate the libraries
    count=0
    while [ "x${target_libs[count]}" != "x" ]
    do
        local library=${target_libs[count]}
        run_rpath_fixer "-f -g '$library' -l $INSTALL_PREFIX/lib -d $INSTALL_PREFIX/lib"
        count=$(($count + 1))
    done

    # Relocate the executables
    export ORIGIN=$INSTALL_PREFIX/bin
    # can't fix python if python is running the fixer script
    run_rpath_fixer "-f -g '[!python]*' -l $INSTALL_PREFIX/lib -d $INSTALL_PREFIX/bin"
    # now fix python
    safeRunCommand "$INSTALL_PREFIX/bin/patchelf --set-rpath $INSTALL_PREFIX/lib $INSTALL_PREFIX/bin/python"
    # run a second time forcing to RUNPATH as well: https://stackoverflow.com/questions/43616505/setting-rpath-for-python-not-working
    safeRunCommand "$INSTALL_PREFIX/bin/patchelf --force-rpath --set-rpath $INSTALL_PREFIX/lib $INSTALL_PREFIX/bin/python"
}

run_rpath_fixer() {
    safeRunCommand "PATH=$INSTALL_PREFIX/bin $INSTALL_PREFIX/bin/python3 -m rpathology.fixer $*"
}

safeRunCommand() {
    typeset cmnd="$*"
    typeset ret_code

    echo cmnd=$cmnd
    eval $cmnd
    ret_code=$?
    if [ $ret_code != 0 ]; then
        printf "Error : [%d] when executing command: '$cmnd'" $ret_code
        exit $ret_code
    fi
}


# Do what we came here to do
INSTALL_PREFIX=$(get_abs_path $1)

# Fix the Python build
relocate_python

# Rewrite instances where the local install path is needed
rewrite_build_machine_paths

# Fix library and binary RPATHS
rewrite_rpaths
