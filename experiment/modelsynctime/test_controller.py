import os
import sys

from constellation import run_controller
from constellation.controller import THINKER_NAME as thinker_name

CURRENT_DIR = os.path.dirname(__file__)
sys.path.append(os.path.join(CURRENT_DIR, ".."))

from utils import getenv

if __name__ == "__main__":
    env = getenv(os.path.join(CURRENT_DIR, "expconfig.env"))
    name = env["THINKER_NAME"]
    if name not in thinker_name:
        raise ValueError(f"Invalid thinker name: {name}")
    run_controller(name)
