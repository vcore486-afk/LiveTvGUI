import requests
from pathlib import Path
from bs4 import BeautifulSoup

def find_related_events(url, tournament_name):
    """Ищет все элементы, относящиеся к заданному турниру по названию."""
    response = requests.get(url)
    soup = BeautifulSoup(response.content, 'html.parser')

    events = []

    # Находим все изображения с соответствующим названием турнира
    images = soup.find_all('img', attrs={'alt': tournament_name})

    for image in images:
        # Находим ближайшего родителя, содержащий событие
        event_container = image.find_parent(class_=lambda cls: cls is None or cls != '')
        if event_container:
            # Находим следующую важную ссылку
            next_link = event_container.find_next('a')
            if next_link:
                title = next_link.get_text(strip=True)
                href = f"https://livetv869.me{next_link.get('href', '')}"
                events.append((title, href))

    return events

def save_to_file(events, filename="events.txt"):
    """Сохраняет события в указанный файл."""
    home_dir = Path.home()
    livetv_dir = home_dir / ".livetv"
    livetv_dir.mkdir(parents=True, exist_ok=True)

    file_path = livetv_dir / filename

    try:
        with open(file_path, "w", encoding="utf-8") as file:
            for title, href in events:
                file.write(f"{title}\t{href}\n")
        print(f"События успешно сохранены в файл {file_path}.")
    except IOError as e:
        print(f"Ошибка при сохранении файла: {e}")

def main(tournament_name):
    url = "https://livetv869.me/allupcomingsports/1"
    related_events = find_related_events(url, tournament_name)

    if related_events:
        print("Связанные события:\n")
        for title, href in related_events:
            print(f"- Название: {title}\n  Ссылка: {href}\n")
        
        # Сохраняем события в файл
        save_to_file(related_events)
    else:
        print("Нет событий, связанных с указанным турниром.")

if __name__ == "__main__":
    # Запускаем основную функцию с передачей первого аргумента
    main("Лига Европы")