import requests
import os
from pathlib import Path
from bs4 import BeautifulSoup

def find_related_events(url, img_pattern):
    """Ищет все элементы, относящиеся к заданному изображению."""
    response = requests.get(url)
    soup = BeautifulSoup(response.content, 'html.parser')

    events = []  # Список для хранения собранных событий

    # Находим все изображения с нужным альт-текстом
    images = soup.find_all('img', attrs={'alt': 'Юношеская лига УЕФА', 'src': '//cdn.livetv869.me/img/icons/uefay.gif'})

    for image in images:
        # Переходим к ближайшему родителю, содержащему событие
        event_container = image.find_parent(class_=lambda cls: cls is None or cls != '')
        if event_container:
            # Следующая важная информация находится в следующем элементе списка
            next_link = event_container.find_next('a')
            if next_link:
                title = next_link.get_text(strip=True)
                href = f"https://livetv869.me{next_link.get('href', '')}"
                events.append((title, href))

    return events


def save_to_file(events, filename="events.txt"):
    """Сохраняет события в указанный файл."""
    # Получение пути к домашней директории пользователя
    home_dir = Path.home()
    
    # Создание пути к папке .livetv
    livetv_dir = home_dir / ".livetv"
    
    # Создание папки .livetv, если она не существует
    livetv_dir.mkdir(parents=True, exist_ok=True)
    
    # Полный путь к файлу events.txt
    file_path = livetv_dir / filename
    
    try:
        with open(file_path, "w", encoding="utf-8") as file:
            for title, href in events:
                file.write(f"{title}\t{href}\n")
        print(f"События успешно сохранены в файл {file_path}.")
    except IOError as e:
        print(f"Ошибка при сохранении файла: {e}")


def main():
    url = "https://livetv869.me/allupcomingsports/1"  # Постоянный URL
    img_pattern = '<img width=27 height=25 alt="Лига Чемпионов" src="//cdn.livetv869.me/img/icons/cleag.gif">'

    related_events = find_related_events(url, img_pattern)

    if related_events:
        print("Связанные события:\n")
        for title, href in related_events:
            print(f"- Название: {title}\n  Ссылка: {href}\n")
        
        # Сохраняем события в файл
        save_to_file(related_events)
    else:
        print("Нет событий, связанных с указанным изображением.")

if __name__ == "__main__":
    main()