import os
import string
import SCons.Environment
from SCons.Script import Dir
from SCons.Variables import Variables, EnumVariable, BoolVariable
import sys

class Directives:
	"""
		Creates a list of C preprocessor directives.
	"""

	def __init__(self, cfg_name, cmd_line_args, bool_options, enum_options, other_options):
		self._cfg_name = cfg_name
		self._cmd_line_args = cmd_line_args
		self._bool_options = bool_options
		self._enum_options = enum_options
		self._other_options = other_options
		self._env = SCons.Environment.Environment()
		self.updateVariables()
		self.updateValues()

	def updateValues(self):
		self._variables.Update(self._env)

	def updateVariables(self):
		"""
			Retrieve and define help options for configuration variables.
		"""
		rootdir = Dir('#').abspath
		cfg_files = [
			rootdir + "/library/configuration/" + self._cfg_name + "/mtm.py",
		]
		
		cfgVars = Variables(cfg_files, self._cmd_line_args)
		[cfgVars.Add(BoolVariable(name, helptext, default)) for name, helptext, default in self._bool_options]
		[cfgVars.Add(EnumVariable(name, helptext, default, valid)) for name, helptext, default, valid in self._enum_options]
		[cfgVars.Add(option) for option in self._other_options]

		self._variables = cfgVars

	def getBooleanDirectives(self, options):
		"""
			Takes the boolean directives of this instance and composes them
			into a list where each entry is 'X' if X is True.
		"""
		directives = []
		for name, helptext, default in options:
			if self._env[name] is True:
				directive = '%s' % name
				directives.append(directive)
		return directives

	def getDirectives(self, options):
		"""
			Takes a list of options and returns composes them
			into a list where each entry is 'NAME=VAL'.
		"""
		directives = []
		for option in options:
			directive = '%s=%s' % (option[0], self._env[option[0]])
			directives.append(directive)
		return directives

	def getPreprocessorDefinitions(self):
		"""
			Takes the input configuration and generates a string of preprocessor definitions
			appropriate to the environment as the configuration demands.
		"""
		bool_directives = self.getBooleanDirectives(self._bool_options)
		enum_directives = self.getDirectives(self._enum_options)
		other_directives = self.getDirectives(self._other_options)
		all_directives = bool_directives + enum_directives + other_directives

		return all_directives
