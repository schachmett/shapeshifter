#!/usr/bin/env python3
"""This script shall test the bindings."""
# pylint: disable=no-name-in-module

from shapeshifter import PySprite

def main():
    """Main function."""
    sprite = PySprite(b"sprites/clubs/bremen42.png")
    sprite.show_position()
    sprite.set_position(1, 5.2)
    sprite.show_position()

if __name__ == "__main__":
    main()
