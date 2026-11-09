using seti3.Models;

namespace seti3.Api;

public interface IGeocodingClient
{
    Task<List<Location>> SearchLocationsAsync(string request);
}

public interface IWeatherClient
{
    Task<Weather> GetWeatherInfoAsync(Coordinates coordinates);
}

public interface IPlacesClient
{
    Task<List<Place>> FindPlacesAsync(Coordinates coordinates);
    Task<string?> GetDescAsync(long pageId);
}