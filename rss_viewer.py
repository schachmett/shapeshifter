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
    PySprite, PyCanvasObjectList, PyAnimationLoop#, EdgeBehavior
)

URL = "http://newsfeed.zeit.de/index"


def main():
    rss = RSS(URL)
    sprites = PyCanvasObjectList()
    for i in range(len(rss.feed.entries)):
        entry_dict = rss.parse_entry(i)
        img_fname = rss.load_img_url(entry_dict["img_url"])
        sprite = PySprite(img_fname)
        sprite.height = 64
        sprite.position = 0, 0
        sprite.visible = False
        sprites[str(i)] = sprite

    animation = PyAnimationLoop(sprites, frame_time_ms=20)
    animation.start()
    try:
        active_idx = 0
        while True:
            for i, sprite in enumerate(sprites.values()):
                if i != active_idx:
                    sprite.visible = False
                else:
                    sprite.visible = True
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
            out["img_url"] = soup.a.img.get("src")
            if "body" in soup:
                out["description"] = soup.body.text.strip()
            else:
                out["description"] = soup.text.strip()
            return out
        raise NotImplementedError

    def load_img_url(self, url):
        if url in self.files:
            return self.files[url]
        tmp = self.tmp_fname
        self.tmp_fname += 1
        # ext = url.split(".")[-1]
        local_fname = f"{self.cache}{tmp}"#".{ext}"
        with open(local_fname, "wb") as img_file:
            img_file.write(requests.get(url).content)
        self.files[url] = local_fname
        return local_fname


def get_timestring(time_struct):
    dtime = datetime.fromtimestamp(time.mktime(time_struct))
    return dtime.strftime("%a, %X")




if __name__ == "__main__":
    main()
