using System.Net;
using System.Text.Json;

namespace seti3;

public static class ErrorMessages
{
    public static bool IsExpected(Exception error) =>
        error is HttpRequestException or TaskCanceledException
            or InvalidOperationException or JsonException;
    public static string Describe(Exception error)
    {
        switch (error)
        {
            case HttpRequestException { StatusCode: HttpStatusCode.Unauthorized }:
                return "Неверный или неактивированный API-ключ";
            case HttpRequestException { StatusCode: HttpStatusCode.TooManyRequests }:
                return "Превышен лимит запросов, попробуйте позже";
            case HttpRequestException { StatusCode: null }:
                return "Нет связи с сервером";
            case HttpRequestException { StatusCode: { } code }:
                return $"Сервер ответил ошибкой {(int)code}";
            case TaskCanceledException:
                return "Сервер не ответил вовремя";
            case JsonException:
                return "Сервис вернул неожиданный ответ";
            default:
                return error.Message;
        }
    }
}