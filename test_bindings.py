#!/usr/bin/env python3
"""This script shall test the bindings."""
# pylint: disable=no-name-in-module

import time
import random

from bindings import (
    PySprite, PySpriteList, PySpriteAnimationLoop, EdgeBehavior
)

def main():
    """Main function."""
    # sprite = PySprite("sprites/clubs/bremen42.png", ID="bremen")
    # sprite.position = 1, 5.2
    # sprite.direction = 370
    # sprite.speed = 3.1
    # sprite.do_step()
    # sprite.print_status()
    random.seed()

    slist = PySpriteList(
        bremen=PySprite("sprites/clubs/bremen42.png"),
        # bremen=PySprite(
        #     "http://www.httpvshttps.com/check.png?15962028684488_6"
        # )
        # freiburg=PySprite("sprites/clubs/freiburg42.png")
    )
    # slist["freiburg"].position = 180, 5
    bremen = slist["bremen"]
    # bremen.height = 10
    bremen.direction = 30
    bremen.speed = 1
    bremen.edge_behavior = EdgeBehavior.LOOP_INDIRECT
    animation = PySpriteAnimationLoop(
        slist,
        frame_time_ms=10,
        pwm_dither_bits=1,
    )
    animation.start()
    try:
        while True:
            bremen.direction += random.randint(-5, 5)
            bremen.speed += random.randint(-100, 100) / 1000
            if bremen.speed > 2:
                bremen.speed -= 0.2
            elif bremen.speed < -2:
                bremen.speed += 0.2
            time.sleep(0.01)
    except KeyboardInterrupt:
        print("User interrupt")
    finally:
        animation.end()


if __name__ == "__main__":
    main()
