import requests
from bs4 import BeautifulSoup
import os

# URL страницы для парсинга
URL = "https://livetv869.me/allupcomingsports/1/"

# Папка для сохранения результатов
homePath = os.path.expanduser("~")
output_dir = os.path.join(homePath, ".livetv")
os.makedirs(output_dir, exist_ok=True)

OUTPUT_FILE = os.path.join(output_dir, "matchday_events.txt")

# 1. Загружаем страницу
try:
    response = requests.get(URL, headers={"User-Agent": "Mozilla/5.0"})
    response.raise_for_status()
    html_content = response.text
except requests.RequestException as e:
    print(f"❌ Ошибка загрузки страницы: {e}")
    exit()

# 2. Парсим HTML
soup = BeautifulSoup(html_content, "lxml")

# 3. Находим заголовок "Главные матчи дня"
header = soup.find("b", text="Главные матчи дня")
if not header:
    print("❌ Заголовок не найден")
    exit()

container = header.find_parent("table")
matches_found = 0

# 4. Сохраняем результаты в файл
with open(OUTPUT_FILE, "w", encoding="utf-8") as out:
    out.write("🔥 Главные матчи дня (первые 10):\n\n")

    for a in container.find_all_next("a", class_="live"):
        match = a.get_text(strip=True)
        raw_href = a["href"].lstrip("/")  # убираем ведущий слэш

        # Убираем "allupcomingsports/" если есть
        if raw_href.startswith("allupcomingsports/"):
            raw_href = raw_href[len("allupcomingsports/"):]

        # Формируем ссылку вида: https://livetv869.me/eventinfo/...
        link = f"https://livetv869.me/{raw_href}"

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
