#!/usr/bin/env python3
"""This script shall test the bindings."""
# pylint: disable=no-name-in-module
# pylint: disable=invalid-name
# pylint: disable=unused-import

import time
import random

from bindings import (
    PySprite, PyText, PyCanvasObjectList, PyAnimationLoop#, EdgeBehavior
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

    # import pdb; pdb.set_trace()
    sprites = PyCanvasObjectList()

    # sprite = PySprite("sprites/dorie.png")
    # sprite = PyText("helloooo")
    # sprites["one"] = sprite
    # print(dir(sprites["one"]))

    nemo = PySprite("sprites/nemo.png")
    nemo.position = 130, 5
    nemo.rotation = 30
    nemo.height = 40
    dorie = PySprite("sprites/dorie_lr.png")
    dorie.speed = 0.5

    sprites["nemo"] = nemo
    sprites["dorie"] = dorie

    # for i in range(1):
    #     sprite = PySprite("sprites/nemo.png")
    #     sprite.position = random.randrange(192), random.randrange(64)
    #     sprite.speed = 1.5
    #     sprite.direction = 180
    #     sprite.visible = True
    #     sprites[f"nemo_{i}"] = sprite
    # for i in range(1):
    #     sprite = PySprite("sprites/dorie_lr.png")
    #     sprite.position = random.randrange(192), random.randrange(64)
    #     sprite.speed = 1.5
    #     sprite.visible = True
    #     sprites[f"dorie_lr_{i}"] = sprite
    # text = PyText("moin")
    # text.position = 40, 10
    # text.visible = True
    # sprites["moin"] = text


    animation = PyAnimationLoop(sprites, frame_time_ms=20)

    animation.start()
    try:
        while True:
            # nemo.rotation += 5
            # print(dorie.position)
            # print(dorie.get_overlap(nemo))
            # for sid, s in sprites.items():
            #     if "dorie" in sid or "nemo" in sid:
            #         s.direction += random.randint(-10, 10)
            time.sleep(0.5)
    except KeyboardInterrupt:
        print("User interrupt")
    finally:
        animation.end()
    return

    # slist = PyCanvasObjectList(
    #     bremen=PySprite("sprites/clubs/bremen42.png"),
    #     # bremen=PySprite(
    #     #     "http://www.httpvshttps.com/check.png?15962028684488_6"
    #     # )
    #     # freiburg=PySprite("sprites/clubs/freiburg42.png")
    # )
    # # slist["freiburg"].position = 180, 5
    # bremen = slist["bremen"]
    # # bremen.height = 10
    # bremen.direction = 30
    # bremen.speed = 1
    # bremen.edge_behavior = EdgeBehavior.LOOP_INDIRECT
    # animation = PyAnimationLoop(
    #     slist,
    #     frame_time_ms=10,
    #     pwm_dither_bits=1,
    # )
    # animation.start()
    # try:
    #     while True:
    #         bremen.direction += random.randint(-5, 5)
    #         bremen.speed += random.randint(-100, 100) / 1000
    #         if bremen.speed > 2:
    #             bremen.speed -= 0.2
    #         elif bremen.speed < -2:
    #             bremen.speed += 0.2
    #         time.sleep(0.01)
    # except KeyboardInterrupt:
    #     print("User interrupt")
    # finally:
    #     animation.end()


if __name__ == "__main__":
    main()
