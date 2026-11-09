using System.Text.Json.Serialization;

namespace seti3.Api.Responses;

public record OpenWeatherResponse(List<WeatherInfo> Weather, MainInfo Main, WindInfo Wind);

public record WeatherInfo(string Description);

public record MainInfo(double Temp, [property: JsonPropertyName("feels_like")] double FeelsLike, int Humidity, double Pressure);

public record WindInfo(double Speed);
