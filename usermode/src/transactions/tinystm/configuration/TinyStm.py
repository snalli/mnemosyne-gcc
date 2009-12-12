import string
import SCons.Environment
from SCons.Variables import Variables, EnumVariable, BoolVariable

class Environment(SCons.Environment.Environment):
	"""A specialization of the SCons Environment class which does some particular
	   munging of the build variables needed in the source files."""
	
	boolean_options = {
		'ROLLOVER_CLOCK':           'Roll over clock when it reaches its maximum value.  Clock rollover can be safely disabled on 64 bits to save a few cycles, but it is necessary on 32 bits if the application executes more than 2^28 (write-through) or 2^31 (write-back) transactions.',
		'CLOCK_IN_CACHE_LINE':      'Ensure that the global clock does not share the same cache line than some other variable of the program.  This should be normally enabled.',
		'NO_DUPLICATES_IN_RW_SETS': 'Prevent duplicate entries in read/write sets when accessing the same address multiple times.  Enabling this option may reduce performance so leave it disabled unless transactions repeatedly read or write the same address.',
		'WAIT_YIELD':               'Yield the processor when waiting for a contended lock to be released. This only applies to the CM_WAIT and CM_PRIORITY contention managers.',
		'USE_BLOOM_FILTER':         'Use a (degenerate) bloom filter for quickly checking in the write set whether an address has previously been written.  This approach is directly inspired by TL2.  It only applies to the WRITE_BACK_CTL design.',
		'EPOCH_GC':                 'Use an epoch-based memory allocator and garbage collector to ensure that accesses to the dynamic memory allocated by a transaction from another transaction are valid.  There is a slight overhead from enabling this feature.',
		'CONFLICT_TRACKING':        'Keep track of conflicts between transactions and notifies the application (using a callback), passing the identity of the two conflicting transaction and the associated threads.  This feature requires EPOCH_GC.',
		'READ_LOCKED_DATA':         'Allow transactions to read the previous version of locked memory locations, as in the original LSA algorithm (see [DISC-06]). This is achieved by peeking into the write set of the transaction that owns the lock.  There is a small overhead with non-contended workloads but it may significantly reduce the abort rate, especially with transactions that read much data.  This feature only works with the WRITE_BACK_ETL design and requires EPOCH_GC.',
		'LOCK_IDX_SWAP':            'Tweak the hash function that maps addresses to locks so that consecutive addresses do not map to consecutive locks. This can avoid cache line invalidations for application that perform sequential memory accesses. The last byte of the lock index is swapped with the previous byte.'
	}
	
	enumerable_options = [
		EnumVariable('DEBUG',
		                 'Selects level of debugging output.',
		                 'None', # Default
		                 ['0', '1', '2'],
		                 map = {'None': 0, 'Light': 1, 'Heavy': 2}),
		EnumVariable('DESIGN',
		                 'Determines the version management policy for the STM.',
		                 'WRITE_BACK_ETL',
		                 ['WRITE_BACK_ETL', 'WRITE_BACK_CTL', 'WRITE_THROUGH']),
		EnumVariable('CONFLICT_MANAGER',
		                 'Determines the conflict_management policy for the STM.',
		                 'CM_SUICIDE',
		                 ['CM_SUICIDE', 'CM_DELAY', 'CM_BACKOFF', 'CM_PRIORITY'])
	]
	
	def __init__(self, configuration_name):
		"""
			Applies the definitions in configuration_name to generate a correct
			environment (specificall the set of compilation flags() for the
			TinySTM library. That environment is returned
		"""
		SCons.Environment.Environment.__init__(self)
		configuration_variables = self.GetConfigurationVariables(configuration_name)
		configuration_variables.Update(self)
		self.Append(RANLIBFLAGS = self.preprocessorDefinitions())
		self.Append(CFLAGS = self.preprocessorDefinitions())
		self.Append(LIBS=['atomic'])
	
	def GetConfigurationVariables(self, configuration_name):
		"""
			Retrieve and define help options for configuration variables of
			TinySTM.
		"""
		configuration_files = [
			"configuration/" + configuration_name + "/debugging.py",
			"configuration/" + configuration_name + "/other.py",
			"configuration/" + configuration_name + "/policies.py"
		]

		configuration = Variables(configuration_files)
		[configuration.Add(option) for option in self.enumerable_options]
		for boolean_name in self.boolean_options:
			name = boolean_name
			definition = self.boolean_options[name]
			variable = BoolVariable(name, definition, True)
			configuration.Add(variable)
		
		return configuration
	
	def booleanDirectives(self):
		"""
			Takes the boolean directives of this instance and composes them
			into a list where each entry is '-DX' if X is True.
		"""
		directives = []
		for boolean_name in self.boolean_options:
			if self[boolean_name] is True:
				directive = '-D{name}'.format(name = boolean_name)
				directives.append(directive)
		return directives
	
	def preprocessorDefinitions(self):
		"""
			Takes the input configuration and generates a string of preprocessor definitions
			appropriate to the environment as the configuration demands.
		"""
		def directive_for_debug_level(level):
			if level is 1:
				return '-DDEBUG'
			elif level is 2:
				return '-DDEBUG -DDEBUG2'
			else:
				return None
		
		bool_directives = self.booleanDirectives()
		debug_directive = directive_for_debug_level(self['DEBUG'])
		design_directive = 'tnhauehsaeou -DDESIGN={design}'.format(design = self['DESIGN'])
		conflict_manager_directive = '-DCM={cm}'.format(cm = self['CONFLICT_MANAGER'])
		
		all_directives = bool_directives
		all_directives.extend([debug_directive, design_directive, conflict_manager_directive])
		return string.join(all_directives)
