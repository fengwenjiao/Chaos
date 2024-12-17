import argparse
import os
import subprocess
import time
import datetime
import torch
import torch.nn as nn
import torch.optim as optim
import torchvision
import torchvision.transforms as transforms
import torchvision.models as models
# import matplotlib.pyplot as plt
from torch.utils.data import random_split

from constellation.pytorch import Trainer
from constellation import run_controller

seed = 48
torch.manual_seed(seed)
torch.cuda.manual_seed(seed)

trainer = Trainer.Trainer()

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

# 设置设备
device = torch.device(f"cuda:{args.gpu}" if torch.cuda.is_available() else "cpu")

# 数据预处理和加载
transform = transforms.Compose(
    [
        transforms.RandomResizedCrop(224),
        transforms.RandomHorizontalFlip(),
        transforms.ToTensor(),
        transforms.Normalize((0.485, 0.456, 0.406), (0.229, 0.224, 0.225)),
    ]
)

data_path = os.path.join(os.path.dirname(__file__), "../data")

trainset = torchvision.datasets.CIFAR10(
    root=data_path, train=True, download=False, transform=transform
)
testset = torchvision.datasets.CIFAR10(
    root=data_path, train=False, download=False, transform=transform
)

# 使用random_split选择部分数据进行训练
subset = 10
train_subset, _ = random_split(trainset, [subset, len(trainset) - subset])
trainloader = torch.utils.data.DataLoader(
    train_subset, batch_size=1, shuffle=True, num_workers=2
)
test_subset, _ = random_split(testset, [1000, len(testset) - 1000])
testloader = torch.utils.data.DataLoader(
    test_subset, batch_size=1, shuffle=False, num_workers=2
)


# 定义ResNet-18模型
class ResNet18(nn.Module):
    """ResNet-18 model"""

    def __init__(self, num_classes=10):
        super(ResNet18, self).__init__()
        self.model = models.resnet18(pretrained=False)
        self.model.fc = nn.Linear(self.model.fc.in_features, num_classes)

    def forward(self, x):
        return self.model(x)

# 定义ResNet-50模型
class ResNet50(nn.Module):
    """ResNet-50 model"""

    def __init__(self, num_classes=10):
        super(ResNet50, self).__init__()
        self.model = models.resnet50(pretrained=False)
        self.model.fc = nn.Linear(self.model.fc.in_features, num_classes)

    def forward(self, x):
        return self.model(x)


# 实例化模型并移动到GPU
# model = ResNet18(num_classes=10).to(device)
model = ResNet50(num_classes=10).to(device)

# 定义损失函数和优化器
criterion = nn.CrossEntropyLoss()
optimizer = optim.SGD(model.parameters(), lr=0.001)

rank = trainer.rank

# exec hostname cmd and print its output
hostname = subprocess.check_output(['hostname']).decode('utf-8')

log_file = os.path.join(os.path.dirname(__file__), f"resnet18_{hostname}_{rank}.log")
log = open(log_file, "w")

trainer.init(model_opt=(model, optimizer))
from constellation.trainer import ConstelTrainer

migrate = ConstelTrainer._migrate
def _migrate(self, keys, values):
    print("Time(start migrate): ",  datetime.datetime.now(), file=log)
    log.flush()
    migrate(self, keys, values)

ConstelTrainer._migrate = _migrate

# 记录损失和精度
train_losses = []
test_accuracies = []

# print time
print("Time(finish init): ",  datetime.datetime.now(), file=log)
log.flush()

# 训练模型
num_epochs = 100
for epoch in range(num_epochs):
    model.train()
    running_loss = 0.0
    for inputs, labels in trainloader:
        inputs, labels = inputs.to(device), labels.to(device)

        outputs = model(inputs)  # pylint: disable=not-callable
        loss = criterion(outputs, labels)
        loss.backward()
        trainer.update()
        running_loss += loss.item()
        trainer.batch_end()
    train_losses.append(running_loss / len(trainloader))
    print(
        f"Epoch [{epoch + 1}/{num_epochs}], Loss: {running_loss / len(trainloader):.4f}"
    )
    
