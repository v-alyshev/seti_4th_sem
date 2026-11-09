# Lab_3 — Места

Ищем локации по названию, а для выбранной показывает погоду, интересные места поблизости и их описания из Википедии. Запросы к API выполняются асинхронно.

Используемые API: GraphHopper Geocoding, OpenWeatherMap, MediaWiki.

## Требования

- .NET 10 SDK
- API-ключи:
  - GraphHopper — https://www.graphhopper.com/
  - OpenWeatherMap — https://openweathermap.org/api

## Настройка

Заполнить `Config.env` в Config и вписать ключи:

```
GRAPHHOPPER_API_KEY=ключ
OPENWEATHER_API_KEY=ключ
LANGUAGE=ru
REQUEST_TIMEOUT_SECONDS=15
```

## Запуск

```
dotnet run
```

Команду нужно запускать из папки с `seti3.csproj`. Чтобы выйти, нажмите Ctrl+D (в Windows — Ctrl+Z).
