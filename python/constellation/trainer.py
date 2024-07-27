# -*- coding: utf-8 -*-
import ctypes
import warnings
from array import array

from .base import _LIB, check_call, TrainerHandle, c_str, c_array, c_array_buf, c_uint
from .carray import CArrayBase

__all__ = ['ConstelTrainer', 'create_trainer_handle', 'convert_to_carray']


def _c_carray_handles_array(carrays):
    res = (ctypes.c_void_p * len(carrays))()
    res[:] = [carray.handle for carray in carrays]
    return res


def _ctype_key_value_cast(keys, values):
    if isinstance(keys, (list, tuple)):
        assert len(keys) == len(values)
        c_keys = []
        c_values = []
        for key, value in zip(keys, values):
            c_key, c_value = _ctype_key_value_cast(key, value)
            c_keys += c_key
            c_values += c_value

        c_keys_arr = c_array(ctypes.c_int, c_keys)
        c_values_arr = c_array(ctypes.c_void_p, c_values)

        return c_keys_arr, c_values_arr

    assert isinstance(keys, int), "Unsupported key type: " + str(type(keys)) + "Only support int key. "
    if issubclass(values.__class__, CArrayBase):
        c_keys_arr = c_array_buf(ctypes.c_int, array('i', [keys]))
        c_values_arr = _c_carray_handles_array([values])
        return c_keys_arr, c_values_arr
    else:
        raise NotImplementedError("Have not supported multiple device yet")
        # for value in values:
        #     assert (issubclass(value,CArrayBase)), "Unsupported value type: " + str(type(value))


def check_keys_unique(keys):
    if isinstance(keys, (tuple, list)):
        return len(keys) == len(set(keys))
    return True


def convert_to_carray(item, cls_):
    if isinstance(item, list):
        return [convert_to_carray(sub_item, cls_) for sub_item in item]
    else:
        return cls_(item)


class ConstelTrainerBase(object):
    """ An Abstract Class for Constellation Trainers."""

    def init(self, keys, values):
        raise NotImplementedError

    def pushpull(self, keys, values, out):
        raise NotImplementedError

    @property
    def rank(self):
        raise NotImplementedError

    @property
    def num_trainers(self):
        raise NotImplementedError

    registry = {}

    @staticmethod
    def register(klass):
        """Registers a new KVStore."""
        assert (isinstance(klass, type))
        name = klass.__name__.lower()
        if name in ConstelTrainerBase.registry:
            warnings.warn(f'WARNING: New kvstore {klass.__module__}.{klass.__name__} is overriding '
                          'existing kvstore '
                          f'{ConstelTrainerBase.registry[name].__module__}.'
                          f'{ConstelTrainerBase.registry[name].__name__}')
        ConstelTrainerBase.registry[name] = klass
        return klass


class ConstelTrainer(ConstelTrainerBase):

    def __init__(self, handle):
        self.handle = handle

    def __del__(self):
        check_call(_LIB.ConstelTrainerHandleFree(self.handle))

    def pushpull(self, keys, values, out=None):
        assert check_keys_unique(keys), "Have not supported multiple device yet. Keys must be unique."

        ckeys, cvalues = _ctype_key_value_cast(keys, values)
        if out is not None:
            ckeys_out, c_values_out = _ctype_key_value_cast(keys, out)
        else:
            ckeys_out, c_values_out = ckeys, cvalues

        check_call(_LIB.ConstellationTrainerPushPull(self.handle, c_uint(len(ckeys)),
                                                     ckeys, c_uint(len(ckeys_out)), ckeys_out,
                                                     cvalues, c_values_out))

    def init(self, keys, values):
        assert check_keys_unique(keys), "Have not supported multiple device yet. Keys must be unique."

        ckeys, cvalues = _ctype_key_value_cast(keys, values)
        check_call(_LIB.ConstellationTrainerInit(self.handle, c_uint(len(ckeys)), ckeys, cvalues))

    @property
    def rank(self):
        rank = ctypes.c_int()
        check_call(_LIB.ConstellationTrainerRank(self.handle, ctypes.byref(rank)))
        return rank.value

    @property
    def num_trainers(self):
        num_trainers = ctypes.c_int()
        check_call(_LIB.ConstellationTrainerNumTrainers(self.handle, ctypes.byref(num_trainers)))
        return num_trainers.value


def create_trainer_handle(name=''):
    handle = TrainerHandle()
    check_call(_LIB.ConstelTrainerHandleCreate(c_str(name), ctypes.byref(handle)))
    return handle


def create(name=''):
    if not isinstance(name, str):
        raise TypeError('name must be a string')
    name = name.lower()

    if name in ConstelTrainerBase.registry:
        return ConstelTrainerBase.registry[name]()
    else:
        handle = create_trainer_handle()

    return ConstelTrainer(handle)
