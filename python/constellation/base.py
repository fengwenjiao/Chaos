# -*- coding: utf-8 -*-

import ctypes
from .find_lib_path import find_lib_path


# type definitions
TrainerHandle = ctypes.c_void_p


# ctypes functions
def c_str(string):
    return ctypes.c_char_p(string.encode('utf-8'))


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
