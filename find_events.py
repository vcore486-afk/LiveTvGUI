#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from pathlib import Path
from urllib.parse import urljoin
import requests
from bs4 import BeautifulSoup

# Функция для чтения домена из конфигурационного файла
def read_base_domain():
    config_path = Path.home() / ".livetv" / "config.txt"
    try:
        with open(config_path, "r", encoding="utf-8") as f:
            base_domain = f.read().strip()
            if not base_domain:
                raise ValueError("Base domain is empty in the config file.")
            return base_domain
    except FileNotFoundError:
        print(f"Error: {config_path} not found.")
        raise
    except Exception as e:
        print(f"Error reading config file: {e}")
        raise

# Чтение BASE_DOMAIN
BASE_DOMAIN = read_base_domain()

# ---------- ПРОКСИ ----------
def read_proxy_config():
    proxy_path = Path.home() / ".livetv" / "proxy.txt"
    if not proxy_path.exists():
        return None

    data = {}
    with proxy_path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or "=" not in line:
                continue
            k, v = line.split("=", 1)
            data[k.strip()] = v.strip()

    if not data.get("host") or not data.get("port"):
        return None

    return data


def build_proxies():
    cfg = read_proxy_config()
    if not cfg:
        return None

    proto = cfg.get("type", "http")
    host = cfg["host"]
    port = cfg["port"]
    user = cfg.get("user")
    password = cfg.get("password")

    if user and password:
        proxy_url = f"{proto}://{user}:{password}@{host}:{port}"
    else:
        proxy_url = f"{proto}://{host}:{port}"

    return {
        "http": proxy_url,
        "https": proxy_url,
    }
# ---------------------------


def find_related_events(url, tournament_name, session=None, timeout=10):
    if session is None:
        session = requests.Session()
        proxies = build_proxies()
        if proxies:
            session.proxies.update(proxies)

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

        time_span = link.find_next("span", class_="evdesc")
        event_time = time_span.get_text(" ", strip=True) if time_span else ""

        title_with_time = f"{title}\t{event_time}" if event_time else title

        if (title_with_time, full_href) not in seen:
            seen.add((title_with_time, full_href))
            events.append((title_with_time, full_href))

    return events


def save_to_file(events, filename="events.txt"):
    path = Path.home() / ".livetv" / filename
    path.parent.mkdir(parents=True, exist_ok=True)

    with path.open("w", encoding="utf-8") as f:
        for title_with_time, href in events:
            f.write(f"{title_with_time}\t{href}\n")

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
