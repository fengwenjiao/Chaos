import numpy
import os
import sys
import re
import matplotlib.pyplot as plt

def get_model_name(dir_path):
    return dir_path.split("-")[0]

def get_trainer_log(dir_path):
    res = {}
    model_name = get_model_name(dir_path)
    dir_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), dir_path
    )
    # list all files in the directory
    for f in os.listdir(dir_path):
        if not f.startswith(model_name):
            continue
        # parses the file name to get the trainer rank : vgg11-{rank}-xxx.log
        rank = int(re.search(rf"{model_name}-(\d+)-\d+.log", f).group(1))
        if rank not in res:
            res[rank] = []
        res[rank].append(os.path.join(dir_path, f))
    return res


def extract_data(log_dict, rank=0, data_type="acc"):
    assert data_type in ["acc", "loss"]
    # Test Loss[0]: 86.7442  -> {0: 86.7442}
    # Accuracy[0]: 0.1000 -> {0: 0.1000}
    if data_type == "acc":
        pattern = re.compile(r"Accuracy\[(\d+)\]: (\d+\.\d+)")
    else:
        pattern = re.compile(r"Test Loss\[(\d+)\]: (\d+\.\d+)")
    res = {}
    for f in log_dict[rank]:
        with open(f, "r") as file:
            lines = file.readlines()

        for line in lines:
            match = pattern.search(line)
            if match:
                batch = int(match.group(1))
                value = float(match.group(2))
                res[batch] = value

    def _check(data):
        keys = sorted(data.keys())
        interval = keys[1] - keys[0]
        for i in range(1, len(keys)):
            if keys[i] - keys[i - 1] != interval:
                return False
        return True

    def _format(data):
        keys = sorted(data.keys())
        values = [data[k] for k in keys]
        return numpy.array(keys), numpy.array(values)
    assert _check(res)
    return _format(res)


if __name__ == "__main__":
    log = get_trainer_log("resnet101-5-5")
    print(log)
    data = extract_data(log, 0, "loss")
    print(data)
