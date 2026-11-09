using seti3.Api;
using seti3.Models;

namespace seti3.Services;

public class PlaceService(IGeocodingClient ghClient, 
    IWeatherClient owClient, IPlacesClient wClient )
{
    public async Task<List<Location>> GetLocationsAsync(string query)
    {
        return await ghClient.SearchLocationsAsync(query);
    }

    public async Task<Report> GetLocationReportAsync(Location location)
    {
        var weatherTask = TryGetWeatherAsync(location.LocatedCoordinates);
        var placesTask = TryFindPlacesAsync(location.LocatedCoordinates);

        await Task.WhenAll(weatherTask, placesTask);

        var (weather, weatherError) = await weatherTask;
        var (places, placesError) = await placesTask;

        return new Report(location, weather, places, weatherError, placesError);
    }

    private async Task<(Weather? Weather, string? Error)> TryGetWeatherAsync(Coordinates coordinates)
    {
        try
        {
            return (await owClient.GetWeatherInfoAsync(coordinates), null);
        }
        catch (Exception e) when (ErrorMessages.IsExpected(e))
        {
            return (null, ErrorMessages.Describe(e));
        }
    }

    private async Task<(Place[] Places, string? Error)> TryFindPlacesAsync(Coordinates coordinates)
    {
        try
        {
            return (await FindPlacesAndDescAsync(coordinates), null);
        }
        catch (Exception e) when (ErrorMessages.IsExpected(e))
        {
            return (Array.Empty<Place>(), ErrorMessages.Describe(e));
        }
    }

    private async Task<Place[]> FindPlacesAndDescAsync(Coordinates coordinates)
    {
        var wikiResult = await wClient.FindPlacesAsync(coordinates);
        var paralledDescs = wikiResult.Select(DescribePlaceAsync);
        
        var describedPlaces = await Task.WhenAll(paralledDescs);
        return describedPlaces;
    }

    private async Task<Place> DescribePlaceAsync(Place place)
    {
        try
        {
            var desc = await wClient.GetDescAsync(place.PageNum);
            return place with { Description = desc };
        }
        catch (Exception e) when (ErrorMessages.IsExpected(e))
        {
            return place with { DescriptionError = ErrorMessages.Describe(e) };
        }
    }
}