# -*- coding: utf-8 -*-
import warnings
import ctypes

from ..base import TrainerHandle, _LIB, c_str, check_call

__all__ = ['ConstelTrainerBase', 'create']


class ConstelTrainerBase(object):
    """ An Abstract Class for Constellation Trainers."""

    def init(self, key, value, out):
        raise NotImplementedError

    def pushpull(self, key, value, out):
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
                          f'{ConstelTrainerBase.registry[name].__module__}.{ConstelTrainerBase.registry[name].__name__}')
        ConstelTrainerBase.registry[name] = klass
        return klass


def create(name=''):
    if not isinstance(name, str):
        raise TypeError('name must be a string')
    name = name.lower()

    if name in ConstelTrainerBase.registry:
        return ConstelTrainerBase.registry[name]()
    else:
        handle = TrainerHandle()
        check_call(_LIB.ConstelTrainerHandleCreate(c_str(name), ctypes.byref(handle)))

    from .trainer import ConstelTrainer
    return ConstelTrainer(handle)
