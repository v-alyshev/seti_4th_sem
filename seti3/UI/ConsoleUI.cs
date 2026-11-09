namespace seti3.UI;
using Models;
public class ConsoleUI
{
    private const string Bold = "\u001b[1m";
    private const string Reset = "\u001b[0m";

    private const int MaxDescriptionLength = 500;

    private static readonly Dictionary<string, string> TagNames = new()
    {
        ["bus_stop"] = "остановка",
        ["tram_stop"] = "трамвайная остановка",
        ["station"] = "станция",
        ["square"] = "площадь",
        ["attraction"] = "достопримечательность",
        ["museum"] = "музей",
        ["monument"] = "памятник",
        ["memorial"] = "мемориал",
        ["park"] = "парк",
        ["theatre"] = "театр",
        ["cafe"] = "кафе",
        ["restaurant"] = "ресторан",
        ["hotel"] = "гостиница",
        ["university"] = "университет",
        ["school"] = "школа",
        ["hospital"] = "больница",
        ["church"] = "церковь",
        ["house"] = "дом",
        ["residential"] = "жилая улица",
        ["primary"] = "главная дорога",
        ["secondary"] = "дорога",
        ["tertiary"] = "дорога",
        ["unclassified"] = "местная дорога",
        ["service"] = "проезд",
        ["pedestrian"] = "пешеходная улица",
        ["city"] = "город",
        ["town"] = "город",
        ["village"] = "деревня",
        ["suburb"] = "район",
        ["neighbourhood"] = "микрорайон",
        ["river"] = "река",
        ["island"] = "остров",
        ["peak"] = "вершина",
        ["lake"] = "озеро",
        ["water"] = "водоём",
        ["forest"] = "лес",
        ["wood"] = "лес",
        ["beach"] = "пляж",
        ["mountain_range"] = "горный хребет"
    };

    public string SearchQuery()
    {
        while (true)
        {
            Console.Write("Введите запрос. ");
            var str = ReadLine();
            if (str is not null)
            {
                return str;
            }
            Console.WriteLine("Пустой запрос, попробуйте еще раз");
        }
    }

    public void ShowNothing()
    {
        Console.WriteLine("Ничего не найдено");
    }

    public Location ChooseLocation(IReadOnlyList<Location> locations)
    {
        for (int i = 0; i < locations.Count; i++)
        {
            var location = locations[i];
            var fields = new[]
                {
                    Field("Тип", TranslateTag(location.ObjType)),
                    Field("Страна", location.Country),
                    Field("Город", location.City),
                    Field("Улица", location.Street)
                }
                .Where(field => field is not null);

            Console.WriteLine($"{i + 1}. {location.Name} — {string.Join("; ", fields)}");
        }

        while (true)
        {
            Console.WriteLine("Выберите локацию по ее номеру");
            var str = ReadLine();
            if (str is not null && int.TryParse(str, out var num) && 
                (num > 0 &&  num <= locations.Count))
            {
                return locations[num - 1];
            }
            Console.WriteLine($"Ввод должен быть числом от 1 до {locations.Count}");
        }
    }

    public void ShowReport(Report report)
    {
        Console.WriteLine($"{Bold}{report.ReportedLocation.Name}{Reset}");
        Console.WriteLine();

        if (report.ReportedWeather is not null)
        {
            ShowWeather(report.ReportedWeather);
        }
        else
        {
            Console.WriteLine($"Погода недоступна: {report.WeatherError}");
        }

        Console.WriteLine();

        if (report.PlacesError is not null)
        {
            Console.WriteLine($"Интересные места недоступны: {report.PlacesError}");
        }
        else
        {
            ShowPlaces(report.ReportedPlaces);
        }
    }

    public bool AskChooseAnother()
    {
        Console.Write("Выбрать другую локацию из списка? (y/n): ");
        var answer = ReadLine();
        return answer == "y" || answer =="д";
    }

    public void ShowErr(string message)
    {
        Console.Error.WriteLine($"Ошибка: {message}");
    }

    private void ShowWeather(Weather weather)
    {
        Console.WriteLine($"Температура: {weather.Temperature:F1} °C, ощущается как: {weather.TempFeel:F1}°C\n" +
                          $"Скорость ветра: {weather.WindSpeed:F1} м/c\n" +
                          $"Влажность: {weather.Humidity}%, давление {weather.PressureMmHg:F0} мм.рт.ст\n" +
                          $"{weather.Description}");
    }

    private void ShowPlaces(IReadOnlyList<Place> places)
    {
        if (places.Count == 0)
        {
            Console.WriteLine("Интересные места поблизости отсутствуют");
            return;
        }

        for (int i = 0; i < places.Count; i++)
        {
            var place = places[i];
            Console.WriteLine($"{i+1}. {Bold}{place.Title}{Reset}\n" +
                              $"Расстояние до: {place.Distance:F0} м\n" +
                              $"{DescriptionText(place)}\n");
        }
        
        int failedCount = places.Count(place => place.DescriptionError is not null);
        if (failedCount > 0)
        {
            Console.WriteLine($"Не удалось загрузить описания: {failedCount} из {places.Count}");
        }
    }

    private static string? TranslateTag(string? tag)
    {
        if (tag is null)
        {
            return null;
        }

        return TagNames.TryGetValue(tag, out var name) ? name : tag;
    }

    private static string Truncate(string text)
    {
        if (text.Length <= MaxDescriptionLength)
        {
            return text;
        }

        return text[..MaxDescriptionLength].TrimEnd() + "…";
    }

    private static string DescriptionText(Place place)
    {
        if (place.DescriptionError is not null)
        {
            return $"Описание недоступно: {place.DescriptionError}";
        }

        if (string.IsNullOrWhiteSpace(place.Description))
        {
            return "Описание отсутствует";
        }

        return Truncate(place.Description);
    }

    private string? ReadLine()
    {
        var readedLine = Console.ReadLine() ?? throw new EndOfStreamException("Ввод прерван");
        if (!string.IsNullOrWhiteSpace(readedLine))
        {
            return readedLine.Trim();
        }

        return null;
    }

    private static string? Field(string label, string? value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return null;
        }

        return $"{label}: {value}";
    }
}