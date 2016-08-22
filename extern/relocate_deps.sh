
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

rewrite_python_shebangs() {
    local new_shebang_line="#!/usr/bin/env python3"
    local sed_program='1 s%^.*$%'$new_shebang_line'%g'

    local entry_points=(2to3 2to3-3.4 cygdb cython
        easy_install easy_install-3.4 f2py3 guidata-tests guiqwt-tests
        idle3 idle3.4 ipcluster ipcluster3 ipcontroller ipcontroller3
        ipengine ipengine3 iptest iptest3 ipython ipython3
        nosetests nosetests-3.4 pip pip3 pip3.4 pnuke prsync pscp pslurp pssh
        pssh-askpass pydoc3 pydoc3.4 pygmentize pyvenv pyvenv-3.4 rst2html.py
        rst2latex.py rst2man.py rst2odt_prepstyles.py rst2odt.py rst2pseudoxml.py
        rst2s5.py rst2xetex.py rst2xml.py rstpep2html.py sift sphinx-apidoc
        sphinx-autogen sphinx-build sphinx-quickstart wheel
    )

    local count=0
    while [ "x${entry_points[count]}" != "x" ]
    do
        local script_path=$INSTALL_PREFIX/bin/${entry_points[count]}
        [ -f $script_path ] && sed -i "$sed_program" $script_path
        count=$(($count + 1))
    done

}

# Do what we came here to do
INSTALL_PREFIX=$(get_abs_path $1)

# Fix the Python build
relocate_python