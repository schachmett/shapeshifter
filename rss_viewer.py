#!/usr/bin/env python3
"""Display rss feed content."""
# pylint: disable=missing-docstring

import time
from datetime import datetime
import os

import feedparser
from bs4 import BeautifulSoup
import requests

from bindings import (
    PySprite, PyText, PyCanvasObjectList, PyAnimationLoop#, EdgeBehavior
)

URL = "http://newsfeed.zeit.de/index"


def main():
    rss = RSS(URL)
    sprites = PyCanvasObjectList()
    # sprite = PySprite("sprites/dorie.png")
    # sprite.position = 20, 20
    # sprite.speed = 0.5
    # sprite.visible = True
    # sprites["dorie"] = sprite
    # text = PyText("moin")
    # text.position = 10, 10
    # text.visible = True
    # sprites["moin"] = text
    #
    #
    # animation = PyAnimationLoop(sprites, frame_time_ms=20)
    #
    # animation.start()
    # try:
    #     while True:
    #         time.sleep(5)
    # except KeyboardInterrupt:
    #     print("User interrupt")
    # finally:
    #     animation.end()
    # return

    for i in range(len(rss.feed.entries)):
        entry_dict = rss.parse_entry(i)
        img_fname = rss.load_img_url(entry_dict["img_url"])
        if img_fname:
            sprite = PySprite(img_fname)
            sprite.height = 64
            sprite.position = 0, 0
            sprite.visible = False
            sprites[f"img_{str(i)}"] = sprite
        text = PyText("test")
        text.position = 128, 32
        text.visible = False
        sprites[f"desc_{str(i)}"] = text

    animation = PyAnimationLoop(sprites, frame_time_ms=20)
    animation.start()
    try:
        active_idx = 0
        while True:
            for i, sprite in enumerate(sprites.values()):
                desc = sprites[f"desc_{str(i)}"]
                if i != active_idx:
                    sprite.visible = False
                    desc.visible = False
                else:
                    sprite.visible = True
                    desc.visible = True
            print(desc)
            time.sleep(5)
            active_idx += 1
            if active_idx >= len(sprites):
                active_idx = 0
    except KeyboardInterrupt:
        print("User interrupt")
    finally:
        animation.end()


class RSS:
    tmp_fname = 0
    cache = ".cache/"
    def __init__(self, url):
        self.url = url
        self.files = {}
        self.feed = feedparser.parse(url)
        os.makedirs(self.cache, exist_ok=True)

    def parse_entry(self, idx):
        entry = self.feed.entries[idx]
        out = {}
        out["title"] = entry.title
        out["pubtime"] = get_timestring(entry.published_parsed)
        if "zeit" in self.url:
            soup = BeautifulSoup(entry.description, "html.parser")
            if soup.find("a") and soup.find("a").find("img"):
                out["img_url"] = soup.a.img.get("src")
            else:
                out["img_url"] = ""
            if "body" in soup:
                out["description"] = soup.body.text.strip()
            else:
                out["description"] = soup.text.strip()
            return out
        raise NotImplementedError

    def load_img_url(self, url):
        if not url:
            return ""
        if url in self.files:
            return self.files[url]
        try:
            tmp = self.tmp_fname
            self.tmp_fname += 1
            # ext = url.split(".")[-1]
            local_fname = f"{self.cache}{tmp}"#".{ext}"
            with open(local_fname, "wb") as img_file:
                img_file.write(requests.get(url).content)
            self.files[url] = local_fname
            return local_fname
        except AttributeError:
            return ""


def get_timestring(time_struct):
    dtime = datetime.fromtimestamp(time.mktime(time_struct))
    return dtime.strftime("%a, %X")




if __name__ == "__main__":
    main()
