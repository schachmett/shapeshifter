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
    PySprite, PySpriteList, PySpriteAnimationLoop#, EdgeBehavior
)

URL = "http://newsfeed.zeit.de/index"


def main():
    rss = RSS(URL)
    entry_dict = rss.parse_entry(0)
    img_fname = rss.load_img_url(entry_dict["img_url"])

    first = PySprite(img_fname)
    first.position = 0, 0
    sprites = PySpriteList(first=first)
    first.height = 64

    animation = PySpriteAnimationLoop(sprites, frame_time_ms=20)
    animation.start()
    try:
        while True:
            time.sleep(0.1)
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
