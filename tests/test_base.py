import sys
import os
import random
import tempfile
import unittest

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from pybbhash.boophf import mphf


class TestBase(unittest.TestCase):
    def setUp(self):
        random.seed(123)
        self.N = 200
        self.M = 10000
        self.keys = random.sample(range(1, self.M), self.N)
        self.m = mphf(len(self.keys), self.keys, gamma=1.5)
        self.tmpdir = tempfile.TemporaryDirectory()
        self.save_path = os.path.join(self.tmpdir.name, "mphf_test.pkl")

    def tearDown(self):
        try:
            self.tmpdir.cleanup()
        except Exception:
            pass

    def _validate_mphf_complete_mapping(self, mph, keys, test_name=""):
        """Helper: validate that mph maps all keys to a complete 0..N-1 permutation."""
        print(f"\n[{test_name}] Validating complete mapping for {len(keys)} keys...")
        
        # collect all mapped indices
        mapped_keys = []
        bad_keys = []
        
        for k in keys:
            idx = mph.lookup(k)
            if not (0 <= idx < len(keys)):
                bad_keys.append((k, idx))
            mapped_keys.append(idx)
        
        # check for bad mappings
        if bad_keys:
            print(f"  ERROR: {len(bad_keys)} keys mapped outside [0, {len(keys)-1}]:")
            for k, idx in bad_keys[:5]:
                print(f"    key {k} -> {idx}")
            self.fail(f"{len(bad_keys)} keys mapped out of range")
        
        # check that all indices form a complete permutation
        sorted_indices = sorted(mapped_keys)
        expected = list(range(len(keys)))
        
        if sorted_indices != expected:
            missing = set(expected) - set(mapped_keys)
            duplicates = [idx for idx in set(mapped_keys) if mapped_keys.count(idx) > 1]
            print(f"  ERROR: Mapping is NOT a valid permutation!")
            if missing:
                print(f"    Missing indices: {list(missing)[:10]}")
            if duplicates:
                print(f"    Duplicate indices: {duplicates[:10]}")
            self.fail("Mapping does not form a complete 0..N-1 permutation")
        
        print(f"  ✓ All {len(keys)} keys map to valid 0..{len(keys)-1} permutation")
        return True

    def test_base_complete_mapping(self):
        """Test that base mphf maps all keys to complete 0..N-1 permutation."""
        print(f"\nBuilt MPH for {self.m.nbKeys()} keys (N={self.N}, M={self.M})")
        self._validate_mphf_complete_mapping(self.m, self.keys, "BASE")

    def test_lookup_nonnegative(self):
        """Basic check: all lookups return non-negative integers."""
        print(f"\nTesting lookup non-negativity for {len(self.keys)} keys...")
        for k in self.keys:
            idx = self.m.lookup(k)
            self.assertIsInstance(idx, int)
            self.assertGreaterEqual(idx, 0, f"Lookup for {k} returned negative {idx}")
        print(f"  ✓ All lookups non-negative")

    def test_save_load(self):
        """Test save/load: loaded mph should pass all base tests."""
        print(f"\nSaving mphf to {self.save_path}...")
        self.m.save(self.save_path)
        self.assertTrue(os.path.exists(self.save_path), "Save file not created")
        
        print(f"Loading mphf from {self.save_path}...")
        loaded = mphf.load(self.save_path)
        
        # verify metadata
        print(f"  Loaded nbKeys: {loaded.nbKeys()} (expected {self.m.nbKeys()})")
        self.assertEqual(self.m.nbKeys(), loaded.nbKeys(), "nbKeys mismatch after load")
        
        # verify all lookups match original
        print(f"  Comparing lookups for all {len(self.keys)} keys...")
        mismatches = []
        for k in self.keys:
            orig_idx = self.m.lookup(k)
            loaded_idx = loaded.lookup(k)
            if orig_idx != loaded_idx:
                mismatches.append((k, orig_idx, loaded_idx))
        
        if mismatches:
            print(f"  ERROR: {len(mismatches)} lookup mismatches:")
            for k, orig, loaded_val in mismatches[:5]:
                print(f"    key {k}: original={orig}, loaded={loaded_val}")
            self.fail(f"{len(mismatches)} lookups differ after load")
        
        print(f"  ✓ All lookups match original")
        
        # validate loaded mph passes complete mapping test
        self._validate_mphf_complete_mapping(loaded, self.keys, "LOADED")

    def test_save_stats(self):
        """Test metadata consistency after save/load."""
        print(f"\nTesting metadata consistency...")
        self.m.save(self.save_path)
        loaded = mphf.load(self.save_path)
        
        print(f"  nbKeys: {loaded.nbKeys()} == {self.m.nbKeys()}")
        self.assertEqual(self.m.nbKeys(), loaded.nbKeys())
        
        print(f"  _lastbitsetrank: {loaded._lastbitsetrank} == {self.m._lastbitsetrank}")
        self.assertEqual(self.m._lastbitsetrank, loaded._lastbitsetrank)
        
        print(f"  _nb_levels: {loaded._nb_levels} == {self.m._nb_levels}")
        self.assertEqual(self.m._nb_levels, loaded._nb_levels)
        
        print(f"  _gamma: {loaded._gamma} == {self.m._gamma}")
        self.assertAlmostEqual(self.m._gamma, loaded._gamma, places=5)
        
        print(f"  ✓ All metadata consistent")


if __name__ == '__main__':
    unittest.main()
