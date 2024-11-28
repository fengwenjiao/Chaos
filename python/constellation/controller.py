# -*- coding: utf-8 -*-
from .base import ControllerHandle, check_call, _LIB, c_str

import ctypes

__all__ = ["ConstelController"]


def create_controller_handle(thinker_name=""):
    handle = ControllerHandle()
    check_call(
        _LIB.ConstelControllerHandleCreate(ctypes.byref(handle), c_str(thinker_name))
    )
    return handle


class ConstelController(object):
    def __init__(self, controller_handle=None, *, thinker_name: str = ""):
        self.thinker_name = thinker_name
        if controller_handle is None:
            self.handle = create_controller_handle(thinker_name)
        else:
            self.handle = controller_handle

    def set_thinker(self, thinker_name: str):
        self.thinker_name = thinker_name
        check_call(_LIB.ConstelControllerSetThinker(self.handle, c_str(thinker_name)))

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
