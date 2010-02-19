import os
import string
import SCons.Environment
from SCons.Script import ARGUMENTS
from SCons.Variables import Variables, EnumVariable, BoolVariable


class Environment(SCons.Environment.Environment):
	"""
		A specialization of the SCons Environment class which does some particular
		munging of the build variables needed in the source files.
	"""
	
	def __init__(self):
		"""
			Applies the definitions in configuration_name to generate a correct
			environment (specificall the set of compilation flags() for the
			TinySTM library. That environment is returned
		"""
		SCons.Environment.Environment.__init__(self)
		
		configuration_variables = Environment._GetConfigurationVariables(self)
		configuration_variables.Update(self)
		self.Append(CPPPATH = '#library/common')
		
		# Make output pretty.
		if not self['VERBOSE']:
			self.Replace(
			                 CCCOMSTR = '(COMPILE)  $SOURCES',
			                CXXCOMSTR = '(COMPILE)  $SOURCES',
			               SHCCCOMSTR = '(COMPILE)  $SOURCES',
			               ASPPCOMSTR = '(ASSEMBLE) $SOURCES',
			                 ARCOMSTR = '(BUILD)    $TARGET',
			             RANLIBCOMSTR = '(INDEX)    $TARGET',
			               LINKCOMSTR = '(LINK)     $TARGET',
			             SHLINKCOMSTR = '(LINK)     $TARGET')
	
	def _GetConfigurationVariables(self):
		"""
			Retrieve and define help options for configuration variables of
			TinySTM.
		"""

		command_line_arguments = ARGUMENTS
		configuration = Variables(None, command_line_arguments)
		for boolean_name in Environment._boolean_options:
			name = boolean_name
			definition, default = Environment._boolean_options[name]
			variable = BoolVariable(name, definition, default)
			configuration.Add(variable)

		return configuration
	
	#: Build options which are either on or off.
	_boolean_options = {
		# These are more to do with build behavior and output
		'VERBOSE': ('If set, displays the actual commands used and their flags instead of the default "neat" output.',
			False)
	}
