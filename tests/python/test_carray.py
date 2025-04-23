import unittest

from constellation.carray import CArrayBase
from constellation.pytorch.Trainer import CArray


class TestCArray(unittest.TestCase):
    def test_carraybasedefault(self):
        carray = CArrayBase()
        self.assertEqual(carray.bytes_size(), 0)
        self.assertEqual(carray.dtype(), 0)
        self.assertIsNone(carray.data_ptr().value)

    def test_carraybase(self):
        carray = CArrayBase(bytes_size=400, dtype=0)
        self.assertEqual(carray.bytes_size(), 400)
        self.assertEqual(carray.dtype(), 0)
        self.assertIsNotNone(carray.data_ptr().value)

    def test_carray(self):
        import torch

        tensor = torch.randn(size=(100, 100), dtype=torch.float32, device="cuda")
        carray = CArray(tensor)
        self.assertEqual(carray.bytes_size(), 40000)
        self.assertEqual(carray.dtype(), 0)
        self.assertIsNotNone(carray.data_ptr().value)
        carray.tensor_.fill_(1)
        # update tensor
        carray._update_tensor()
        self.assertEqual(tensor.sum().item(), 10000)
        # clone tensor
        carray_clone = carray.clone()
        self.assertEqual(carray_clone.bytes_size(), 40000)
        self.assertEqual(carray_clone.dtype(), 0)
        self.assertIsNotNone(carray_clone.data_ptr().value)
        self.assertNotEqual(carray.data_ptr().value, carray_clone.data_ptr().value)
        carray_clone.tensor_.fill_(2)
        # update tensor
        carray_clone._update_tensor()
        self.assertEqual(carray_clone.tensor_.sum().item(), 20000)
        self.assertEqual(tensor.sum().item(), 20000)
        


if __name__ == "__main__":
    unittest.main()
