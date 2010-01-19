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
	
	def __init__(self, configuration_name):
		"""
			Applies the definitions in configuration_name to generate a correct
			environment (specificall the set of compilation flags() for the
			TinySTM library. That environment is returned
		"""
		SCons.Environment.Environment.__init__(self)
		
		# Apply the configuration variables specific to TinySTM.
		configuration_variables = self._GetConfigurationVariables(configuration_name)
		configuration_variables.Update(self)
		
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
			CPPDEFINES = self._PreprocessorDefinitions(),
			CPPPATH = ['include'] + osinclude,
			ENV = os.environ,
			LIBS = ['atomic_ops'])
		
		# Make output pretty.
		if not self['VERBOSE']:
			self.Replace(
				    CCCOMSTR = '(COMPILE) $SOURCES',
				    ARCOMSTR = '(BUILD)   $TARGET',
				RANLIBCOMSTR = '(INDEX)   $TARGET',
				  SHCCCOMSTR = '(COMPILE) $SOURCES',
				  LINKCOMSTR = '(LINK)    $TARGET')
	
	
	def _GetConfigurationVariables(self, configuration_name):
		"""
			Retrieve and define help options for configuration variables of
			TinySTM.
		"""
		configuration_files = [
			"../../config/" + configuration_name + "/mtm.py",
		]
		
		command_line_arguments = ARGUMENTS
		
		configuration = Variables(configuration_files, command_line_arguments)
		[configuration.Add(option) for option in self._enumerable_options]
		[configuration.Add(option) for option in self._numerical_options]
		for boolean_name in self._boolean_options:
			name = boolean_name
			definition, default = self._boolean_options[name]
			variable = BoolVariable(name, definition, default)
			configuration.Add(variable)
		
		return configuration
	
	
	def _BooleanDirectives(self):
		"""
			Takes the boolean directives of this instance and composes them
			into a list where each entry is 'X' if X is True.
		"""
		directives = []
		for boolean_name in self._boolean_options:
			if self[boolean_name] is True:
				directive = '{name}'.format(name = boolean_name)
				directives.append(directive)
		return directives

	def _Directives(self, options):
		"""
			Takes a list of options and returns composes them
			into a list where each entry is 'NAME=VAL'.
		"""
		directives = []
		for option in options:
			directive = '-D{name}={val}'.format(name = option[0], val = self[option[0]])
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
		conflict_manager_directive = 'CM={cm}'.format(cm = self['CONFLICT_MANAGER'])
		all_directives = bool_directives + numerical_directives
		all_directives.append(conflict_manager_directive)

		return all_directives
	
	
	#: Build options which are either on or off.
	_boolean_options = {
		'ROLLOVER_CLOCK':           ('Roll over clock when it reaches its maximum value.  Clock rollover can be safely disabled on 64 bits to save a few cycles, but it is necessary on 32 bits if the application executes more than 2^28 (write-through) or 2^31 (write-back) transactions.',
			True),
		'CLOCK_IN_CACHE_LINE':      ('Ensure that the global clock does not share the same cache line than some other variable of the program.  This should be normally enabled.',
			True),
		'NO_DUPLICATES_IN_RW_SETS': ('Prevent duplicate entries in read/write sets when accessing the same address multiple times.  Enabling this option may reduce performance so leave it disabled unless transactions repeatedly read or write the same address.',
			True),
		'WAIT_YIELD':               ('Yield the processor when waiting for a contended lock to be released. This only applies to the CM_WAIT and CM_PRIORITY contention managers.',
			True),
		'EPOCH_GC':                 ('Use an epoch-based memory allocator and garbage collector to ensure that accesses to the dynamic memory allocated by a transaction from another transaction are valid.  There is a slight overhead from enabling this feature.',
			True),
		'CONFLICT_TRACKING':        ('Keep track of conflicts between transactions and notifies the application (using a callback), passing the identity of the two conflicting transaction and the associated threads.  This feature requires EPOCH_GC.',
			True),
		'READ_LOCKED_DATA':         ('Allow transactions to read the previous version of locked memory locations, as in the original LSA algorithm (see [DISC-06]). This is achieved by peeking into the write set of the transaction that owns the lock.  There is a small overhead with non-contended workloads but it may significantly reduce the abort rate, especially with transactions that read much data.  This feature only works with the WRITE_BACK_ETL design and requires EPOCH_GC.',
			True),
		'LOCK_IDX_SWAP':            ('Tweak the hash function that maps addresses to locks so that consecutive addresses do not map to consecutive locks. This can avoid cache line invalidations for application that perform sequential memory accesses. The last byte of the lock index is swapped with the previous byte.',
			True),
		
		# These are more to do with build behavior and output
		'VERBOSE': ('If set, displays the actual commands used and their flags instead of the default "neat" output.',
			False)
	}
	
	#: Build options which have enumerated values.
	_enumerable_options = [
		EnumVariable('CONFLICT_MANAGER',
		                 'Determines the conflict_management policy for the STM.',
		                 'CM_SUICIDE',
		                 ['CM_SUICIDE', 'CM_DELAY', 'CM_BACKOFF', 'CM_PRIORITY'])
	]
	
	#: Build options which have numerical values
	_numerical_options = [
		('RW_SET_SIZE',
		 'Initial size of the read and write sets. These sets will grow dynamically when they become full.',
		 4096 # Default
				 ),
		('LOCK_ARRAY_LOG_SIZE',
		 'Number of bits used for indexes in the lock array. The size of the array will be 2 to the power of LOCK_ARRAY_LOG_SIZE.',
		 20 # Default
				 ),
		('MIN_BACKOFF',
		 'Minimum value of the exponential backoff delay. This parameter is only used with the CM_BACKOFF contention manager.',
		 0x4 # Default
		 ),
		('MAX_BACKOFF',
		 'Maximum value of the exponential backoff delay. This parameter is only used with the CM_BACKOFF contention manager.',
		 0x80000000 # Default
		 ),
		('VR_THRESHOLD_DEFAULT', 
		 'Number of aborts due to failed validation before switching to visible reads. A value of 0 indicates no limit. This parameter is only used with the CM_PRIORITY contention manager. It can also be set using an environment variable of the same name.',
		 3 # Default
		 ),
		('CM_THRESHOLD_DEFAULT',
		 'Number of executions of the transaction with a CM_SUICIDE contention management strategy before switching to CM_PRIORITY. This parameter is only used with the CM_PRIORITY contention manager. It can also be set using an environment variable of the same name.',
		 0 # Default
		 )
		 
	]
