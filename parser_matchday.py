import requests
from bs4 import BeautifulSoup
import os
import argparse

# Функция для чтения базового домена из конфигурационного файла
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

# Чтение домена из config.txt
BASE_DOMAIN = read_base_domain()

# Функция для парсинга и сохранения данных
def parse_page(page_number):
    # URL страницы для парсинга с учетом номера страницы
    URL = f"{BASE_DOMAIN}/allupcomingsports/{page_number}/"

    # Папка для сохранения результатов
    homePath = os.path.expanduser("~")
    output_dir = os.path.join(homePath, ".livetv")
    os.makedirs(output_dir, exist_ok=True)

    OUTPUT_FILE = os.path.join(output_dir, "matchday_events.txt")  # Всегда одно имя файла

    # 1. Загружаем страницу
    try:
        response = requests.get(URL, headers={"User-Agent": "Mozilla/5.0"})
        response.raise_for_status()
        html_content = response.text
    except requests.RequestException as e:
        print(f"❌ Ошибка загрузки страницы: {e}")
        return

    # 2. Парсим HTML
    soup = BeautifulSoup(html_content, "lxml")

    # 3. Находим заголовок "Главные матчи дня"
    header = soup.find("b", text="Главные матчи дня")
    if not header:
        print("❌ Заголовок не найден")
        return

    container = header.find_parent("table")
    matches_found = 0

    # 4. Сохраняем результаты в файл
    with open(OUTPUT_FILE, "w", encoding="utf-8") as out:  # Здесь не меняем имя файла
        out.write(f"🔥 Главные матчи дня (первые 10) - Страница {page_number}:\n\n")

        for a in container.find_all_next("a", class_="live"):
            match = a.get_text(strip=True)
            raw_href = a["href"].lstrip("/")  # убираем ведущий слэш

            # Убираем "allupcomingsports/" если есть
            if raw_href.startswith("allupcomingsports/"):
                raw_href = raw_href[len("allupcomingsports/"):]

            # Формируем ссылку вида: https://livetv869.me/eventinfo/...
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

# Главная функция
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Парсинг матчей с сайта")
    parser.add_argument("page_number", type=int, help="Номер страницы для парсинга")
    args = parser.parse_args()

    # Явная очистка файла перед парсингом
    homePath = os.path.expanduser("~")
    output_dir = os.path.join(homePath, ".livetv")
    OUTPUT_FILE = os.path.join(output_dir, "matchday_events.txt")
    if os.path.exists(OUTPUT_FILE):
        os.remove(OUTPUT_FILE)

    # Запуск парсинга для указанной страницы
    parse_page(args.page_number)