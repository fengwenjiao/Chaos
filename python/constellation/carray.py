# -*- coding: utf-8 -*-
from .base import (_LIB, check_call, CArrayHandle,
                   CArrayDataPtr, get_basic_type_byte_size)

import ctypes


class TensorMixinBase(object):
    def __init__(self, tensor_):
        self.tensor_ = tensor_

    def _dtype(self) -> int:
        raise NotImplementedError()

    def _bytes_size(self) -> int:
        raise NotImplementedError()

    def _data_ptr(self) -> CArrayDataPtr:
        raise NotImplementedError()


class CArrayBase(TensorMixinBase):
    def __init__(self, tensor_=None, **kwargs):
        self.handle = CArrayHandle()
        if tensor_ is not None:
            super(CArrayBase, self).__init__(tensor_)

            self.dtype = self._dtype()
            self.bytes_size = self._bytes_size()

            bytes_size = ctypes.c_int(self.bytes_size)
            dtype = ctypes.c_int(self.dtype)
            data_ptr = self._data_ptr()

            check_call(_LIB.ConstellationCArrayCreate(ctypes.byref(self.handle), data_ptr,
                                                      bytes_size, dtype))
        else:
            size = kwargs.get('bytes_size', 0)
            dtype = kwargs.get('dtype', 0)
            type_bytes_size = get_basic_type_byte_size(dtype)
            if size % type_bytes_size != 0:
                raise ValueError(f"bytes_size{size} is not a multiple of dtype {dtype}")

            self.bytes_size = size
            self.dtype = dtype

            size = ctypes.c_int(self.bytes_size)
            dtype = ctypes.c_int(self.dtype)

            check_call(_LIB.ConstellationCArrayCreateDefault(ctypes.byref(self.handle), size, dtype))

    def __del__(self):
        check_call(_LIB.ConstellationCArrayFree(self.handle))
