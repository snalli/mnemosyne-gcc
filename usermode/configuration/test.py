import os
import string
import SCons.Environment
from SCons.Script import ARGUMENTS, Dir
from SCons.Variables import Variables, EnumVariable, BoolVariable, PathVariable


class Environment(SCons.Environment.Environment):
	"""
		A specialization of the SCons Environment class which properly configures 
        the CxxTest framework.
	"""
	
	def __init__(self, configuration_name):
		"""
			Applies the definitions in configuration_name to generate a correct
			environment. That environment is returned.
		"""
		SCons.Environment.Environment.__init__(self)
		
		# Apply the configuration variables
		configuration_variables = self._GetConfigurationVariables(configuration_name)
		configuration_variables.Update(self)

		self['CXXTEST'] = os.path.join(self['CXXTEST_PATH'], 'python/cxxtest/cxxtestgen.py')
		self.Tool('cxxtest', toolpath=[os.path.join(self['CXXTEST_PATH'], 'build_tools/SCons')])

	
	def _GetConfigurationVariables(self, configuration_name):
		"""
			Retrieve and define help options for configuration variables of
			TinySTM.
		"""
		rootdir = Dir('#').abspath
		configuration_files = [
			os.path.join(rootdir, "configuration/" + configuration_name + "/test.py"),
		]
		command_line_arguments = ARGUMENTS
		
		configuration = Variables(configuration_files, command_line_arguments)
		configuration.Add(PathVariable('CXXTEST_PATH', 'where the CxxTest framework is installed', '$cxxtest_dir'))
		#[configuration.Add(option) for option in self._enumerable_options]
		#[configuration.Add(option) for option in self._numerical_options]
		
		return configuration
	
	
	def _BooleanDirectives(self):
		"""
			Takes the boolean directives of this instance and composes them
			into a list where each entry is 'X' if X is True.
		"""
		directives = []
		for boolean_name in self._boolean_options:
			if self[boolean_name] is True:
				directive = '%s' % boolean_name
				directives.append(directive)
		return directives

	def _Directives(self, options):
		"""
			Takes a list of options and returns composes them
			into a list where each entry is 'NAME=VAL'.
		"""
		directives = []
		for option in options:
			directive = '-D%s=%s' % (option[0], self[option[0]])
			directives.append(directive)
		return directives
	
	
	def _PreprocessorDefinitions(self):
		"""
			Takes the input configuration and generates a string of preprocessor definitions
			appropriate to the environment as the configuration demands.
		"""
		def directive_for_debug_level(level):
			if level is '1':
				return '-DDEBUG'
			elif level is '2':
				return '-DDEBUG -DDEBUG2'
			else:
				return ''

		bool_directives = self._BooleanDirectives()
		numerical_directives = self._Directives(self._numerical_options)
		conflict_manager_directive = 'CM=%s' % self['CONFLICT_MANAGER']
		all_directives = bool_directives + numerical_directives
		all_directives.append(conflict_manager_directive)

		return all_directives
	
	
	#: Build options which are either on or off.
	_boolean_options = {
	}
	
	#: Build options which have enumerated values.
	_enumerable_options = [
	]
	
	#: Build options which have numerical values
	_numerical_options = [
	]
