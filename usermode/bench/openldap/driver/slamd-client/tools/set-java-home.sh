#!/bin/sh

###############################################################################
#                             Sun Public License
#
# The contents of this file are subject to the Sun Public License Version
# 1.0 (the "License").  You may not use this file except in compliance with
# the License.  A copy of the License is available at http://www.sun.com/
#
# The Original Code is the SLAMD Distributed Load Generation Engine.
# The Initial Developer of the Original Code is Neil A. Wilson.
# Portions created by Neil A. Wilson are Copyright (C) 2004-2010.
# Some preexisting portions Copyright (C) 2002-2006 Sun Microsystems, Inc.
# All Rights Reserved.
#
# Contributor(s):  Neil A. Wilson
###############################################################################


# Uncomment the next two lines and edit the path as appropriate to specify the
# path to the Java installation to use for the SLAMD client and associated
# tools.
#JAVA_HOME=/usr/java
#export JAVA_HOME


#
# You should not need to edit anything below this point.
#


# Determine the location to this script.
USER_WORKING_DIR=`pwd`
cd "`dirname "${0}"`"
SJH_SCRIPT_DIR=`pwd`
cd "${USER_WORKING_DIR}"


# Determine whether this is a SLAMD server or client installation and use that
# to set the appropriate locations.
if test -d "${SJH_SCRIPT_DIR}/../webapps/slamd/WEB-INF"
then
  BASE_DIR="${SJH_SCRIPT_DIR}/../webapps/slamd/WEB-INF"
else
  if test -d "${SJH_SCRIPT_DIR}/../lib"
  then
    BASE_DIR="${SJH_SCRIPT_DIR}/.."
  else
    BASE_DIR="${SJH_SCRIPT_DIR}"
  fi
fi


# Set the correct CLASSPATH to use.
CLASSPATH="${CLASSPATH}:${BASE_DIR}/classes"
for JAR in "${BASE_DIR}/lib/"*.jar
do
  CLASSPATH="${CLASSPATH}:${JAR}"
done
export CLASSPATH


# If no JAVA_HOME value is set, then try to figure out a correct value from the
# user's PATH.
if test -z "${JAVA_CMD}"
then
  if test -z "${JAVA_HOME}"
  then
    JAVA_HOME=`java com.slamd.tools.FindJavaHome`
    if test -z "${JAVA_HOME}"
    then
      echo "ERROR:  Unable to determine the path to the Java environment to"
      echo "use.  Please set the JAVA_HOME environment variable or edit the"
      echo "${SJH_SCRIPT_DIR}/set-java-home.sh"
      echo "script to specify the appropriate JAVA_HOME value."
      JAVA_HOME_ERROR=1
      export JAVA_HOME_ERROR
      exit 1
    fi
  fi

  JAVA_CMD="${JAVA_HOME}/bin/java"
  if test -f "${JAVA_CMD}"
  then
    export JAVA_HOME JAVA_CMD
  else
    echo "ERROR:  Java command ${JAVA_CMD}"
    echo "does not exist.  Please set the JAVA_HOME environment variable or"
    echo "edit the ${SJH_SCRIPT_DIR}/set-java-home.sh"
    echo "script to specify the appropriate JAVA_HOME value."
      JAVA_HOME_ERROR=1
      export JAVA_HOME_ERROR
      exit 1
  fi
fi

JAVA_HOME_ERROR=0
export JAVA_HOME_ERROR

