import sys
import os
import random

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from pybbhash.boophf import mphf

if __name__ == '__main__':
	# Predefined parameters
	N = 1000          # number of keys to generate
	M = 10000        # universe size (keys sampled from 1..M-1)
	TEST_COUNT = N # how many keys to test
	SEED = 41        # optional deterministic seed for reproducibility

	random.seed(SEED)

	# generate N unique keys in range [1, M-1]
	keys = random.sample(range(1, M), N)
	m = mphf(len(keys), keys, gamma=1.5)
	print('Built MPH for', m.nbKeys(), 'keys (N=%d, M=%d)' % (N, M))

	# pick TEST_COUNT keys from the generated keys for lookup testing
	test_keys = random.sample(keys, min(TEST_COUNT, len(keys)))
	print('Testing %d keys...' % len(test_keys))

	mapped_keys = []
	for k in test_keys:
		idx = m.lookup(k)
		print(k, '->', idx)
		mapped_keys.append(idx)

	# Sanity checks
	bad = False
	for k in keys:
		idx = m.lookup(k)
		if not (0 <= idx < len(keys)):
			print('Error: key', k, 'mapped to', idx)
			bad = True

	if not bad:
		if sorted(mapped_keys) == list(range(len(test_keys))):
			print('Sampled mapped keys form a 0..K-1 permutation (K=%d)' % len(test_keys))
		else:
			print('Note: sampled mapped keys are not a full 0..K-1 permutation (expected for subset)')
