import time

from python.constellation import trainer
from python.constellation import carray
from python.constellation.pytorch import Trainer

import torch
import os
import time

t1 = torch.tensor([2, 2, 3], dtype=torch.float32, device='cuda:0')

trner = Trainer.Trainer()

trner.init(1, t1)
start = time.time()
trner.pushpull(1, t1)
trner.pushpull(1, t1)
trner.pushpull(1, t1)
trner.pushpull(1, t1)
end = time.time()
print(end - start)
print(t1)
