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


# Uncomment the next line in order to explicitly specify which Java environment
# should be used.
#JAVA_HOME=/usr/java


# Specify the arguments that should be provided to the JVM when running this
# tool.
JVM_ARGS=""


#
# You should not need to edit anything below this line.
#


# Determine the user's current working directory and the path to this script.
USER_WORKING_DIRECTORY=`pwd`
cd "`dirname "${0}"`"
SCRIPT_DIR=`pwd`
cd "${USER_WORKING_DIRECTORY}"
export USER_WORKING_DIRECTORY SCRIPT_DIR


# Set up the appropriate Java environment.
export JAVA_HOME JVM_ARGS SCRIPT_DIR USER_WORKING_DIRECTORY
. "${SCRIPT_DIR}/set-java-home.sh"
if test "${JAVA_HOME_ERROR}" -ne 0
then
  exit 1
fi


# Invoke the tool.
TOOL_CLASS="com.slamd.tools.tcpreplay.TCPCapture"
exec "${JAVA_CMD}" ${JVM_ARGS} ${TOOL_CLASS} "${@}"

