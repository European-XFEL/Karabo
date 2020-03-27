#!/bin/sh

# THIS IS A WORKAROUND DUE TO THE LACK OF automake in our CI images,
# we rely on cmake and uvloop offers a cmake build.
# This script will overwrite the delivered one that uses automake.

cmake .
make
# move the library where the setup.py expects it.
mkdir -p .libs
cp libuv_a.a .libs/libuv.a
# spoof the configure option
rm -f configure
echo "#!/bin/sh" > configure
echo "exit 0" >> configure
chmod u+x configure

exit 0