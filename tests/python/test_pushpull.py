# coding: utf-8
"""Test pushpull API performance of Constellation."""

import time

import torch
import numpy as np
from constellation.pytorch import Trainer


KEY_NUM = 200           # 200 keys
PARAMS_NUM = 1000000    # 1M parameters per key
TEST_ITER = 20          # 20 iterations

keys = list(range(KEY_NUM))
data = [torch.randn(PARAMS_NUM, dtype=torch.float32) for _ in range(KEY_NUM)]

trainer = Trainer.Trainer()
trainer.init()
trainer.broadcast(keys, data)

print(f"each test pushpull {PARAMS_NUM * KEY_NUM * 4 / 1e6} M bytes")

time_cost = []

for i in range(TEST_ITER):
    start = time.time()
    trainer.pushpull(keys, data)
    end = time.time()
    # print("pushpull time: ", end - start)
    time_cost.append(end - start)

print("avg time: ", sum(time_cost) / len(time_cost))
time_cost = np.array(time_cost)
# save
# np.savetxt(f"./constellation-pushpull_time-{trainer.rank}.txt", time_cost)

# since exit logic is not implemented, we need to sleep for a while to keep the process alive
time.sleep(5)
