#!/usr/bin/env python3
"""Sends simple commands to the sprite-mover."""
# pylint: disable=missing-docstring

import time
import random
import json

FIFO_PATH = "/tmp/led-fifo"

def main():
    message = {
        "command": "addSpeed",
        "ID": "bremen",
        "filename": "sprites/clubs/muenchen42.png",
        "argument": 1
    }
    send_message(message)
#    random.seed()
#    send_json(100)


def send_json(num):
    for i in range(num):
        message = {
            "command": random.choice(["addSpeed"]),
            "argument": -0.05
        }
        message_string = json.dumps(message)
        with open(FIFO_PATH, "w") as fifo:
            fifo.write(f"{message_string}\n")
            fifo.flush()
        time.sleep(0.1)
        print(f"did {i}: {message_string}")

def send_message(message):
    message_string = json.dumps(message)
    with open(FIFO_PATH, "w") as fifo:
        fifo.write(f"{message_string}\n")
        fifo.flush()
    time.sleep(0.1)
    print(f"did: {message_string}")



if __name__ == "__main__":
    main()
