namespace seti3;

public static class Program
{
    public static async Task<int> Main()
    {
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
    }
}