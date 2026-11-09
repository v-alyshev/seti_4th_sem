using System.Net.Http.Json;
using seti3.Api.Responses;
using seti3.Models;

namespace seti3.Api;

public class GraphHopperClient(string language, HttpClient client, string key) : IGeocodingClient
{
    private const string Address = "https://graphhopper.com/api/1/geocode";
    private const int Limit = 10;
    
    public async Task<List<Location>>SearchLocationsAsync(string request)
    {
        var url = UrlBuilder.Build(Address, new Dictionary<string, object>()
        {
            ["q"] = request, ["locale"] = language, ["limit"] = Limit, ["key"] = key 
        });
        var response = await client.GetFromJsonAsync<GraphHopperResponse>(url, JsonSet.Options);
        if (response is null)
        {
            throw new InvalidOperationException("GraphHopperApi вернул null");
        }

        var locations = new List<Location>(); 
        foreach (var hit in response.Hits)
        {
            if (hit.Point is null)
            {
                continue;
            }
            var cord = new Coordinates(hit.Point.Lat, hit.Point.Lng);
            Location location = new Location(cord, hit.Name ?? "Без названия", hit.Street, hit.City,hit.Country,hit.OsmVal);
            locations.Add(location);
        }

        return locations;
    }
}