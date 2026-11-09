namespace seti3.Config;

public record AppConfig(string GraphHopperKey, string OpenWeatherKey, string Language, TimeSpan Timeout)
{
    private static string Require(string name)
    {
        string? envVal =  Environment.GetEnvironmentVariable(name);
        if (string.IsNullOrWhiteSpace(envVal))
        {
            throw new InvalidOperationException($"Пустой или отсутствующий ключ для {name}");
        }
        return  envVal;
    }

    public static AppConfig FromEnvironment()
    {
        int timeout = int.TryParse(Environment.GetEnvironmentVariable("REQUEST_TIMEOUT_SECONDS"), out int parsedTimeout) 
                      && parsedTimeout > 0 ? parsedTimeout : 15; 
        
        return new AppConfig(
            GraphHopperKey: Require("GRAPHHOPPER_API_KEY"),
            OpenWeatherKey: Require("OPENWEATHER_API_KEY"),
            Language: Require("LANGUAGE"),
            Timeout: TimeSpan.FromSeconds(timeout)
        );
    }
    
}