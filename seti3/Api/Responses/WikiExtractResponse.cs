namespace seti3.Api.Responses;

public record WikiExtractResponse(ExtractQuery? Query = null, WikiError? Error = null);

public record ExtractQuery(List<ExtractPage> Pages);

public record ExtractPage(string? Extract = null);