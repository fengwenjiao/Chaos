import os
import sys
import subprocess
import datetime
import torch
import torch.nn as nn
import torch.optim as optim
import torchvision
import torchvision.transforms as transforms
import torchvision.models as models

sys.path.append(os.path.join(os.path.dirname(__file__), ".."))
from utils import getenv, model_parameters_summary

env = getenv(os.path.join(os.path.dirname(__file__), "expconfig.env"))
model_name = env["MODEL_NAME"]
resume = int(env["RESUME"]) == 1

NUM_GPUS = 6
MAX_SPILITS = 5
MAX_TRAINER_NUM = int(env["STARTUP_WORKERS"])
START_TRAINER_NUM = int(env["START_TRAINER_NUM"])
PROJECT_NAME = f"{model_name}-{MAX_TRAINER_NUM}-{START_TRAINER_NUM}"

output_dir = os.path.join(os.path.dirname(__file__), f"./{PROJECT_NAME}")
assert os.path.exists(output_dir), f"Output directory {output_dir} does not exist."

# import matplotlib.pyplot as plt
from torch.utils.data import random_split

from constellation.pytorch import Trainer


seed = 48
torch.manual_seed(seed)
torch.cuda.manual_seed(seed)

trainer = Trainer.Trainer()

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
        self.model = models.resnet50()
        self.model.fc = nn.Linear(self.model.fc.in_features, num_classes)

    def forward(self, x):
        return self.model(x)


class ResNet101(nn.Module):
    """ResNet-101 model"""

    def __init__(self, num_classes=10):
        super(ResNet101, self).__init__()
        self.model = models.resnet101(pretrained=False)
        self.model.fc = nn.Linear(self.model.fc.in_features, num_classes)

    def forward(self, x):
        return self.model(x)


class Vgg11(nn.Module):
    """Vgg11 model"""

    def __init__(self, num_classes=10):
        super(Vgg11, self).__init__()
        self.model = models.vgg11(pretrained=False)
        self.model.classifier[6] = nn.Linear(
            self.model.classifier[6].in_features, num_classes
        )

    def forward(self, x):
        return self.model(x)


class Vgg16(nn.Module):
    """Vgg16 model"""

    def __init__(self, num_classes=10):
        super(Vgg16, self).__init__()
        self.model = models.vgg16(pretrained=False)
        self.model.classifier[6] = nn.Linear(
            self.model.classifier[6].in_features, num_classes
        )

    def forward(self, x):
        return self.model(x)


class AlexNet(nn.Module):
    """AlexNet model"""

    def __init__(self, num_classes=10):
        super(AlexNet, self).__init__()
        self.model = models.alexnet(pretrained=False)
        self.model.classifier[6] = nn.Linear(
            self.model.classifier[6].in_features, num_classes
        )

    def forward(self, x):
        return self.model(x)


def savecheckpoint(model, optimizer, epoch, glb_batch):
    checkpoint = {
        "model": model.state_dict(),
        "optimizer": optimizer.state_dict(),
        "epoch": epoch,
        "glb_batch": glb_batch,
    }
    path = os.path.join(output_dir, "checkpoint.pth")
    torch.save(checkpoint, path)
    print(f"Saved. {model_parameters_summary(model)}", file=log)
    log.flush()


def loadcheckpoint(model, optimizer):
    path = os.path.join(output_dir, "checkpoint.pth")
    if os.path.exists(path) and resume:
        checkpoint = torch.load(path)
        model.load_state_dict(checkpoint["model"])
        optimizer.load_state_dict(checkpoint["optimizer"])
        epoch = checkpoint["epoch"]
        glb_batch = checkpoint["glb_batch"]
        return model, optimizer, epoch, glb_batch
    return model, optimizer, 0, 0


id = trainer.myid
gpus = [0, 1, 2, 3, 4]
gpu = gpus[id % len(gpus)]
device = torch.device(f"cuda:{gpu}")

if model_name == "resnet18":
    model = ResNet18(num_classes=10).to(device)
elif model_name == "resnet50":
    model = ResNet50(num_classes=10).to(device)
elif model_name == "resnet101":
    model = ResNet101(num_classes=10).to(device)
elif model_name == "vgg11":
    model = Vgg11(num_classes=10).to(device)
elif model_name == "vgg16":
    model = Vgg16(num_classes=10).to(device)
elif model_name == "alexnet":
    model = AlexNet(num_classes=10).to(device)
else:
    raise ValueError(f"Invalid model name: {model_name}")

# 定义损失函数和优化器
criterion = nn.CrossEntropyLoss()
# optimizer = optim.SGD(model.parameters(), lr=0.001)
optimizer = optim.Adam(model.parameters(), lr=0.001)

epoch_start = 0
glb_batch = 0
if resume:
    model, optimizer, epoch_start, glb_batch = loadcheckpoint(model, optimizer)
    epoch_start += 1

trainer.init(model_opt=(model, optimizer))

rank = trainer.rank

# 使用random_split选择部分数据进行训练
subset = len(trainset) // MAX_SPILITS
spilits = [subset] * MAX_SPILITS + [len(trainset) - subset * MAX_SPILITS]
train_subsets = random_split(trainset, spilits)[0:MAX_SPILITS]

train_subset = train_subsets[rank]
trainloader = torch.utils.data.DataLoader(
    train_subset,
    batch_size=64,
    shuffle=True,
    drop_last=True,
)
subset = len(testset)
test_subset, _ = random_split(testset, [subset, len(testset) - subset])
testloader = torch.utils.data.DataLoader(
    test_subset,
    batch_size=128,
    shuffle=False,
    num_workers=2,
)

# exec hostname cmd and print its output
hostname = subprocess.check_output(["hostname"]).decode("utf-8").strip()

log_file = os.path.join(output_dir, f"{model_name}-{rank}-{trainer.myid}.log")
mode = "a" if resume else "w"
log = open(log_file, mode)


def evaluate(model, dataloader):
    torch.backends.cudnn.deterministic = True
    torch.backends.cudnn.benchmark = False
    model.eval()
    correct = 0
    total = 0
    total_loss = 0.0

    print(f"{model_parameters_summary(model)}", file=log)
    with torch.no_grad():
        for inputs, labels in dataloader:
            inputs, labels = inputs.to(device), labels.to(device)
            outputs = model(inputs)  # pylint: disable=not-callable
            loss = criterion(outputs, labels)
            total_loss += loss.item()
            _, predicted = torch.max(outputs.data, 1)
            total += labels.size(0)
            correct += (predicted == labels).sum().item()
    avg_loss = total_loss / len(dataloader)
    accuracy = correct / total
    return avg_loss, accuracy


def evaluate_train(model, dataloader):
    torch.backends.cudnn.deterministic = True
    torch.backends.cudnn.benchmark = False
    model.eval()
    correct = 0
    total = 0
    total_loss = 0.0

    with torch.no_grad():
        for inputs, labels in dataloader:
            inputs, labels = inputs.to(device), labels.to(device)
            outputs = model(inputs)  # pylint: disable=not-callable
            loss = criterion(outputs, labels)
            total_loss += loss.item()
            _, predicted = torch.max(outputs.data, 1)
            total += labels.size(0)
            correct += (predicted == labels).sum().item()
    avg_loss = total_loss / len(dataloader)
    accuracy = correct / total
    return avg_loss, accuracy


if resume:
    print(
        f"Resume: epoch={epoch_start}, glb_batch={glb_batch} {model_parameters_summary(model)}",
        file=log,
    )

# 训练模型
num_epochs = 400
for epoch in range(epoch_start, num_epochs):
    running_loss = 0.0
    for inputs, labels in trainloader:
        model.train()
        inputs, labels = inputs.to(device), labels.to(device)

        print(f"Time[{glb_batch}]: {datetime.datetime.now()}", file=log)

        outputs = model(inputs)  # pylint: disable=not-callable
        loss = criterion(outputs, labels)
        loss.backward()
        trainer.update()

        print(f"Loss[{glb_batch}]: {loss.item():.4f}", file=log)
        if glb_batch % 39 == 0:
            loss, acc = evaluate(model, testloader)
            print(f"Test Loss[{glb_batch}]: {loss:.4f}", file=log)
            print(f"Accuracy[{glb_batch}]: {acc:.4f}", file=log)

        print(file=log)

        log.flush()

        trainer.batch_end()
        glb_batch += 1

    if trainer.rank == 0:
        savecheckpoint(model, optimizer, epoch, glb_batch)
