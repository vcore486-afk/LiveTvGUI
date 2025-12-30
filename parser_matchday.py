import requests
from bs4 import BeautifulSoup
import os
import argparse

# =========================
# Чтение базового домена
# =========================
def read_base_domain():
    config_path = os.path.join(os.path.expanduser("~"), ".livetv", "config.txt")
    try:
        with open(config_path, "r", encoding="utf-8") as f:
            base_domain = f.read().strip()
            if not base_domain:
                raise ValueError("Base domain is empty in the config file.")
            return base_domain
    except FileNotFoundError:
        print(f"❌ Ошибка: файл {config_path} не найден.")
        raise
    except Exception as e:
        print(f"❌ Ошибка при чтении конфигурационного файла: {e}")
        raise


# =========================
# Чтение прокси
# =========================
def read_proxy():
    proxy_path = os.path.join(os.path.expanduser("~"), ".livetv", "proxy.txt")

    try:
        config = {}
        with open(proxy_path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line or "=" not in line:
                    continue
                key, value = line.split("=", 1)
                config[key.strip()] = value.strip()

        host = config.get("host")
        port = config.get("port")
        ptype = config.get("type", "http")
        user = config.get("user")
        password = config.get("password")

        if not host or not port:
            raise ValueError("host или port не указаны в proxy.txt")

        if user and password:
            proxy_url = f"{ptype}://{user}:{password}@{host}:{port}"
        else:
            proxy_url = f"{ptype}://{host}:{port}"

        return {
            "http": proxy_url,
            "https": proxy_url,
        }

    except FileNotFoundError:
        print(f"❌ Ошибка: файл {proxy_path} не найден.")
        raise
    except Exception as e:
        print(f"❌ Ошибка при чтении proxy.txt: {e}")
        raise


# =========================
# Загрузка настроек
# =========================
BASE_DOMAIN = read_base_domain()
PROXIES = read_proxy()


# =========================
# Парсинг страницы
# =========================
def parse_page(page_number):
    URL = f"{BASE_DOMAIN}/allupcomingsports/{page_number}/"

    homePath = os.path.expanduser("~")
    output_dir = os.path.join(homePath, ".livetv")
    os.makedirs(output_dir, exist_ok=True)

    OUTPUT_FILE = os.path.join(output_dir, "matchday_events.txt")

    # 1. Загружаем страницу
    try:
        response = requests.get(
            URL,
            headers={"User-Agent": "Mozilla/5.0"},
            proxies=PROXIES,
            timeout=15
        )
        response.raise_for_status()
        html_content = response.text
    except requests.RequestException as e:
        print(f"❌ Ошибка загрузки страницы: {e}")
        return

    # 2. Парсим HTML
    soup = BeautifulSoup(html_content, "lxml")

    # 3. Находим заголовок
    header = soup.find("b", text="Главные матчи дня")
    if not header:
        print("❌ Заголовок не найден")
        return

    container = header.find_parent("table")
    matches_found = 0

    # 4. Запись в файл
    with open(OUTPUT_FILE, "w", encoding="utf-8") as out:
        out.write(f"🔥 Главные матчи дня (первые 10) - Страница {page_number}:\n\n")

        for a in container.find_all_next("a", class_="live"):
            match = a.get_text(strip=True)
            raw_href = a["href"].lstrip("/")

            if raw_href.startswith("allupcomingsports/"):
                raw_href = raw_href[len("allupcomingsports/"):]

            link = f"{BASE_DOMAIN}/{raw_href}"

            evdesc = a.find_next("span", class_="evdesc")
            if not evdesc:
                continue

            lines = list(evdesc.stripped_strings)
            date_time = lines[0]
            league = lines[-1].strip("()")

            out.write(f"{league}\t{match}\n")
            out.write(f"{date_time}\n")
            out.write(f"({league})\n")
            out.write(f"{link}\n\n")

            matches_found += 1
            if matches_found == 10:
                break

    if matches_found == 0:
        print("❌ Матчи не найдены")
    else:
        print(f"✅ Найдено матчей: {matches_found}")
        print(f"📄 Результаты сохранены в файл: {OUTPUT_FILE}")


# =========================
# Точка входа
# =========================
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Парсинг матчей с сайта")
    parser.add_argument("page_number", type=int, help="Номер страницы для парсинга")
    args = parser.parse_args()

    homePath = os.path.expanduser("~")
    output_dir = os.path.join(homePath, ".livetv")
    OUTPUT_FILE = os.path.join(output_dir, "matchday_events.txt")

    if os.path.exists(OUTPUT_FILE):
        os.remove(OUTPUT_FILE)

    parse_page(args.page_number)
