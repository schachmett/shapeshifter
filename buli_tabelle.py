#!/usr/bin/env python3
"""Displays the Bundesliga table."""
# pylint: disable=missing-docstring

import sys
import json
import time


FIFO_PATH = "/tmp/led-fifo"


def main():
    teams = []
    before1 = {"x": -53, "y": 10}
    before2 = {"x": -117, "y": 10}
    before3 = {"x": -181, "y": 10}
    pos1 = {"x": 139, "y": 10}
    pos2 = {"x": 75, "y": 10}
    pos3 = {"x": 11, "y": 10}
    after1 = {"x": 331, "y": 10}
    after2 = {"x": 267, "y": 10}
    after3 = {"x": 203, "y": 10}
    for i, team in enumerate(TABLE):
        fname = LOGOS[team]
        teams.append(team)
        send_dict(ID=team, command="addSprite", filename=fname,
                  position=before1, edge_behavior="disappear")
        time.sleep(0.01)

    while True:
        for i in range(0, 18, 3):
            send_dict(ID=teams[i], command="set", position=before1)
            send_dict(ID=teams[i+1], command="set", position=before2)
            send_dict(ID=teams[i+2], command="set", position=before3)
        time.sleep(2)

        for i in range(0, 18, 3):
            send_dict(ID=teams[i], command="target", position=pos1,
                      duration=2500)
            send_dict(ID=teams[i+1], command="target", position=pos2,
                      duration=2500)
            send_dict(ID=teams[i+2], command="target", position=pos3,
                      duration=2500)
            time.sleep(3)

            send_dict(ID=teams[i], command="target", position=after1,
                      duration=2500)
            send_dict(ID=teams[i+1], command="target", position=after2,
                      duration=2500)
            send_dict(ID=teams[i+2], command="target", position=after3,
                      duration=2500)
        time.sleep(3)

def triple_send(teams, idx, **kwargs):
    for xxx in range(3):
        send_dict(ID=teams[idx+xxx], **kwargs)
        time.sleep(0.01)


def send_dict(**kwargs):
    message_string = json.dumps(kwargs)
    with open(FIFO_PATH, "w") as fifo:
        fifo.write(f"{message_string}\n")
        fifo.flush()
        time.sleep(0.01)
    print(f"Send message: {message_string}")


TABLE = [
    "FC Bayern München",
    "Borussia Dortmund",
    "RB Leipzig",
    "Borussia Mönchengladbach",
    "Bayer 04 Leverkusen",
    "TSG 1899 Hoffenheim",
    "VfL Wolfsburg",
    "SC Freiburg",
    "Eintracht Frankfurt",
    "Hertha BSC",
    "1. FC Union Berlin",
    "FC Schalke 04",
    "1. FSV Mainz 05",
    "1. FC Köln",
    "FC Augsburg",
    "SV Werder Bremen",
    "Fortuna Düsseldorf",
    "SC Paderborn 07"
]


LOGOS = {
    "FC Augsburg": "sprites/clubs/augsburg42.png",
    "Hertha BSC": "sprites/clubs/berlin_H42.png",
    "1. FC Union Berlin": "sprites/clubs/berlin_U42.png",
    "SV Werder Bremen": "sprites/clubs/bremen42.png",
    "Borussia Dortmund": "sprites/clubs/dortmund42.png",
    "Fortuna Düsseldorf": "sprites/clubs/duesseldorf42.png",
    "Eintracht Frankfurt": "sprites/clubs/frankfurt42.png",
    "SC Freiburg": "sprites/clubs/freiburg42.png",
    "FC Schalke 04": "sprites/clubs/gelsenkirchen42.png",
    "TSG 1899 Hoffenheim": "sprites/clubs/hoffenheim42.png",
    "1. FC Köln": "sprites/clubs/koeln42.png",
    "RB Leipzig": "sprites/clubs/leipzig42.png",
    "Bayer 04 Leverkusen": "sprites/clubs/leverkusen42.png",
    "1. FSV Mainz 05": "sprites/clubs/mainz42.png",
    "Borussia Mönchengladbach": "sprites/clubs/moenchengladbach42.png",
    "FC Bayern München": "sprites/clubs/muenchen42.png",
    "SC Paderborn 07": "sprites/clubs/paderborn42.png",
    "VfL Wolfsburg": "sprites/clubs/wolfsburg42.png",
}


if __name__ == "__main__":
    sys.argv.append("--led-rows=64")
    sys.argv.append("--led-cols=64")
    sys.argv.append("--led-chain=3")
    main()
