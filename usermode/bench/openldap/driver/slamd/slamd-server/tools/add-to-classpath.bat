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


set CP=%1
if ""%1"" == """" goto done
shift


:appendNext
if ""%1"" == """" goto done
set CP=%CP% %1
shift
goto appendNext


:done
set CLASSPATH=%CLASSPATH%;%CP%

