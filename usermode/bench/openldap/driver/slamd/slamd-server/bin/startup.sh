#!/bin/sh
# -----------------------------------------------------------------------------
# Start Script for the CATALINA Server
#
# $Id: startup.sh,v 1.2 2006/03/19 19:36:28 nawilson Exp $
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


# Specify the set of arguments to provide to the JVM.
CATALINA_OPTS="-server -Xms512m -Xmx512m -Djava.awt.headless=true"
export CATALINA_OPTS


# Better OS/400 detection: see Bugzilla 31132
os400=false
darwin=false
case "`uname`" in
CYGWIN*) cygwin=true;;
OS400*) os400=true;;
Darwin*) darwin=true;;
esac

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
if $os400; then
  # -x will Only work on the os400 if the files are:
  # 1. owned by the user
  # 2. owned by the PRIMARY group of the user
  # this will not work if the user belongs in secondary groups
  eval
else
  if [ ! -x "$PRGDIR"/"$EXECUTABLE" ]; then
    echo "Cannot find $PRGDIR/$EXECUTABLE"
    echo "This file is needed to run this program"
    exit 1
  fi
fi

exec "$PRGDIR"/"$EXECUTABLE" start "$@"
