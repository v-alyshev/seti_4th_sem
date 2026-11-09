using System.Globalization;
using System.Net.Http.Json;
using seti3.Api.Responses;
using seti3.Models;

namespace seti3.Api;

public class WikiClient(string language, HttpClient client) : IPlacesClient
{
    private const int Gsradius = 10000;
    private const int Gslimit = 10;
    private readonly string _address = $"https://{language}.wikipedia.org/w/api.php";


    public async Task<List<Place>> FindPlacesAsync(Coordinates coordinates)
    {
        string parsedCoordinates = string.Create(CultureInfo.InvariantCulture,
            $"{coordinates.Latitude}|{coordinates.Longitude}");
        
        var url = UrlBuilder.Build(_address, new Dictionary<string, object>()
            {
                ["action"] = "query", ["list"] = "geosearch", ["gscoord"] = parsedCoordinates, 
                ["gsradius"] = Gsradius, ["gslimit"] = Gslimit, ["format"] = "json", ["formatversion"] = 2
            });
        var response = await client.GetFromJsonAsync<WikiGeosearchResponse>(url, JsonSet.Options);
        var query = CheckValid(response?.Error, response?.Query);
        
        var places = new List<Place>();
        foreach (var nonParsedPlace in query.GeoSearch)
        {
            places.Add(new Place(nonParsedPlace.PageId, nonParsedPlace.Title, nonParsedPlace.Dist, null));
        }

        return places;
    }

    public async Task<string?> GetDescAsync(long pageId)
    {
        var url = UrlBuilder.Build(_address, new Dictionary<string, object>()
        {
            ["action"] = "query", ["prop"] = "extracts", ["pageids"] = pageId, ["exintro"] = 1,
            ["explaintext"] = 1, ["format"] = "json", ["formatversion"] = 2
        });
        var response = await client.GetFromJsonAsync<WikiExtractResponse>(url, JsonSet.Options);
        var query = CheckValid(response?.Error, response?.Query);

        return query.Pages[0].Extract;
    }

    private TQuery CheckValid<TQuery>(WikiError? error, TQuery? query) where TQuery : class
    {
        if (error is not null)
        {
            throw new InvalidOperationException($"Ошибка Википедии: {error.Code}, {error.Info}");
        }

        return query ?? throw new InvalidOperationException("Ошибка Википедии: пустой ответ");
    }
}