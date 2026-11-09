using System.Text.Json;

namespace seti3.Api;

public static class JsonSet
{
    public static readonly JsonSerializerOptions Options = new JsonSerializerOptions
    {
        PropertyNameCaseInsensitive = true,
        RespectRequiredConstructorParameters = true
    };
}