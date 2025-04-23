import argparse
import torch
import torch.nn as nn
import torch.optim as optim
import torchvision
import torchvision.transforms as transforms
import torchvision.models as models
import matplotlib.pyplot as plt
import time
from torch.utils.data import random_split
from constellation import run_controller
from constellation.pytorch import Trainer

run_controller('ContelSimpleThinker')

seed  = 48
torch.manual_seed(seed)
torch.cuda.manual_seed(seed)

trainer = Trainer.Trainer()

# 解析命令行参数
parser = argparse.ArgumentParser(
    description="Train ResNet-18 on CIFAR-10 with specified GPU."
)
parser.add_argument("--gpu", type=int, default=2, help="GPU id to use (default: 0)")
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

trainset = torchvision.datasets.CIFAR10(
    root="./data", train=True, download=True, transform=transform
)
testset = torchvision.datasets.CIFAR10(
    root="./data", train=False, download=True, transform=transform
)

# 使用random_split选择部分数据进行训练
subset = len(trainset) // 2
print(subset)
# train_subset, _ = random_split(trainset, [subset, len(trainset) - subset])
train_subset, _ = random_split(trainset, [1000, len(trainset) - 1000])
trainloader = torch.utils.data.DataLoader(
    train_subset, batch_size=64, shuffle=True, num_workers=2
)

testloader = torch.utils.data.DataLoader(
    testset, batch_size=64, shuffle=False, num_workers=2
)


# 定义ResNet-18模型
class ResNet18(nn.Module):
    def __init__(self, num_classes=10):
        super(ResNet18, self).__init__()
        self.model = models.resnet18(pretrained=False)
        self.model.fc = nn.Linear(self.model.fc.in_features, num_classes)

    def forward(self, x):
        return self.model(x)


# 实例化模型并移动到GPU
model = ResNet18(num_classes=10).to(device)


# keys = []
# params = []
# grads = []

# for i, param in enumerate(model.parameters()):
#     if param is not None:
#         keys.append(i)
#         params.append(param)

# trainer.init(keys, params)

# 打印第一层模型参数
# print(f"First layer model parameters: {model.model.fc.weight[0]}")

# 定义损失函数和优化器
criterion = nn.CrossEntropyLoss()
optimizer = optim.SGD(model.parameters(), lr=0.001)

trainer.init(model_opt=(model, optimizer))

# 记录损失和精度
train_losses = []
test_accuracies = []

# 训练模型
num_epochs = 100
for epoch in range(num_epochs):
    model.train()
    running_loss = 0.0
    for inputs, labels in trainloader:
        inputs, labels = inputs.to(device), labels.to(device)

        outputs = model(inputs) # pylint: disable=not-callable
        loss = criterion(outputs, labels)
        loss.backward()
        # grads = []
        # print("begin to pushpull")
        # if len(grads) == 0:
        #     for i, param in enumerate(params):
        #         if param.grad is not None:
        #             grads.append(param.grad)
        #     assert len(grads) == len(keys)
            
        # with torch.no_grad():
        # start = time.time()
        # trainer.pushpull(keys, grads)
        # end = time.time()
        # print("pushpull time: ", end - start)
        # print("end pushpull")
            # 打印第一层模型参数的梯度
        # print(f"First layer model parameters: {model.model.conv1.weight[0]}")
        # print(grads[1])
        # print("=================")
        # print(list(model.parameters())[keys[1]].grad is grads[1])
            # print(list(model.parameters())[keys[1]].grad)
        # print(list(model.parameters())[keys[1]] is params[1])

        # optimizer.step()
        trainer.update()

        # 打印第一层模型参数
        
        # if epoch == 1:
        #     print(trainer.num_trainers)
        #     print(f"First layer model parameters: {model.model.conv1.weight[1]}")

        running_loss += loss.item()
        trainer.batch_end()
    # print(params[1])
    # if epoch == 1:
    #     for name, param in model.named_parameters():
    #         print(name, param.data[0])

    train_losses.append(running_loss / len(trainloader))
    print(
        f"Epoch [{epoch + 1}/{num_epochs}], Loss: {running_loss / len(trainloader):.4f}"
    )

    # 测试模型
    # model.eval()
    # correct = 0
    # total = 0
    # with torch.no_grad():
    #     for inputs, labels in testloader:
    #         inputs, labels = inputs.to(device), labels.to(device)
    #         outputs = model(inputs) # pylint: disable=not-callable
    #         _, predicted = torch.max(outputs.data, 1)
    #         total += labels.size(0)
    #         correct += (predicted == labels).sum().item()

    # accuracy = 100 * correct / total
    # test_accuracies.append(accuracy)
    # print(f"Accuracy of the model on the test images: {accuracy:.2f}%")
    
# 绘制损失和精度曲线
plt.figure(figsize=(12, 5))

plt.subplot(1, 2, 1)
plt.plot(range(1, num_epochs + 1), train_losses, label="Training Loss")
plt.xlabel("Epoch")
plt.ylabel("Loss")
plt.title("Training Loss over Epochs")
plt.legend()

plt.subplot(1, 2, 2)
plt.plot(range(1, num_epochs + 1), test_accuracies, label="Test Accuracy")
plt.xlabel("Epoch")
plt.ylabel("Accuracy (%)")
plt.title("Test Accuracy over Epochs")
plt.legend()

plt.tight_layout()
plt.show()
plt.savefig("resnet_18_loss_accuracy.png")
