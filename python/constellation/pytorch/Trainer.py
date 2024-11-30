# -*- coding: utf-8 -*-
import torch
from ctypes import cast
from typing import List, Tuple, Optional, Union

from ..base import CArrayDataPtr, ConsDataTypeEnum, get_basic_type_byte_size
from ..trainer import ConstelTrainer, create_trainer_handle
from ..carray import CArrayBase, TensorMixinBase

PYTORCH_TENSOR_TYPE = {torch.float32: ConsDataTypeEnum.FLOAT32}

__all__ = ["Trainer", "CArray"]


class PyTorchTensorMixin(TensorMixinBase):
    def __init__(self, tensor_: torch.Tensor):
        super().__init__(tensor_)
        if not self.tensor_.is_contiguous():
            raise ValueError("Tensor must be contiguous")

    def _dtype(self) -> int:
        try:
            type_ = PYTORCH_TENSOR_TYPE[self.tensor_.dtype]
        except KeyError:
            raise KeyError("Unsupported tensor type: {}".format(self.tensor_.dtype))

        return type_.value

    def _bytes_size(self) -> int:
        elem_size = self.tensor_.element_size()
        if elem_size != get_basic_type_byte_size(self._dtype()):
            raise ValueError(
                "Tensor element size is not equal to the size of the corresponding data type"
            )
        bytes_size = elem_size * self.tensor_.nelement()
        return bytes_size

    def _data_ptr(self) -> CArrayDataPtr:

        return cast(self.tensor_.data_ptr(), CArrayDataPtr)


class CArray(CArrayBase, PyTorchTensorMixin):
    def __init__(self, tensor_=None, **kwargs):
        _tensor = None if tensor_ is None else tensor_.to("cpu").contiguous().detach()
        super().__init__(_tensor, **kwargs)
        if self.tensor_ is not tensor_:
            self.origin_tensor_ = tensor_
        else:
            self.origin_tensor_ = None

    def _update_tensor(self, scale=1):
        if self.origin_tensor_ is not None:
            with torch.no_grad():
                self.origin_tensor_.copy_(self.tensor_ / scale)

    def sycn_tensor(self):
        if self.origin_tensor_ is not None:
            with torch.no_grad():
                self.tensor_.copy_(self.origin_tensor_)

    @staticmethod
    def update_tensor(tensor_list, scale=1):
        if isinstance(tensor_list, list):
            for item in tensor_list:
                CArray.update_tensor(item, scale)
        elif isinstance(tensor_list, CArray):
            tensor_list._update_tensor(scale)  # pylint: disable=protected-access
        else:
            raise ValueError("Unsupported type: {}".format(type(tensor_list)))


class Trainer(ConstelTrainer):
    def __init__(self, handle=None):
        if handle is None:
            handle = create_trainer_handle()
        super().__init__(handle)
        self._optimizer = None
        self._model = None
        self._keys = None
        self._params = None
        self._lens = None
        self._grads = None
        self._is_model_opt_set = False
        self._rank = 0
        self._num_trainers = 1

    def allreduce(self, keys, values, out=None):
        self._rank = self.rank
        self._num_trainers = self.num_trainers
        values_carray = self._convert_to_carray(values)
        out = out if out is not None else values
        if out is None:
            out_carray = None
        else:
            out_carray = self._convert_to_carray(out)
        super().allreduce(keys, values_carray, out_carray)
        CArray.update_tensor(out_carray, self._num_trainers)

    def _recv(self, keys, values):
        values_carray = self._convert_to_carray(values)
        super()._recv(keys, values_carray)
        CArray.update_tensor(values_carray)

    def broadcast(self, keys, values):
        values_carray = self._convert_to_carray(values)
        super().broadcast(keys, values_carray)
        CArray.update_tensor(values_carray)

    def _migrate(self, keys, values):
        values_carray = self._convert_to_carray(values)
        super()._migrate(keys, values_carray)

    def batch_end(self, keys=None, values=None, _need_notify=True):
        keys_, vals_ = None, None
        need_notify = True
        if keys is not None and values is not None:
            keys_, vals_ = keys, values
        else:
            assert (
                self._is_model_opt_set
            ), "Model and optimizer have not been set. Please call set_model_opt() first."

            keys_ = self._batch_end_get_migrate_keys(get_from_cache=False)
            if len(keys_) == 0:
                return

            need_notify = False

            vals_ = [
                param for i, param in enumerate(self._model.parameters()) if i in keys_
            ]

        super().batch_end(keys_, vals_, need_notify)

    def _convert_to_carray(self, item, cls_=CArray):
        return super()._convert_to_carray(item, cls_)

    def set_model_opt(self, model, optimizer):
        assert isinstance(model, torch.nn.Module)
        assert isinstance(optimizer, torch.optim.Optimizer)
        self._model = model
        self._optimizer = optimizer
        self._is_model_opt_set = True

    def init(
        self,
        model_sync=True,
        keys: Optional[List] = None,
        params: Optional[List[torch.Tensor]] = None,
        model_opt: Optional[Tuple[torch.nn.Module, torch.optim.Optimizer]] = None,
        **kwargs
    ):  # pylint: disable=arguments-differ

        keys_, lens_, params_ = None, None, None
        if model_opt is None:
            assert keys is not None, "keys is required if model_opt is not provided"
            assert params is not None, "params is required if model_opt is not provided"
            lens_ = [param.nelement() * param.element_size() for param in params]
            keys_, params_ = keys, params
        else:
            self._params = []
            self._keys = []
            self._lens = []
            self.set_model_opt(model_opt[0], model_opt[1])
            for i, param in enumerate(self._model.parameters()):
                if param is not None:
                    self._keys.append(i)
                    self._params.append(param)
                    self._lens.append(param.nelement() * param.element_size())
            keys_, lens_, params_ = self._keys, self._lens, self._params

        super()._init_ready_state(model_sync, keys_, lens_)

        if self._is_scale:
            if self._need_sync:
                self._recv(keys_, params_)
        else:
            self.broadcast(keys_, params_)

        if self._optimizer is not None:
            self._optimizer.zero_grad(set_to_none=False)

    def update(self):
        assert (
            self._is_model_opt_set
        ), "Model and optimizer have not been set. Please call set_model_opt() first."

        self._grads = [param.grad for param in self._params if param.grad is not None]
        self.allreduce(self._keys, self._grads)
        self._optimizer.step()
        self._optimizer.zero_grad(set_to_none=False)
