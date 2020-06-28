#!/usr/bin/env python3
"""Sends simple commands to the sprite-mover."""
# pylint: disable=missing-docstring

import time
import json

FIFO_PATH = "/tmp/led-fifo"

def main():
    start_position = {"x": 81, "y":16}
    add_sprite("bremen", "sprites/clubs/bremen42.png", position=start_position,
               speed=0.2)
    time.sleep(0.1)
    for _ in range(50):
        # send_dict(ID="bremen", command="add", speed=0.5)
        send_dict(ID="bremen", command="add", direction=5)
        time.sleep(0.1)
    send_dict(ID="bremen", command="removeSprite")
    send_dict(ID="bremen", command="add", direction=5)


def send_dict(**kwargs):
    send_message(kwargs)

def send_message(message):
    message_string = json.dumps(message)
    with open(FIFO_PATH, "w") as fifo:
        fifo.write(f"{message_string}\n")
        fifo.flush()
    print(f"Send message: {message_string}")

def add_sprite(sprite_id, file, **kwargs):
    send_dict(ID=sprite_id, command="addSprite", filename=file, **kwargs)

def accelerate(sprite_id, speed_inc, **kwargs):
    send_dict(ID=sprite_id, command="add", speed=speed_inc, **kwargs)



if __name__ == "__main__":
    main()
