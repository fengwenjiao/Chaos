# -*- coding: utf-8 -*-

from .base import ConstelTrainerBase
from ..base import _LIB, check_call


__all__ = ['ConstelTrainer']


class ConstelTrainer(ConstelTrainerBase):

    def __init__(self, handle):
        self.handle = handle

    def __del__(self):
        check_call(_LIB.ConstelTrainerHandleFree(self.handle))

    def pushpull(self, key, value, out):
        pass

    def init(self, key, value, out):
        pass

    @property
    def rank(self):
        pass

    @property
    def num_trainers(self):
        pass
