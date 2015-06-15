#!/bin/bash

EXTS="cpp hh h"
OLD="log4cpp"
NEW="krb_log4cpp"

# To uppercase...
OLD_UP=`echo $OLD | tr '[:lower:]' '[:upper:]'`
NEW_UP=`echo $NEW | tr '[:lower:]' '[:upper:]'`

if [ -d include/${OLD} ]; then

    if [ -d include/${NEW} ]; then
        # Remove header file subdir (left from previous script call)
	rm -r include/${NEW}
    fi

    # Rename header file subdir
    mv include/${OLD} include/${NEW}
fi


if [ "$(uname -s)" == "Darwin" ]; then

# Update makefiles
find . -type f -name Makefile -exec sed -i "" -E "s|\(includedir\)/${OLD}|\(includedir\)/${NEW}|" {} \;
sed -i "" -E "s|SUBDIRS = ${OLD}|SUBDIRS = ${NEW}|" include/Makefile

# Update source files
for ext in $EXTS; do
    
    # Update name of header file subdir
    find . -type f -name "*.${ext}" -exec sed -i "" -E "s|include +<${OLD}|include <${NEW}|g; s|include +\"${OLD}|include \"${NEW}|g" {} \;

    # Update namespace
    find . -type f -name "*.${ext}" -exec sed -i "" -E "s|namespace +${OLD}|namespace ${NEW}|g; s|${OLD}::|${NEW}::|g" {} \;

    # Rename macros
    find . -type f -name "*.${ext}" -exec sed -i "" -E "s|${OLD_UP}|${NEW_UP}|g" {} \;
done

else

find . -type f -name Makefile -exec sed -i -r "s|\(includedir\)/${OLD}|\(includedir\)/${NEW}|" {} \;
sed -i -r "s|SUBDIRS = ${OLD}|SUBDIRS = ${NEW}|" include/Makefile

# Update source files
for ext in $EXTS; do
    
    # Update name of header file subdir
    find . -type f -name "*.${ext}" -exec sed -i -r "s|include +<${OLD}|include <${NEW}|g; s|include +\"${OLD}|include \"${NEW}|g" {} \;

    # Update namespace
    find . -type f -name "*.${ext}" -exec sed -i -r "s|namespace +${OLD}|namespace ${NEW}|g; s|${OLD}::|${NEW}::|g" {} \;

    # Rename macros
    find . -type f -name "*.${ext}" -exec sed -i -r "s|${OLD_UP}|${NEW_UP}|g" {} \;
done

fi
