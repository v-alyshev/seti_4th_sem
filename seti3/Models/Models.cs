namespace seti3.Models;

public record Coordinates(double Latitude, double Longitude );

public record Location(Coordinates LocatedCoordinates, string Name,
    string? Street, string? City, string? Country, string? ObjType);

public record Weather(string Description, double Temperature, double TempFeel,
    int Humidity, double WindSpeed, double PressureMmHg);

public record Place(long PageNum, string Title, double Distance,
    string? Description, string? DescriptionError = null);

public record Report(Location ReportedLocation, Weather? ReportedWeather,
    IReadOnlyList<Place> ReportedPlaces, string? WeatherError = null, string? PlacesError = null);