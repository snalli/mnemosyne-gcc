#!/bin/sh
# -----------------------------------------------------------------------------
# Stop script for the CATALINA Server
#
# $Id: shutdown.sh,v 1.2 2006/03/19 19:36:28 nawilson Exp $
# -----------------------------------------------------------------------------

# Determine the user's current working directory and the path to this script.
USER_WORKING_DIRECTORY=`pwd`
cd `dirname "${0}"`
SCRIPT_DIR=`pwd`
cd ${USER_WORKING_DIRECTORY}
export USER_WORKING_DIRECTORY SCRIPT_DIR


# Set up the appropriate Java environment.
export JAVA_HOME JVM_ARGS SCRIPT_DIR USER_WORKING_DIRECTORY
. ${SCRIPT_DIR}/../tools/set-java-home.sh
if test "${JAVA_HOME_ERROR}" -ne 0
then
  exit 1
fi


# resolve links - $0 may be a softlink
PRG="$0"

while [ -h "$PRG" ] ; do
  ls=`ls -ld "$PRG"`
  link=`expr "$ls" : '.*-> \(.*\)$'`
  if expr "$link" : '/.*' > /dev/null; then
    PRG="$link"
  else
    PRG=`dirname "$PRG"`/"$link"
  fi
done

PRGDIR=`dirname "$PRG"`
EXECUTABLE=catalina.sh

# Check that target executable exists
if [ ! -x "$PRGDIR"/"$EXECUTABLE" ]; then
  echo "Cannot find $PRGDIR/$EXECUTABLE"
  echo "This file is needed to run this program"
  exit 1
fi

exec "$PRGDIR"/"$EXECUTABLE" stop "$@"
