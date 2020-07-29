#!/usr/bin/env python3
"""This script shall test the bindings."""
# pylint: disable=no-name-in-module

from shapeshifter_pybindings import (
    PySprite, PySpriteList, PySpriteAnimationLoop,
    PyRGBPanel, PanelOptions
)

def main():
    """Main function."""
    # sprite = PySprite("sprites/clubs/bremen42.png", ID="bremen")
    # sprite.position = 1, 5.2
    # sprite.direction = 370
    # sprite.speed = 3.1
    # sprite.do_step()
    # sprite.print_status()


    slist2 = PySpriteList(
        berlinH=PySprite("sprites/clubs/berlin_H42.png"),
        freiburg=PySprite("sprites/clubs/freiburg42.png")
    )
    # print(slist2["freiburg"].position)
    # opt = PanelOptions()
    # print(opt.hardware_mapping)
    # panel = PyRGBPanel(drop_privileges=0)
    animation = PySpriteAnimationLoop(slist2)


if __name__ == "__main__":
    main()
