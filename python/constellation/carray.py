# -*- coding: utf-8 -*-
from .base import (
    _LIB,
    check_call,
    CArrayHandle,
    CArrayDataPtr,
    get_basic_type_byte_size,
    get_basic_type_ctype,
)

import ctypes
import numpy as np


class TensorMixinBase(object):
    def __init__(self, tensor_):
        self.tensor_ = tensor_

    def dtype(self) -> int:
        if hasattr(self, "dtype_"):
            return self.dtype_
        raise NotImplementedError()

    def bytes_size(self) -> int:
        if hasattr(self, "bytes_size_"):
            return self.bytes_size_
        raise NotImplementedError()

    def data_ptr(self) -> CArrayDataPtr:
        if hasattr(self, "data_ptr_"):
            return self.data_ptr_
        if not hasattr(self, "handle"):
            raise NotImplementedError()
        ctypes_data_ptr = ctypes.c_void_p()
        check_call(
            _LIB.ConstellationCArrayDataPtr(
                ctypes.byref(self.handle), ctypes.byref(ctypes_data_ptr)
            )
        )
        return ctypes_data_ptr

    def ctypes_data_ptr(self):
        if not hasattr(self, "dtype_") or not hasattr(self, "data_ptr_"):
            raise NotImplementedError()
        ctypes_ptr = get_basic_type_ctype(self.dtype_)
        return ctypes.cast(self.data_ptr_, ctypes.POINTER(ctypes_ptr))


class CArrayBase(TensorMixinBase):
    def __init__(self, tensor_=None, bytes_size=0, dtype=0, **kwargs):
        self.handle = CArrayHandle()
        if tensor_ is not None:
            super(CArrayBase, self).__init__(tensor_)

            # provied by TensorMixin
            self.dtype_ = self.dtype()
            self.bytes_size_ = self.bytes_size()
            self.data_ptr_ = self.data_ptr()

            bytes_size = ctypes.c_uint64(self.bytes_size_)
            dtype = ctypes.c_int(self.dtype_)
            data_ptr = self.data_ptr()

            check_call(
                _LIB.ConstellationCArrayCreate(
                    ctypes.byref(self.handle), data_ptr, bytes_size, dtype
                )
            )
        else:
            type_bytes_size = get_basic_type_byte_size(dtype)
            if dtype % type_bytes_size != 0:
                raise ValueError(
                    f"bytes_size{bytes_size} is not a multiple of dtype {dtype}"
                )

            self.bytes_size_ = bytes_size
            self.dtype_ = dtype

            size = ctypes.c_uint64(self.bytes_size_)
            dtype = ctypes.c_int(self.dtype_)

            check_call(
                _LIB.ConstellationCArrayCreateDefault(
                    ctypes.byref(self.handle), size, dtype
                )
            )
            self.data_ptr_ = self.data_ptr()

    def clone(self) -> "CArrayBase":
        carray = CArrayBase(bytes_size=self.bytes_size_, dtype=self.dtype_)
        type_bytes_size = get_basic_type_byte_size(self.dtype_)
        ele_size = self.bytes_size_ // type_bytes_size
        buffer_dst = np.ctypeslib.as_array(carray.ctypes_data_ptr(), shape=(ele_size,))
        buffer_src = np.ctypeslib.as_array(self.ctypes_data_ptr(), shape=(ele_size,))
        buffer_dst[:] = buffer_src[:]
        return carray

    def free_carry_handle(self):
        if hasattr(self, "handle") and self.handle is not None:
            check_call(_LIB.ConstellationCArrayFree(self.handle))
            self.handle = None

    def __del__(self):
        self.free_carry_handle()
