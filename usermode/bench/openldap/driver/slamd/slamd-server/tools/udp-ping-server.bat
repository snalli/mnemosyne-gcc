@echo off

rem ###########################################################################
rem #                             Sun Public License
rem #
rem # The contents of this file are subject to the Sun Public License Version
rem # 1.0 (the "License").  You may not use this file except in compliance with
rem # the License.  A copy of the License is available at http://www.sun.com/
rem #
rem # The Original Code is the SLAMD Distributed Load Generation Engine.
rem # The Initial Developer of the Original Code is Neil A. Wilson.
rem # Portions created by Neil A. Wilson are Copyright (C) 2004-2010.
rem # Some preexisting portions Copyright (C) 2002-2006 Sun Microsystems, Inc.
rem # All Rights Reserved.
rem #
rem # Contributor(s):  Neil A. Wilson
rem ###########################################################################


rem Specify the arguments that should be provided to the JVM when running this
rem tool.
set JVM_ARGS=


rem
rem You should not need to edit anything below this point.
rem


rem Get the directory containing this batch file.
set BATDIR=%~dp0


rem Prepare the Java environment.
call "%BATDIR%set-java-home.bat"
if not %JAVA_HOME_ERROR% == 0 exit /B %JAVA_HOME_ERROR%


rem Invoke the tool.
set TOOL_CLASS=com.slamd.tools.udpping.UDPPingServer
"%JAVA_CMD%" %JVM_ARGS% %TOOL_CLASS% %*
goto end


:end

