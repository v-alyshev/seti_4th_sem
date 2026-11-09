using DotNetEnv;
using seti3.Api;
using seti3.Services;
using seti3.UI;
using seti3.Config;

namespace seti3;

public sealed class App(HttpClient http, PlaceService service, ConsoleUI ui) : IDisposable
{
    private const string ConfigPath = "Config/Config.env";
    private const string UserAgent = "seti3 lab";
    private const long MaxResponseContentBufferSize = 1000000;
    public static App Create()
    {
        Env.Load(ConfigPath);
        var config = AppConfig.FromEnvironment();

        var http = new HttpClient { Timeout = config.Timeout, MaxResponseContentBufferSize =  MaxResponseContentBufferSize };
        http.DefaultRequestHeaders.UserAgent.ParseAdd(UserAgent);

        var service = new PlaceService(
            new GraphHopperClient(config.Language, http, config.GraphHopperKey),
            new OpenWeatherClient(config.Language, http, config.OpenWeatherKey),
            new WikiClient(config.Language, http));

        return new App(http, service, new ConsoleUI());
    }

    public async Task RunAsync()
    {
        while (true)
        {
            try
            {
                var query = ui.SearchQuery();
                var locations = await service.GetLocationsAsync(query);
                if (locations.Count == 0)
                {
                    ui.ShowNothing();
                    continue;
                }

                do
                {
                    var location = ui.ChooseLocation(locations);
                    var report = await service.GetLocationReportAsync(location);
                    ui.ShowReport(report);
                }
                while (ui.AskChooseAnother());
            }
            catch (Exception e) when (ErrorMessages.IsExpected(e))
            {
                ui.ShowErr(ErrorMessages.Describe(e));
            }
        }
    }
    public void Dispose() => http.Dispose();
}
