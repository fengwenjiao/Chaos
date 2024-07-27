# -*- coding: utf-8 -*-
from .base import ControllerHandle, check_call, _LIB, c_str

import ctypes

__all__ = ['ConstelController']


def create_controller_handle(name=''):
    handle = ControllerHandle()
    check_call(_LIB.ConstelControllerHandleCreate(c_str(name), ctypes.byref(handle)))
    return handle


class ConstelController(object):
    def __init__(self, controller_handle=None):
        if controller_handle is None:
            self.handle = create_controller_handle()
        else:
            self.handle = controller_handle

    def __del__(self):
        check_call(_LIB.ConstelControllerHandleFree(self.handle))

    def run(self):
        check_call(_LIB.ConstellationControllerRun(self.handle))


def _run_controller():
    is_trainer = ctypes.c_int()
    check_call(_LIB.ConstellationIsTrainer(ctypes.byref(is_trainer)))
    if not is_trainer:
        controller = ConstelController()
        controller.run()


_run_controller()
