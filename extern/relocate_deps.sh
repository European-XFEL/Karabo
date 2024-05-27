
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

    local entry_points=(bottle.py chardetect conan conan_build_info conan_server
        coverage coverage3 coverage-3.11 cygdb cython cythonize distro docutils
        flit fonttools f2py f2py3 f2py3.11 flake8 jsonschema
        get_objgraph ipcluster ipcluster3 ipcluster_watcher
	ipcontroller ipcontroller3 ipengine ipengine3 iptest iptest3 ipython
	ipython3 isort isort-identify-imports jupyter jupyter-dejavu jupyter-execute
	jupyter-kernel jupyter-kernelspec jupyter-migrate jupyter-nbconvert
	jupyter-nbextension jupyter-notebook jupyter-qtconsole jupyter-run
	jupyter-serverextension jupyter-trust jupyter-troubleshoot nosetests
	nosetests-3.4 normalizer pint-convert pip pip3 pip3.10 pip3.11
	pybind11-config pyftmerge pyftsubset pygmentize pnuke prsync pscp pslurp
	pyvenv pyflakes py.test pytest pybabel pycodestyle pwiz.py
	readelf.py rpath-fixer rpath-missing rpath-show rst2html.py rst2html4.py
	rst2html5.py rst2latex.py rst2man.py rst2odt_prepstyles.py rst2odt.py
	rst2pseudoxml.py rst2s5.py rst2xetex.py rst2xml.py rstpep2html.py
	sift sphinx-apidoc sphinx-autogen sphinx-build sphinx-quickstart
	tabulate tqdm ttx undill unpickle.py wheel
    )

    local count=0
    while [ "x${entry_points[count]}" != "x" ]
    do
        local script_path=$INSTALL_PREFIX/bin/${entry_points[count]}
        [ -f $script_path ] && sed -i "$sed_program" $script_path
        count=$(($count + 1))
    done
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
