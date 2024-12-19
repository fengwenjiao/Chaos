import torch
import struct
import hashlib


def _floats_to_bytes(float_list, decimal_places=7):
    """
    Serialize a list of floating-point numbers to bytes.

    Args:
        float_list (list of float): The list of floating-point numbers.

    Returns:
        bytes: The serialized bytes.
    """
    # use '>d' to ensure big-endian
    s = "".join([f"{v:.{decimal_places}f}" for v in float_list])
    byte_data = struct.pack(f">{len(s)}s", s.encode())
    return byte_data


def _compute_md5(float_list, decimal_places=7):
    """
    Calculate the MD5 hash of a list of floating-point numbers.

    Args:
        float_list (list of float): The list of floating-point numbers.

    Returns:
        str: MD5 hash of the list of floating-point numbers.
    """
    byte_data = _floats_to_bytes(float_list, decimal_places)
    md5_hash = hashlib.md5(byte_data).hexdigest()
    return md5_hash


def model_parameters_summary(model: torch.nn.Module, decimal=7):
    """
    Calculate a summary value for the parameters of a PyTorch model.

    Args:
        model (torch.nn.Module): The PyTorch model.

    Returns:
        float: The summary value of the model parameters.
    """
    summary_value = 0.0
    summary_list = []
    param_count = 0
    for param in model.parameters():
        if param is None:
            continue
        mean_v = param.mean().item()
        summary_list.append(mean_v)
        summary_value += mean_v
        param_count += 1
    # to str, keep 6 decimal places
    hash_str = _compute_md5(summary_list, decimal)
    summary_str = ", ".join([f"{v:.{decimal}f}" for v in summary_list])
    summary_str = f"hash: {hash_str} \n values: {summary_str}"
    return summary_str
