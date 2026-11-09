namespace seti3.Api.Responses;

public record WikiGeosearchResponse(GeoQuery? Query = null, WikiError? Error = null);

public record GeoQuery(List<GeoSearchPlace> GeoSearch);

public record GeoSearchPlace(long PageId, string Title, double Dist);