import argparse
from constellation import run_controller
from constellation.controller import THINKER_NAME as thinker_name


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--name",
        help="thinker name",
        choices=thinker_name,
        default="SinglePointConfThinker",
    )
    args = parser.parse_args()
    name = args.name
    run_controller(name)
