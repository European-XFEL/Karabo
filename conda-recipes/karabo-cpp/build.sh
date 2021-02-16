export KARABO=${PREFIX}
export PY_LIB=$(python -c "import sys; print(f'-l python{sys.version_info.major}.{sys.version_info.minor}m')")

# build karabo
rm -fr ${SRC_DIR}/cmake-${PKG_NAME}/*
mkdir -p ${SRC_DIR}/cmake-${PKG_NAME}
cd ${SRC_DIR}/cmake-${PKG_NAME}
CXXFLAGS="" \
echo "> conda-recipes/karabo-cpp/build.sh: cmake command: cmake -DCMAKE_PREFIX_PATH=${CONDA_PREFIX} -DCMAKE_INSTALL_PREFIX=${PREFIX} [TEST_BUILDING FLAGS] ${SRC_DIR}"
cmake \
  -DCMAKE_PREFIX_PATH=${CONDA_PREFIX}\
  -DCMAKE_INSTALL_PREFIX=${PREFIX}\
  -DBUILD_UNIT_TESTING=1\
  -DBUILD_INTEGRATION_TESTING=0\
  -DCMAKE_BUILD_TYPE=Release\
  ${SRC_DIR}

make -j${CPU_COUNT} || exit $?
make install || exit $?
# run tests
ctest -VV

ACTIVATE_DIR=${PREFIX}/etc/conda/activate.d
DEACTIVATE_DIR=${PREFIX}/etc/conda/deactivate.d

mkdir -p ${ACTIVATE_DIR}
mkdir -p ${DEACTIVATE_DIR}
mkdir -p ${PREFIX}/var/data
mkdir -p ${PREFIX}/var/log
mkdir -p ${PREFIX}/plugins

# Why do we put files in here? is it necessary for the packaging?
touch ${PREFIX}/var/data/.data_folder
touch ${PREFIX}/var/log/.log_folder
# touch ${PREFIX}/plugins/.plugins

cp -r ${RECIPE_DIR}/scripts/activate.sh ${ACTIVATE_DIR}/karabo-activate.sh
cp -r ${RECIPE_DIR}/scripts/deactivate.sh ${DEACTIVATE_DIR}/karabo-deactivate.sh

if [ ! -d "${PREFIX}/var/environment" ]; then
    cp -r ${SRC_DIR}/src/environment.in "${PREFIX}/var/environment"
fi

echo "${PKG_VERSION}" > "${PREFIX}/VERSION"
