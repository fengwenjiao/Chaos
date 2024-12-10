# pylint: disable=invalid-name,missing-function-docstring
"""Train ResNet-18 on CIFAR-10 with specified GPU."""

import argparse
import torchvision
import os

# 解析命令行参数
parser = argparse.ArgumentParser(
    description="Train ResNet-18 on CIFAR-10 with specified GPU."
)
args = parser.parse_args()


data_path = os.path.join(os.path.dirname(__file__), "data")

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