#!/bin/bash

GPU=$(python3 -c "import random; print(random.choice(range(4)))")

echo "Assigned GPU: $GPU"
sleep 30
exec python3 ./test.py --gpu $GPU