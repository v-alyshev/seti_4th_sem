using System.Text.Json.Serialization;

namespace seti3.Api.Responses;

public record GraphHopperResponse(List<Hit> Hits);

public record Hit(
    GeoPoint? Point = null,
    string? Name = null,
    string? Street = null,
    string? City = null,
    string? Country = null,
    [property: JsonPropertyName("osm_value")] string? OsmVal = null);

public record GeoPoint(double Lat, double Lng);