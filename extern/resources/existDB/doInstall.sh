#!/bin/bash

KARABO=$1
EXIST_BASE="$KARABO/extern/eXistDB"
EXIST_HOME="$KARABO/extern/eXistDB/db"
CURRENT_DIR=`pwd`

#check if java is installed
if type -p java; then
    JAVA=java
elif [[ -n "$JAVA_HOME" ]] && [[ -x "$JAVA_HOME/bin/java" ]];  then
    JAVA="$JAVA_HOME/bin/java"
else
    echo "JAVA not found! If it is installed, please set JAVA_HOME to the appropriate location!"
    exit 
fi

#first we check if the database is installed
if [ ! -d "$EXIST_HOME" ]; then
	cd $EXIST_BASE
	./install $JAVA
fi

#initialize the database
python &> /dev/null <<EOF
from karabo.project_db.util import assure_running, stop_database
assure_running()
stop_database()
EOF

cd $CURRENT_DIR
