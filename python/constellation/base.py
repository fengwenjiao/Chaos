# -*- coding: utf-8 -*-
import ctypes
import enum
from array import array

from .find_lib_path import find_lib_path

# type definitions
TrainerHandle = ctypes.c_void_p
ControllerHandle = ctypes.c_void_p
CArrayHandle = ctypes.c_void_p
CArrayDataPtr = ctypes.c_void_p
c_uint = ctypes.c_uint


# Data Types
class ConsDataTypeEnum(enum.Enum):
    FLOAT32 = 0


ConsDataTypeByteSize = {
    ConsDataTypeEnum.FLOAT32: 4
}


def get_basic_type_byte_size(dtype):
    try:
        dtype_ = ConsDataTypeEnum(int(dtype))
    except ValueError:
        raise ValueError("Unsupported data type: " + str(dtype))

    return ConsDataTypeByteSize[dtype_]


# ctypes functions
def c_str(string):
    return ctypes.c_char_p(string.encode('utf-8'))


def c_array(ctype, values):
    res = (ctype * len(values))()
    res[:] = values
    return res


def c_array_buf(ctype, values):
    if isinstance(values, array):
        return (ctype * len(values)).from_buffer(values)
    else:
        return c_array(ctype, values)


# load the library
def _load_lib():
    lib_path = find_lib_path()[0]
    try:
        lib = ctypes.CDLL(lib_path, ctypes.RTLD_LOCAL)
    except OSError as e:
        print("Failed to load library: " + lib_path + " " + str(e))
        raise

    return lib


_LIB = _load_lib()


def check_call(ret):
    if ret != 0:
        raise RuntimeError("ConstelTrainerBase call failed")
