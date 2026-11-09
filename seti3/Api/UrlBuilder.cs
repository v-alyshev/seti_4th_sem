namespace seti3.Api;
using System.Globalization;

public static class UrlBuilder
{
    public static string Build(string address, IReadOnlyDictionary<string, object> parametrs)
    {
        string url = address;
        if (parametrs.Count == 0)
        {
            return url;
        }

        var parsedParams = new List<string>();
        foreach (var paramPair in parametrs)
        {
            parsedParams.Add(Uri.EscapeDataString(paramPair.Key) +
                             '=' + 
                             Uri.EscapeDataString(Convert.ToString(paramPair.Value, CultureInfo.InvariantCulture) ?? ""));
            
        }

        url+= '?' + string.Join('&', parsedParams);
        return url;
    }
}