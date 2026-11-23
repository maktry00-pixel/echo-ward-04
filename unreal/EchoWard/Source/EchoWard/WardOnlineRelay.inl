constexpr uint32 MaxRelayPeer = 0xffffff;

sockaddr_in RelayEndpoint(uint32 Peer)
{
    return MakeEndpoint(0xf0000000 | Peer, 0);
}

uint32 RelayPeer(const sockaddr_in& Endpoint)
{
    const uint32 Address = ntohl(Endpoint.sin_addr.s_addr);
    return Endpoint.sin_port == 0 && (Address & 0xff000000) == 0xf0000000 ? Address & MaxRelayPeer : 0;
}

uint32 ReadPeer(const Json& Data, const TCHAR* Field)
{
    const double Value = Number(Data, Field);
    return Value > 0 && Value <= MaxRelayPeer ? static_cast<uint32>(Value) : 0;
}

bool WriteWire(const Json& Packet, FString& Wire)
{
    return FJsonSerializer::Serialize(Packet.ToSharedRef(),
        TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Wire));
}

Json ReadWire(const FString& Wire)
{
    Json Packet;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Wire), Packet);
    return Packet;
}

FString RelayUrl()
{
    FString Url;
    if (!FParse::Value(FCommandLine::Get(), TEXT("EchoRelay="), Url))
    {
        const FString File = FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("../../../EchoRelay.txt"));
        FFileHelper::LoadFileToString(Url, *File);
    }
    Url.TrimStartAndEndInline();
    Url.RemoveFromEnd(TEXT("/"));
    if (Url.Len() > 240 || (!Url.StartsWith(TEXT("https://")) &&
        !Url.StartsWith(TEXT("http://127.0.0.1:")))) return {};
    return Url + TEXT("/v1/poll");
}

struct FHttpHandle
{
    HINTERNET Value;
    explicit FHttpHandle(HINTERNET Handle) : Value(Handle) {}
    ~FHttpHandle() { if (Value) WinHttpCloseHandle(Value); }
};

struct FRelayReply
{
    uint32 Status = 0;
    FString Wire;
    int32 Ping = 0;
};

FRelayReply PollRelay(const FString& Url, const FString& Wire)
{
    const double Start = FPlatformTime::Seconds();
    FRelayReply Reply;
    URL_COMPONENTS Parts{};
    Parts.dwStructSize = sizeof(Parts);
    Parts.dwHostNameLength = Parts.dwUrlPathLength = Parts.dwUserNameLength = Parts.dwPasswordLength = DWORD(-1);
    if (!WinHttpCrackUrl(*Url, Url.Len(), 0, &Parts) || !Parts.dwHostNameLength ||
        Parts.dwUserNameLength || Parts.dwPasswordLength) return Reply;
    const FString Host(Parts.dwHostNameLength, Parts.lpszHostName);
    const FString Path(Parts.dwUrlPathLength, Parts.lpszUrlPath);
    const bool Secure = Parts.nScheme == INTERNET_SCHEME_HTTPS;
    if ((!Secure && Host != TEXT("127.0.0.1")) || Path != TEXT("/v1/poll")) return Reply;

    FHttpHandle Session(WinHttpOpen(L"ECHO beta relay", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, nullptr, nullptr, 0));
    if (!Session.Value) return Reply;
    WinHttpSetTimeouts(Session.Value, 2500, 2500, 2500, 2500);
    FHttpHandle Connection(WinHttpConnect(Session.Value, *Host, Parts.nPort, 0));
    if (!Connection.Value) return Reply;
    FHttpHandle Request(WinHttpOpenRequest(Connection.Value, L"POST", *Path, nullptr, nullptr, nullptr,
        Secure ? WINHTTP_FLAG_SECURE : 0));
    if (!Request.Value) return Reply;
    DWORD RedirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    if (!WinHttpSetOption(Request.Value, WINHTTP_OPTION_REDIRECT_POLICY, &RedirectPolicy, sizeof(RedirectPolicy))) return Reply;
    FTCHARToUTF8 Bytes(*Wire);
    if (!WinHttpSendRequest(Request.Value, L"Content-Type: application/json\r\n", DWORD(-1),
        const_cast<char*>(Bytes.Get()), Bytes.Length(), Bytes.Length(), 0) ||
        !WinHttpReceiveResponse(Request.Value, nullptr)) return Reply;
    DWORD Size = sizeof(Reply.Status);
    if (!WinHttpQueryHeaders(Request.Value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        nullptr, &Reply.Status, &Size, nullptr) || Reply.Status != 200) return Reply;
    TArray<uint8> Body;
    uint8 Buffer[4096];
    DWORD Read = 0;
    do
    {
        if (!WinHttpReadData(Request.Value, Buffer, sizeof(Buffer), &Read) || Body.Num() + Read > 240000)
            return {};
        Body.Append(Buffer, Read);
    } while (Read > 0);
    if (!Body.IsEmpty())
    {
        const FUTF8ToTCHAR Text(reinterpret_cast<const char*>(Body.GetData()), Body.Num());
        Reply.Wire = FString(Text.Length(), Text.Get());
    }
    Reply.Ping = FMath::Max(1, FMath::RoundToInt((FPlatformTime::Seconds() - Start) * 1000.0));
    return Reply;
}

struct FRelayMessage
{
    uint32 To;
    FString Type;
    FString Wire;
};
