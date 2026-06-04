import os
import string
import SCons.Environment
from SCons.Script import ARGUMENTS, Dir
from SCons.Variables import Variables, EnumVariable, BoolVariable
import helper
import mnemosyne

class Environment(mnemosyne.Environment):
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
		mnemosyne.Environment.__init__(self, mainEnv, configuration_name)
		
		# Apply the configuration variables specific to MTM.
		directives = helper.Directives(configuration_name, 'mcore', ARGUMENTS, self._boolean_directive_vars, self._enumerable_directive_vars, self._numerical_directive_vars)


		# Bring in appropriate environment variables and preprocessor definitions.
		# Note: the inclusion of the OS environment reportedly threatens to make
		# this build more brittle, but otherwise I have to write an SCons builder
		# for libatomic_ops as well. Instead, I let the system paths make it
		# visible and assume that the user has installed libatomic_ops themselves.
		if ('INCLUDE' in os.environ):
			osinclude = string.split(os.environ['INCLUDE'], ':')
		else:
			osinclude = []
		self.Append(
			CPPDEFINES = directives.getPreprocessorDefinitions(),
			CPPPATH = ['include'] + osinclude,
			ENV = os.environ)


	#: BUILD DIRECTIVES

	#: Build directives which are either on or off.
	_boolean_directive_vars = [
		('MCORE_KERNEL_PMAP',           'User-mode Mnemosyne runs on a kernel supporting pmap.',
			True),
	]
	
	#: Build directives which have enumerated values.
	_enumerable_directive_vars = [
	]
	
	#: Build directives which have numerical values
	_numerical_directive_vars = [
	]
