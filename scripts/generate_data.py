import random

with open("input2.txt", "w") as f:
    for i in range(50000):
        f.write(f"{random.uniform(0, 1000)} {random.uniform(0, 1000)} {random.uniform(0, 1000)}\n")
