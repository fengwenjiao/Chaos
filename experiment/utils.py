# pylint: disable=invalid-name,missing-function-docstring
"""Train ResNet-18 on CIFAR-10 with specified GPU."""

import argparse
import torchvision

# 解析命令行参数
parser = argparse.ArgumentParser(
    description="Train ResNet-18 on CIFAR-10 with specified GPU."
)
parser.add_argument("--gpu", type=int, default=0, help="GPU id to use (default: 0)")
parser.add_argument(
    "--subset",
    type=int,
    default=1000,
    help="Number of samples to use for training (default: 1000)",
)
args = parser.parse_args()

data_path = "/data"

# 下载ciifar10数据集
def download_cifar10():
    torchvision.datasets.CIFAR10(
        root=data_path, train=True, download=True
    )
    torchvision.datasets.CIFAR10(
        root=data_path, train=False, download=True
    )
    
    
if __name__ == "__main__":
    download_cifar10()