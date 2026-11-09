using System.Net.Http.Json;
using seti3.Api.Responses;
using seti3.Models;

namespace seti3.Api;

public class OpenWeatherClient(string language, HttpClient client, string key) : IWeatherClient
{
 private const string Address = "https://api.openweathermap.org/data/2.5/weather";
 private const string Units = "metric";
 private double ToMmHg(double pascals)
 {
  return pascals * 100 / 133.322;
 }
 public async Task<Weather> GetWeatherInfoAsync(Coordinates coordinates)
 {
  var url = UrlBuilder.Build(Address, new Dictionary<string, object>()
  {
   ["lat"] = coordinates.Latitude, ["lon"] = coordinates.Longitude,
   ["units"] = Units, ["lang"] = language, ["appid"] = key
  });

  var response = await client.GetFromJsonAsync<OpenWeatherResponse>(url, JsonSet.Options);
  if (response is null)
  {
   throw new InvalidOperationException("OpenWeatherApi вернул null");
  }

  return new Weather(response.Weather[0].Description, response.Main.Temp,
   response.Main.FeelsLike, response.Main.Humidity, response.Wind.Speed, ToMmHg(response.Main.Pressure) );
 }
}