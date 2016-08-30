
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
    # Includes are in a symlinked directory which has a bad link
    pushd $INSTALL_PREFIX/include
    rm python3.4 && ln -s python3.4m python3.4
    popd

    # Makefile has a 'prefix' which needs to be fixed
    pushd $INSTALL_PREFIX/lib/python3.4/config-3.4m
    local sed_program='s%^prefix=.*$%prefix='${INSTALL_PREFIX}'%'
    sed -i $sed_program Makefile
    popd

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

    local entry_points=(2to3 2to3-3.4 cygdb cython
        easy_install easy_install-3.4 f2py3 get_objgraph.py
        guidata-tests guiqwt-tests idle3 idle3.4
        ipcluster ipcluster3 ipcluster_watcher ipcontroller ipcontroller3
        ipengine ipengine3 iptest iptest3 ipython ipython3
        jupyter jupyter-kernelspec jupyter-migrate jupyter-nbextension
        jupyter-notebook jupyter-qtconsole jupyter-serverextension jupyter-trust
        nosetests nosetests-3.4 pip pip3 pip3.4 pnuke prsync pscp pslurp pssh
        pssh-askpass pydoc3 pydoc3.4 pygmentize pyvenv pyvenv-3.4
        py.test py.test-3.4 pybabel rpath-fixer rpath-missing rpath-show
        rst2html.py rst2latex.py rst2man.py rst2odt_prepstyles.py rst2odt.py
        rst2pseudoxml.py rst2s5.py rst2xetex.py rst2xml.py rstpep2html.py
        sift sphinx-apidoc sphinx-autogen sphinx-build sphinx-quickstart
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
    local target_packages=(Crypto Cython guiqwt h5py matplotlib numpy PIL
        PyQt4 scipy tornado traits zmq
    )

    # Relocate the Python packages
    local count=0
    while [ "x${target_packages[count]}" != "x" ]
    do
        local package=${target_packages[count]}
        local package_dir=$($INSTALL_PREFIX/bin/python3 -c "import os,$package as pkg;print(os.path.dirname(pkg.__file__))")
        $INSTALL_PREFIX/bin/rpath-fixer -f -g "*.so" -l $INSTALL_PREFIX/lib -d $package_dir
        count=$(($count + 1))
    done

    # These are specific libs which need to be relocated
    local target_libs=(libtiffxx.so libhdf5_hl.so libfreetype.so)

    # Relocate the libraries
    count=0
    while [ "x${target_libs[count]}" != "x" ]
    do
        local library=${target_libs[count]}
        $INSTALL_PREFIX/bin/rpath-fixer -f -g "$library" -l $INSTALL_PREFIX/lib -d $INSTALL_PREFIX/lib
        count=$(($count + 1))
    done

    # Relocate the executables
    $INSTALL_PREFIX/bin/rpath-fixer -f -l $INSTALL_PREFIX/lib -d $INSTALL_PREFIX/bin
}

# Do what we came here to do
INSTALL_PREFIX=$(get_abs_path $1)

# Fix the Python build
relocate_python

# Rewrite instances where the local install path is needed
rewrite_build_machine_paths

# Fix library and binary RPATHS
rewrite_rpaths
