# -*- coding: utf-8 -*-
import torch
from ctypes import cast

from ..base import CArrayDataPtr, ConsDataTypeEnum, get_basic_type_byte_size
from ..trainer import ConstelTrainer, create_trainer_handle, convert_to_carray
from ..carray import CArrayBase, TensorMixinBase

PYTORCH_TENSOR_TYPE = {
    torch.float32: ConsDataTypeEnum.FLOAT32
}


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
            raise ValueError("Tensor element size is not equal to the size of the corresponding data type")
        bytes_size = elem_size * self.tensor_.nelement()
        return bytes_size

    def _data_ptr(self) -> CArrayDataPtr:

        return cast(self.tensor_.data_ptr(), CArrayDataPtr)


class CArray(CArrayBase, PyTorchTensorMixin):
    def __init__(self, tensor_=None, **kwargs):
        _tensor = None if tensor_ is None else tensor_.to('cpu').contiguous()
        super().__init__(_tensor, **kwargs)
        if self.tensor_ is not tensor_:
            self.origin_tensor_ = tensor_
        else:
            self.origin_tensor_ = None

    def _update_tensor(self):
        if self.origin_tensor_ is not None:
            self.origin_tensor_.copy_(self.tensor_)

    @staticmethod
    def update_tensor(tensor_list):
        if isinstance(tensor_list, list):
            for item in tensor_list:
                CArray.update_tensor(item)
        elif isinstance(tensor_list, CArray):
            tensor_list._update_tensor()
        else:
            raise ValueError("Unsupported type: {}".format(type(tensor_list)))


class Trainer(ConstelTrainer):
    def __init__(self, handle=None):
        if handle is None:
            handle = create_trainer_handle()
        super().__init__(handle)

    def pushpull(self, keys, values, out=None):
        values_carray = convert_to_carray(values, CArray)
        if out is None:
            out_carray = None
        else:
            out_carray = convert_to_carray(out, CArray)
        super().pushpull(keys, values_carray, out_carray)
        CArray.update_tensor(values_carray)

    def init(self, keys, values):
        values_carray = convert_to_carray(values, CArray)
        super().init(keys, values_carray)
