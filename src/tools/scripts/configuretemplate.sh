#!/bin/bash

toUpper() {
    local char="$*"
    out=$(echo $char | tr [:lower:] [:upper:])
    local retval=$?
    echo "$out"
    unset out char
    return $retval
}

# This function configures the default template 
# @param $1 packagePath    Full path to the current package
# @param $2 packageName    Package name
# @param $3 className      Class name
# @param $4 templateName   Template name (fsmDevice, device, ...)
packagePath=$1
packageName=$2
className=$3 
templateName=$4

cdate=$(date +"%B, %Y, %I:%M %p")
upperClassName=$(toUpper $className)
directoriesToRename=$(find $1 -type d)

for d in $directoriesToRename
do
    newname=`echo $d | sed s#__PACKAGE_NAME__#"${packageName}"#g`
    if [[ "$d" != "$newname" && -e "$d" ]]; then
        mv $d $newname
    fi
done

filesToReplace=$(find $1 -type f -name '*.*')
for file in $filesToReplace
do
    if [ ! -d ${file} ]; then
        sed -i -e s#__DATE__#"${cdate}"#g \
        -e s#__EMAIL__#"${email}"#g \
        -e s#__PACKAGE_NAME__#"${packageName}"#g \
        -e s#__CLASS_NAME__#"${className}"#g \
        -e s#__CLASS_NAME_ALL_CAPS__#"${upperClassName}"#g ${file}
    fi

    newname=`echo $file | sed s#__CLASS_NAME__#"${className}"#g`
    if [ "$file" != "$newname" ]; then
        mv $file $newname
    fi
done
