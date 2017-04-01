# This is the main configuration class. All others inherit this.
#

import os
import string
import SCons.Environment
from SCons.Script import ARGUMENTS
from SCons.Variables import Variables, EnumVariable, BoolVariable
import helper


class Environment(SCons.Environment.Environment):
	"""
		A specialization of the SCons Environment class which does some particular
		munging of the build variables needed in the source files.
	"""
	
	def __init__(self, mainEnv, configuration_name = 'default'):
		"""
			Applies the definitions in configuration_name to generate a correct
			environment (specificall the set of compilation flags() for the
			TinySTM library. That environment is returned
		"""
		SCons.Environment.Environment.__init__(self)

		# Generate build directives 
		mnemosyneDirectives = helper.Directives(configuration_name, 'mnemosyne', ARGUMENTS, self._mnemosyne_boolean_directive_vars, self._mnemosyne_enumerable_directive_vars, self._mnemosyne_numerical_directive_vars)

		configuration_variables = Environment._GetConfigurationVariables(self)
		configuration_variables.Update(self)
	
		self.Append(CPPDEFINES = mnemosyneDirectives.getPreprocessorDefinitions())

        	# Disable some remark messages when using Intel CC
		# We don't use ICC anymore but GCC only.
		if mainEnv is not None and mainEnv['CC'] == 'icc': 
			DISABLE_WARNINGS = ['-wd869',  #remark  #869 : parameter "XYZ" was never referenced
                               ]

			self.Append(CCFLAGS = string.join(DISABLE_WARNINGS, ' '))


		# Inherit some command line options from the main environment
		if mainEnv is not None:
			self['BUILD_LINKAGE'] = mainEnv['BUILD_LINKAGE'] 
			self['BUILD_DEBUG'] = mainEnv['BUILD_DEBUG'] 
			self['BUILD_STATS'] = mainEnv['BUILD_STATS'] 
			self['MY_ROOT_DIR'] = mainEnv['MY_ROOT_DIR'] 
			self['MY_LINKER_DIR'] = mainEnv['MY_LINKER_DIR'] 
			self['CC'] = mainEnv['CC'] 
			self['CXX'] = mainEnv['CXX'] 
            
		if mainEnv is not None and mainEnv['VERBOSE'] == False:
			self.Replace(
			                 CCCOMSTR = '(COMPILE)  $SOURCES',
			                CXXCOMSTR = '(COMPILE)  $SOURCES',
			              SHCXXCOMSTR = '(COMPILE)  $SOURCES',
			               SHCCCOMSTR = '(COMPILE)  $SOURCES',
			               ASPPCOMSTR = '(ASSEMBLE) $SOURCES',
			                 ARCOMSTR = '(BUILD)    $TARGET',
			             RANLIBCOMSTR = '(INDEX)    $TARGET',
			               LINKCOMSTR = '(LINK)     $TARGET',
			             SHLINKCOMSTR = '(LINK)     $TARGET'
				    )

	def set_verbosity(self):

		# Make output pretty.
		if not self['VERBOSE']:
			self.Replace(
			                 CCCOMSTR = '(COMPILE)  $SOURCES',
			                CXXCOMSTR = '(COMPILE)  $SOURCES',
			              SHCXXCOMSTR = '(COMPILE)  $SOURCES',
			               SHCCCOMSTR = '(COMPILE)  $SOURCES',
			               ASPPCOMSTR = '(ASSEMBLE) $SOURCES',
			                 ARCOMSTR = '(BUILD)    $TARGET',
			             RANLIBCOMSTR = '(INDEX)    $TARGET',
			               LINKCOMSTR = '(LINK)     $TARGET',
			             SHLINKCOMSTR = '(LINK)     $TARGET'
				    )

	def _GetConfigurationVariables(self):
		"""
			Retrieve and define help variables for configuration variables.
		"""

		cmd_line_args = ARGUMENTS
		cfgVars = Variables(None, cmd_line_args)
		[cfgVars.Add(BoolVariable(name, helptext, default)) for name, helptext, default in self._boolean_variables]
		[cfgVars.Add(EnumVariable(name, helptext, default, valid)) for name, helptext, default, valid in self._enumerable_variables]
		[cfgVars.Add(option) for option in self._numerical_variables]

		return cfgVars


	# BUILD VARIABLES

	#: Build variables which are either on or off.
	_boolean_variables = [
		# These are more to do with build behavior and output
		('VERBOSE', 'If set, displays the actual commands used and their flags instead of the default "neat" output.',
			False)
	]

	#: Build variables which have enumerated values.
	_enumerable_variables = [
		('LINKAGE',
		                 'Library linkage',
		                 'dynamic',
		                 ['dynamic', 'static'])

	]
	
	#: Build variables which have numerical values
	_numerical_variables = [
	]


	#: BUILD DIRECTIVES

	#: Build variables which are either on or off.
	_mnemosyne_boolean_directive_vars = [
		('M_PCM_EMULATE_CRASH',      'PCM emulation layer emulates system crashes.',
			False),
		('M_PCM_EMULATE_LATENCY',    'PCM emulation layer emulates latency.',
			False),
	]

	#: Build variables which have enumerated values.
	_mnemosyne_enumerable_directive_vars = [
	]
	
	#: Build variables which have numerical values
	_mnemosyne_numerical_directive_vars = [
		('M_PCM_CPUFREQ',            'CPU frequency in GHz used by the PCM emulation layer to calculate latencies', 
			2500), 
		('M_PCM_LATENCY_WRITE',      'Latency of a PCM write in nanoseconds. This latency is in addition to the DRAM latency.', 
			150), 
		('M_PCM_BANDWIDTH_MB',       'Bandwidth to PCM in MB/s. This is used to model sequential writes.', 
			1200), 
	]
