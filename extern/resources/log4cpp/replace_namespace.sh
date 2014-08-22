#!/bin/bash

EXTS="cpp hh"
OLD="log4cpp"
NEW="krb_log4cpp"

# Rename header file subdir
mv include/${OLD} include/${NEW}

# Update makefiles
find . -type f -name Makefile -exec sed -i -r "s|\(includedir\)/${OLD}|\(includedir\)/${NEW}|" {} \;
sed -i -r "s|SUBDIRS = ${OLD}|SUBDIRS = ${NEW}|" include/Makefile

# Update source files
for ext in $EXTS; do
    
    # Update name of header file subdir
    find . -type f -name "*.${ext}" -exec sed -i -r "s/include +<${OLD}/include <${NEW}/g; s/include +\"${OLD}/include \"${NEW}/g" {} \;

    # Update namespace
    find . -type f -name "*.${ext}" -exec sed -i -r "s/namespace +${OLD}/namespace ${NEW}/g; s/${OLD}::/${NEW}::/g" {} \;
done