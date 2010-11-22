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
		
		# Generate build directives 
		directives = helper.Directives(configuration_name, 'mtm', ARGUMENTS, self._boolean_directive_vars, self._enumerable_directive_vars, self._numerical_directive_vars)


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
		('ROLLOVER_CLOCK',           'Roll over clock when it reaches its maximum value.  Clock rollover can be safely disabled on 64 bits to save a few cycles, but it is necessary on 32 bits if the application executes more than 2^28 (write-through) or 2^31 (write-back) transactions.',
			True),
		('CLOCK_IN_CACHE_LINE',      'Ensure that the global clock does not share the same cache line than some other variable of the program.  This should be normally enabled.',
			True),
		('NO_DUPLICATES_IN_RW_SETS', 'Prevent duplicate entries in read/write sets when accessing the same address multiple times.  Enabling this option may reduce performance so leave it disabled unless transactions repeatedly read or write the same address.',
			True),
		('WAIT_YIELD',               'Yield the processor when waiting for a contended lock to be released. This only applies to the CM_WAIT and CM_PRIORITY contention managers.',
			True),
		('EPOCH_GC',                 'Use an epoch-based memory allocator and garbage collector to ensure that accesses to the dynamic memory allocated by a transaction from another transaction are valid.  There is a slight overhead from enabling this feature.',
			False),
		('CONFLICT_TRACKING',        'Keep track of conflicts between transactions and notifies the application (using a callback), passing the identity of the two conflicting transaction and the associated threads.  This feature requires EPOCH_GC.',
			False),
		('READ_LOCKED_DATA',         'Allow transactions to read the previous version of locked memory locations, as in the original LSA algorithm (see [DISC-06]). This is achieved by peeking into the write set of the transaction that owns the lock.  There is a small overhead with non-contended workloads but it may significantly reduce the abort rate, especially with transactions that read much data.  This feature only works with the WRITE_BACK_ETL design and requires EPOCH_GC.',
			False),
		('LOCK_IDX_SWAP',            'Tweak the hash function that maps addresses to locks so that consecutive addresses do not map to consecutive locks. This can avoid cache line invalidations for application that perform sequential memory accesses. The last byte of the lock index is swapped with the previous byte.',
			True),
		('ALLOW_ABORTS',       'Allows transaction aborts. When disabled and combined with no-isolation, the TM system does not need to perform version management for volatile data.',
			False),
		('SYNC_TRUNCATION',          'Synchronously flushes the write set out of the HW cache and truncates the persistent log.',
			True),
		('FLUSH_CACHELINE_ONCE',          'When asynchronously truncating the log, the log manager flushes each cacheline of the write set only once by keeping track flushed cachelines.',
			False),

	]
	
	#: Build directives which have enumerated values.
	_enumerable_directive_vars = [
		('CM',
		                 'Determines the conflict_management policy for the STM.',
		                 'CM_SUICIDE',
		                 ['CM_SUICIDE', 'CM_DELAY', 'CM_BACKOFF', 'CM_PRIORITY']),
		('TMLOG_TYPE',
		                 'Determines the type of the persistent log used.',
		                 'TMLOG_TYPE_BASE',
		                 ['TMLOG_TYPE_BASE', 'TMLOG_TYPE_TORNBIT'])
	]
	
	#: Build directives which have numerical values
	_numerical_directive_vars = [
		('RW_SET_SIZE',
		 'Initial size of the read and write sets. These sets will grow dynamically when they become full.',
		 16384 # Default
				 ),
		('LOCK_ARRAY_LOG_SIZE',
		 'Number of bits used for indexes in the lock array. The size of the array will be 2 to the power of LOCK_ARRAY_LOG_SIZE.',
		 20 # Default
				 ),
		('PRIVATE_LOCK_ARRAY_LOG_SIZE',
		 'Number of bits used for indexes in the private pseudo-lock array. The size of the array will be 2 to the power of PRIVATE_LOCK_ARRAY_LOG_SIZE.',
		 8 # Default
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
