# -*- coding: utf-8 -*-
import ctypes
import warnings
from array import array
import weakref
from typing import Optional, List

from .base import _LIB, check_call, TrainerHandle, c_str, c_array, c_array_buf, c_uint
from .carray import CArrayBase

__all__ = ["ConstelTrainer", "create_trainer_handle"]


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

    assert isinstance(keys, int), (
        "Unsupported key type: " + str(type(keys)) + "Only support int key. "
    )
    if issubclass(values.__class__, CArrayBase):
        c_keys_arr = c_array_buf(ctypes.c_int, array("i", [keys]))
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


class ConstelTrainerBase(object):
    """An Abstract Class for Constellation Trainers."""

    def broadcast(self, keys, values):
        raise NotImplementedError

    def allreduce(self, keys, values, out):
        raise NotImplementedError

    @property
    def rank(self):
        raise NotImplementedError

    @property
    def num_trainers(self):
        raise NotImplementedError

    registry: dict = {}

    @staticmethod
    def register(klass):
        """Registers a new KVStore."""
        assert isinstance(klass, type)
        name = klass.__name__.lower()
        if name in ConstelTrainerBase.registry:
            warnings.warn(
                f"WARNING: New kvstore {klass.__module__}.{klass.__name__} is overriding "
                "existing kvstore "
                f"{ConstelTrainerBase.registry[name].__module__}."
                f"{ConstelTrainerBase.registry[name].__name__}"
            )
        ConstelTrainerBase.registry[name] = klass
        return klass


class ConstelTrainer(ConstelTrainerBase):
    """A Constellation Trainer."""

    def __init__(self, handle):
        self.handle = handle
        self._carray_cache = {}
        self._is_ready = False
        self._is_scale = None
        self._need_sync = None

    def __del__(self):
        check_call(_LIB.ConstelTrainerHandleFree(self.handle))

    def init(
        self, model_sync=True, keys: Optional[List] = None, lens: Optional[List] = None
    ):
        self._need_sync = model_sync
        self._notify_ready(model_sync, keys, lens)
        self._is_scale = self._is_scale()

    @property
    def is_scale(self):
        assert self._is_ready, "Trainer has not been ready."
        return self._is_scale

    def _is_scale(self):
        is_scale = ctypes.c_int()
        check_call(_LIB.ConstelTrainerIsScale(self.handle, ctypes.byref(is_scale)))
        return True if is_scale.value >= 1 else False

    def _notify_ready(self, model_sync=True, keys: list = None, lens: list = None):
        assert not self._is_ready, "Trainer has already been ready."
        model_sync_int = 1 if model_sync else 0
        if not model_sync:
            assert keys is None, "keys must be None if model_sync is False."
            assert lens is None, "lens must be None if model_sync is False."
        if keys is not None:
            assert lens is not None, "lens must be provided if keys is provided."
            assert len(keys) == len(
                lens
            ), "The length of keys and lens must be the same."
            ckeys = c_array(ctypes.c_int, keys)
            clens = c_array(ctypes.c_int, lens)
            check_call(
                _LIB.ConstelTrainerNotifyReadyAndWaitWithKeys(
                    self.handle,
                    ctypes.c_int(model_sync_int),
                    ckeys,
                    clens,
                    c_uint(len(keys)),
                )
            )
        check_call(
            _LIB.ConstelTrainerNotifyReadyAndWait(self.handle),
            ctypes.c_int(model_sync_int),
        )
        self._is_ready = True

    def _batch_end_get_migrate_keys(self):
        size = ctypes.c_int()
        check_call(_LIB.ConstelTrainerBatchEnd(self.handle, ctypes.byref(size)))
        # ConstelTrainerGetKeysToMigrate(int* keys, const int keys_size);
        keys = (ctypes.c_int * size.value)()
        check_call(
            _LIB.ConstelTrainerGetKeysToMigrate(
                ctypes.POINTER(ctypes.c_int)(keys), size
            )
        )
        keys_to_migrate = []
        for i in range(size.value):
            keys_to_migrate.append(keys[i])
        keys_to_migrate.sort()
        return keys_to_migrate

    def batch_end(self, keys, values):
        keys_to_migrate = self._batch_end_get_migrate_keys()

        def check_keys(keys, keys_to_migrate):
            for key in keys:
                if key not in keys_to_migrate:
                    return False
            return True

        assert len(keys) == len(values)
        # check all keys are in keys_to_migrate
        assert check_keys(keys, keys_to_migrate)
        self._migrate(keys, values)

    def allreduce(self, keys, values, out=None):
        assert check_keys_unique(
            keys
        ), "Have not supported multiple device yet. Keys must be unique."

        ckeys, cvalues = _ctype_key_value_cast(keys, values)
        if out is not None:
            ckeys_out, c_values_out = _ctype_key_value_cast(keys, out)
        else:
            ckeys_out, c_values_out = ckeys, cvalues

        check_call(
            _LIB.ConstellationTrainerPushPull(
                self.handle,
                c_uint(len(ckeys)),
                ckeys,
                c_uint(len(ckeys_out)),
                ckeys_out,
                cvalues,
                c_values_out,
            )
        )

    def broadcast(self, keys, values):
        assert check_keys_unique(
            keys
        ), "Have not supported multiple device yet. Keys must be unique."

        ckeys, cvalues = _ctype_key_value_cast(keys, values)
        check_call(
            _LIB.ConstellationTrainerInit(
                self.handle, c_uint(len(ckeys)), ckeys, cvalues
            )
        )

    def _migrate(self, keys, values):
        assert check_keys_unique(
            keys
        ), "Have not supported multiple device yet. Keys must be unique."

        ckeys, cvalues = _ctype_key_value_cast(keys, values)
        check_call(
            _LIB.ConstelTrainerMigrate(self.handle, c_uint(len(ckeys)), ckeys, cvalues)
        )

    def _recv(self, keys, values):
        assert check_keys_unique(
            keys
        ), "Have not supported multiple device yet. Keys must be unique."
        ckeys, cvalues = _ctype_key_value_cast(keys, values)
        check_call(
            _LIB.ConstellationTrainerRecv(
                self.handle, c_uint(len(ckeys)), ckeys, cvalues
            )
        )

    @property
    def rank(self):
        rank = ctypes.c_int()
        check_call(_LIB.ConstellationTrainerRank(self.handle, ctypes.byref(rank)))
        return rank.value

    @property
    def num_trainers(self):
        num_trainers = ctypes.c_int()
        check_call(
            _LIB.ConstellationTrainerNumTrainers(
                self.handle, ctypes.byref(num_trainers)
            )
        )
        return num_trainers.value

    def _convert_to_carray(self, item, cls_):
        if isinstance(item, list):
            return [self._convert_to_carray(sub_item, cls_) for sub_item in item]
        else:
            if id(item) in self._carray_cache:
                carray = self._carray_cache[id(item)]()
                if carray is not None:
                    carray.sycn_tensor()
                    return carray
            carray = cls_(item)
            self._carray_cache[id(item)] = weakref.ref(carray)
            return carray


def create_trainer_handle(name=""):
    handle = TrainerHandle()
    check_call(_LIB.ConstelTrainerHandleCreate(c_str(name), ctypes.byref(handle)))
    return handle


def create(name=""):
    if not isinstance(name, str):
        raise TypeError("name must be a string")
    name = name.lower()

    if name in ConstelTrainerBase.registry:
        return ConstelTrainerBase.registry[name]()
    else:
        handle = create_trainer_handle()

    return ConstelTrainer(handle)
