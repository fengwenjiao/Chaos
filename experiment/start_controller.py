import argparse
from constellation import run_controller

thinker_name = [
    "SimpleEqualConfThinker",
    "SinglePointConfThinker",
    "FAPTEqualConfThinker",
    "FAPTTimeWeightedConfThinker",
    "RoundRobinTimeWeightedThinker",
]
parser = argparse.ArgumentParser()
parser.add_argument("--name", help="thinker name", choices=thinker_name)
args = parser.parse_args()

name = args.name

if __name__ == "__main__":
    run_controller(name)
