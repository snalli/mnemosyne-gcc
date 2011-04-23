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


rem Specify the path to the Java installation to use for the SLAMD client.
set JAVA_HOME=c:\java


rem
rem You should not need to edit anything below this point.
rem


rem Ensure that the JAVA_HOME value is set properly.
set JAVA_CMD=%JAVA_HOME%\bin\java.exe
if not exist "%JAVA_CMD%" goto badJavaHome


rem Determine the location to this batch file.
set SJH_BATDIR=%~dp0


rem Determine whether this is a SLAMD server or client installation and use
rem that to set the appropriate locations.
if not exist "%SJH_BATDIR%..\webapps\slamd\WEB-INF" goto clientInstallation
goto serverInstallation


:clientInstallation
set CLASSPATH=%CLASSPATH%;%SJH_BATDIR%..\classes
for %%F in ("%SJH_BATDIR%..\lib\*.jar") do call "%SJH_BATDIR%add-to-classpath.bat" %%F
goto end


:serverInstallation
set CLASSPATH=%CLASSPATH%;%SJH_BATDIR%..\webapps\slamd\WEB-INF\classes
for %%F in ("%SJH_BATDIR%..\webapps\slamd\WEB-INF\lib\*.jar") do call "%SJH_BATDIR%add-to-classpath.bat" %%F
goto end


:badJavaHome
echo ERROR:  Unable to determine the path to the Java environment to use.
echo Please set the JAVA_HOME environment variable or edit the
echo tools\set-java-home.bat file to specify the appropriate JAVA_HOME value.
set JAVA_HOME_ERROR=1
exit /B %JAVA_HOME_ERROR%


:end
set JAVA_HOME_ERROR=0

