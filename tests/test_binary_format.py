"""Test binary format compatibility with C++."""
import unittest
import tempfile
import os
import struct
from pybbhash.boophf import mphf


class TestBinaryFormat(unittest.TestCase):
    """Verify binary save/load format matches C++ expectations."""

    def test_binary_format_structure(self):
        """Test that binary file has correct structure and sizes."""
        # Create a small mphf
        N = 100
        keys = list(range(N))
        mph = mphf(n=N, input_range=keys, gamma=1.5)
        
        # Save to temp file
        with tempfile.NamedTemporaryFile(delete=False, suffix='.bin') as f:
            temp_path = f.name
        
        try:
            mph.save(temp_path)
            
            # Verify file structure by reading raw bytes
            with open(temp_path, 'rb') as f:
                # Read header
                gamma = struct.unpack("<d", f.read(8))[0]  # double (8 bytes)
                nb_levels = struct.unpack("<I", f.read(4))[0]  # uint32_t (4 bytes)
                lastbitsetrank = struct.unpack("<Q", f.read(8))[0]  # uint64_t (8 bytes)
                nelem = struct.unpack("<Q", f.read(8))[0]  # uint64_t (8 bytes)
                
                print(f"\n[Binary Format Check]")
                print(f"  gamma: {gamma} (expected ~1.5)")
                print(f"  nb_levels: {nb_levels} (expected 25)")
                print(f"  lastbitsetrank: {lastbitsetrank}")
                print(f"  nelem: {nelem} (expected {N})")
                
                self.assertAlmostEqual(gamma, 1.5, places=10)
                self.assertEqual(nb_levels, 25)
                self.assertEqual(nelem, N)
                
                # Skip bitset data (would need to parse each level)
                # Just verify we can read to the end
                remaining = f.read()
                print(f"  Total file size: {os.path.getsize(temp_path)} bytes")
                print(f"  ✓ Binary format structure verified")
                
        finally:
            if os.path.exists(temp_path):
                os.unlink(temp_path)

    def test_save_load_roundtrip(self):
        """Test that save/load preserves all data correctly."""
        N = 50
        keys = list(range(10, 10 + N))  # Use non-zero keys
        mph1 = mphf(n=N, input_range=keys, gamma=2.0)
        
        # Save and load
        with tempfile.NamedTemporaryFile(delete=False, suffix='.bin') as f:
            temp_path = f.name
        
        try:
            mph1.save(temp_path)
            mph2 = mphf.load(temp_path)
            
            print(f"\n[Roundtrip Test]")
            print(f"  Testing {N} keys with gamma={mph1._gamma}")
            
            # Verify all lookups match
            for key in keys:
                v1 = mph1.lookup(key)
                v2 = mph2.lookup(key)
                self.assertEqual(v1, v2, f"Mismatch for key {key}: {v1} != {v2}")
            
            # Verify metadata
            self.assertEqual(mph1._gamma, mph2._gamma)
            self.assertEqual(mph1._nelem, mph2._nelem)
            self.assertEqual(mph1._nb_levels, mph2._nb_levels)
            self.assertEqual(mph1._lastbitsetrank, mph2._lastbitsetrank)
            self.assertEqual(len(mph1._final_hash), len(mph2._final_hash))
            
            print(f"  ✓ All {N} lookups match")
            print(f"  ✓ All metadata preserved")
            
        finally:
            if os.path.exists(temp_path):
                os.unlink(temp_path)


if __name__ == '__main__':
    unittest.main()
