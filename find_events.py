#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from pathlib import Path
from urllib.parse import urljoin
import requests
from bs4 import BeautifulSoup

BASE_DOMAIN = "https://livetv869.me"


def find_related_events(url, tournament_name, session=None, timeout=10):
    session = session or requests.Session()

    try:
        resp = session.get(url, timeout=timeout)
        resp.raise_for_status()
    except Exception:
        return []

    soup = BeautifulSoup(resp.content, "html.parser")
    events = []
    seen = set()

    images = soup.find_all("img", attrs={"alt": tournament_name})
    if not images:
        images = [
            img for img in soup.find_all("img")
            if img.get("alt") and tournament_name in img.get("alt")
        ]

    for img in images:
        ancestor = img
        link = None
        while ancestor is not None:
            link = ancestor.find("a", href=True)
            if link:
                break
            ancestor = ancestor.parent

        if not link:
            link = img.find_next("a", href=True)

        if not link:
            continue

        href = link.get("href", "").strip()
        if not href:
            continue

        full_href = urljoin(BASE_DOMAIN, href)
        title = link.get_text(strip=True) or link.get("title") or full_href

        if (title, full_href) not in seen:
            seen.add((title, full_href))
            events.append((title, full_href))

    return events


def save_to_file(events, filename="events.txt"):
    path = Path.home() / ".livetv" / filename
    path.parent.mkdir(parents=True, exist_ok=True)

    with path.open("w", encoding="utf-8") as f:
        for title, href in events:
            f.write(f"{title}\t{href}\n")

    return str(path)


def main(raw):
    """
    ВАЖНО:
    Эта функция вызывается ИСКЛЮЧИТЕЛЬНО с одним аргументом — строкой:
    'НХЛ|2'
    Qt НЕ передаёт второй аргумент!
    """

    try:
        tournament, page = raw.split("|", 1)
    except ValueError:
        print("FORMAT_ERROR")
        return

    tournament = tournament.strip()
    page = int(page.strip())

    url = f"{BASE_DOMAIN}/allupcomingsports/{page}"

    events = find_related_events(url, tournament)
    save_to_file(events)

    print("OK")
