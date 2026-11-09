using seti3;

try
{
    using var app = App.Create();
    await app.RunAsync();
    return 0;
}
catch (EndOfStreamException)
{
    return 0;
}
catch (Exception error) when (ErrorMessages.IsExpected(error))
{
    Console.Error.WriteLine($"Ошибка: {ErrorMessages.Describe(error)}");
    return 1;
}