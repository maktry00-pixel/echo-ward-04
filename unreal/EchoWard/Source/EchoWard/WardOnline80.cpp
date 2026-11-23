#include "WardOnline80.h"

#include "WardAudio68.h"
#include "WardRescue104.h"
#include "WardHideout102.h"
#include "WardAdmin79.h"
#include "WardCombat63.h"
#include "WardContact98.h"
#include "WardHorror99.h"
#include "WardDoor100.h"
#include "WardGameplay76.h"
#include "WardHomeAssets53.h"
#include "WardMonster59.h"
#include "WardPeople54.h"
#include "WardMonsterVisual59.h"
#include "WardStaging64.h"
#include "Animation/AnimSequence.h"
#include "Animation/SkeletalMeshActor.h"
#include "Async/Async.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "WardBookWorld.h"
#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "WardHospitalGameplay.h"
#include "WardTitleScene.h"
#include "WardTitleMenuStyle.h"
#include "WardOnlineRoster92.h"
#include "Brushes/SlateColorBrush.h"
#include "Containers/Ticker.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Base64.h"
#include "Misc/Compression.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <MSWSock.h>
#include <winhttp.h>
#include "Windows/HideWindowsPlatformTypes.h"

namespace WardOnline80
{
namespace
{
using namespace WardData;

constexpr int FirstPort = 38040;
constexpr int PortCount = 8;
constexpr int Protocol = 1;
constexpr int CoopBuild = 104;
constexpr int PacketLimit = 8192;
constexpr double PeerTimeout = 10.0;
constexpr auto MapName = TEXT("Отделение 04");
const FString HomePage = TEXT("online");
const FString CreatePage = TEXT("online_create");
const FString BrowsePage = TEXT("online_browse");
const FString LobbyPage = TEXT("online_lobby");

const TArray<FString> Difficulties = {TEXT("Легко"), TEXT("Нормально"), TEXT("Сложно"), TEXT("Хардкор")};
const TArray<FString> AccessTypes = {TEXT("Открытая"), TEXT("По паролю"), TEXT("По приглашению")};
enum class EAccess : int32 { Public, Password, Invite };

FString Token()
{
    return FGuid::NewGuid().ToString(EGuidFormats::Digits);
}

FString Clean(FString Value, int32 Limit)
{
    Value.TrimStartAndEndInline();
    Value.LeftInline(Limit);
    Value.ReplaceInline(TEXT("\n"), TEXT(" "));
    Value.ReplaceInline(TEXT("\r"), TEXT(" "));
    Value.ReplaceInline(TEXT("\t"), TEXT(" "));
    return Value;
}

bool IsToken(const FString& Value)
{
    if (Value.Len() != 32) return false;
    for (const TCHAR Ch : Value)
        if (!FChar::IsHexDigit(Ch)) return false;
    return true;
}

FString Proof(const FString& Secret, const FString& Salt, const FString& Client)
{
    const FString SignedText = Salt + TEXT(":") + Client;
    const FTCHARToUTF8 Key(*Secret);
    const FTCHARToUTF8 Data(*SignedText);
    FSHAHash Hash;
    FSHA1::HMACBuffer(Key.Get(), Key.Length(), Data.Get(), Data.Length(), Hash.Hash);
    return Hash.ToString();
}

bool SameEndpoint(const sockaddr_in& A, const sockaddr_in& B)
{
    return A.sin_addr.s_addr == B.sin_addr.s_addr && A.sin_port == B.sin_port;
}

sockaddr_in MakeEndpoint(uint32 Address, int32 Port)
{
    sockaddr_in Result{};
    Result.sin_family = AF_INET;
    Result.sin_addr.s_addr = htonl(Address);
    Result.sin_port = htons(static_cast<uint16>(Port));
    return Result;
}

FString LocalAddress()
{
    char Host[256]{};
    if (gethostname(Host, sizeof(Host)) != 0) return TEXT("127.0.0.1");
    addrinfo Hints{};
    Hints.ai_family = AF_INET;
    addrinfo* Addresses = nullptr;
    if (getaddrinfo(Host, nullptr, &Hints, &Addresses) != 0) return TEXT("127.0.0.1");
    FString Chosen = TEXT("127.0.0.1");
    for (auto* Address = Addresses; Address; Address = Address->ai_next)
    {
        const auto& V4 = *reinterpret_cast<const sockaddr_in*>(Address->ai_addr);
        const uint32 Number = ntohl(V4.sin_addr.s_addr);
        if (!Number || Number >> 24 == 127 || Number >> 16 == 0xa9fe) continue;
        char Buffer[INET_ADDRSTRLEN]{};
        if (!inet_ntop(AF_INET, &V4.sin_addr, Buffer, sizeof(Buffer))) continue;
        Chosen = UTF8_TO_TCHAR(Buffer);
        if (Number >> 24 == 10 || Number >> 16 == 0xc0a8 || Number >> 20 == 0xac1) break;
    }
    freeaddrinfo(Addresses);
    return Chosen;
}

Json Message(const TCHAR* Type)
{
    auto Packet = MakeShared<FJsonObject>();
    Packet->SetNumberField(TEXT("echo"), Protocol);
    Packet->SetStringField(TEXT("type"), Type);
    return Packet;
}

#include "WardOnlineRelay.inl"

struct FRoom
{
    FString Id;
    FString Name;
    int32 MaxPlayers = 4;
    int32 Players = 1;
    int32 Difficulty = 1;
    int32 Chapter = 1;
    EAccess Access = EAccess::Public;
    bool Committed = false;
    bool FreshHome = false;
    int32 Ping = 0;
    double Seen = 0;
    sockaddr_in Address{};

    void Write(Json Packet) const
    {
        Packet->SetStringField(TEXT("room"), Id);
        Packet->SetStringField(TEXT("name"), Name);
        Packet->SetStringField(TEXT("map"), TEXT("ward04"));
        Packet->SetNumberField(TEXT("max"), MaxPlayers);
        Packet->SetNumberField(TEXT("players"), Players);
        Packet->SetNumberField(TEXT("difficulty"), Difficulty);
        Packet->SetNumberField(TEXT("chapter"), Chapter);
        Packet->SetNumberField(TEXT("coop_build"), CoopBuild);
        Packet->SetNumberField(TEXT("access"), static_cast<int32>(Access));
        Packet->SetBoolField(TEXT("committed"), Committed);
        Packet->SetBoolField(TEXT("fresh_home"), FreshHome);
    }

    bool Read(Json Packet, const sockaddr_in& Source)
    {
        Id = String(Packet, TEXT("room"));
        if (!IsToken(Id) || String(Packet, TEXT("map")) != TEXT("ward04")) return false;
        Name = Clean(String(Packet, TEXT("name")), 40);
        MaxPlayers = FMath::Clamp(int32(Number(Packet, TEXT("max"))), 2, 10);
        Players = FMath::Clamp(int32(Number(Packet, TEXT("players"))), 1, MaxPlayers);
        Difficulty = FMath::Clamp(int32(Number(Packet, TEXT("difficulty"))), 0, 3);
        Chapter = FMath::Clamp(int32(Number(Packet, TEXT("chapter"))), 1, 20);
        Access = static_cast<EAccess>(FMath::Clamp(int32(Number(Packet, TEXT("access"))), 0, 2));
        Committed = Boolean(Packet, TEXT("committed"));
        FreshHome = Boolean(Packet, TEXT("fresh_home"));
        Address = Source;
        Seen = FPlatformTime::Seconds();
        return !Name.IsEmpty();
    }
};

enum class ECarriedWeapon : uint8 { None, Pistol, Shotgun };

struct FMember
{
    FString Id;
    FString Name;
    sockaddr_in Address{};
    double LastSeen = 0;
    int32 ReadySequence = 0;
    bool Ready = false;
    bool Host = false;
    FString Credential;
    FString Profile;
    FVector Position = FVector::ZeroVector;
    float Yaw = 0;
    double LastPose = 0;
    bool Moving = false;
    float StepTravel = 0;
    bool Disconnected = false;
    int32 PoseFrame = 0;
    float Pitch = 0, Health = 100;
    int32 Rounds = 0, Reserve = 0, Shells = 0;
    bool Pistol = false, Shotgun = false;
    bool LoadoutReady = false, PreferShotgun = false, Aiming = false, LastShotLong = false;
    ECarriedWeapon HeldWeapon = ECarriedWeapon::None;
    int32 Finish = 0;
    double LastHit = 0, NextShot = 0, ReloadAt = 0;
    int32 Action = 0;
    int32 RespawnSerial = 0, ConfirmedRespawn = 0, ShotsFired = 0;
    bool GroupSpawn = false;
    int32 WorldAck = 0, WorldPart = 0;
    double RespawnAt = 0;
    double BleedUntil = 0, RescueLease = 0, ProtectedUntil = 0, RescueStarted = 0;
    float RescueProgress = 0;
    FString RescueIntent, Rescuer;
    FVector RescueStart = FVector::ZeroVector;
    FVector SeatPosition = FVector::ZeroVector, ShotOrigin = FVector::ZeroVector;
    TMap<FString, int32> Items;
    FString Notice;
    int32 Character = 0;
};

enum class ERole { Idle, Host, Joining, Guest };

class FOnline : public TSharedFromThis<FOnline>
{
public:
    explicit FOnline(AEWardCampaignMode& Game, bool DriveUI = true) : Owner(&Game), DrivePages(DriveUI) {}
    ~FOnline() { Close(); }

    TWeakObjectPtr<AEWardCampaignMode> Owner;
    ERole Role = ERole::Idle;
    FRoom Room;
    TArray<FRoom> Available;
    TArray<FMember> Members;
    FString Status;
    FString RoomName = TEXT("Ночная смена");
    FString PlayerName = TEXT("Игрок");
    FString Password;
    FString JoinPassword;
    FString Invitation;
    FString SelectedRoom;
    int32 PreviewCharacter = INDEX_NONE;
    int32 MaxPlayers = 4;
    int32 Difficulty = 1;
    EAccess Access = EAccess::Public;
    int32 Revision = 0;
    bool InMap = false;

    void Feedback(const FString& Text) { Status = Text; }

    bool Open(bool Hosting)
    {
        if (!Started)
        {
            WSADATA Data{};
            if (WSAStartup(MAKEWORD(2, 2), &Data) != 0) return false;
            Started = true;
        }
        CloseSocket();
        Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (Socket == INVALID_SOCKET) return false;
        // Probing multiple LAN ports normally reaches some closed ports.
        // Their ICMP responses must not interrupt reads of valid UDP replies.
        DWORD Disabled = 0;
        DWORD BytesReturned = 0;
        WSAIoctl(Socket, SIO_UDP_CONNRESET, &Disabled, sizeof(Disabled), nullptr, 0, &BytesReturned, nullptr, nullptr);
        unsigned long Nonblocking = 1;
        int Broadcast = 1;
        if (ioctlsocket(Socket, FIONBIO, &Nonblocking) != 0 ||
            setsockopt(Socket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&Broadcast), sizeof(Broadcast)) != 0)
        {
            CloseSocket();
            return false;
        }
        const int32 Attempts = Hosting ? PortCount : 1;
        for (int32 Index = 0; Index < Attempts; ++Index)
        {
            Port = Hosting ? FirstPort + Index : 0;
            const auto BindAddress = MakeEndpoint(INADDR_ANY, Port);
            if (bind(Socket, reinterpret_cast<const sockaddr*>(&BindAddress), sizeof(BindAddress)) == 0) return true;
        }
        CloseSocket();
        return false;
    }

    void Host()
    {
        const FString Name = Clean(RoomName, 40);
        const FString Player = Clean(PlayerName, 24);
        if (Name.IsEmpty() || Player.IsEmpty()) { Feedback(TEXT("Введи имя игрока и название комнаты.")); return; }
        if (Access == EAccess::Password && Password.Len() < 4) { Feedback(TEXT("В пароле должно быть от 4 до 32 символов.")); return; }
        InitialCheckpoint = Checkpoint(SelectedSlot);
        if (ContinueSaved && !InitialCheckpoint) { Feedback(TEXT("В этом слоте нет сетевой контрольной точки.")); return; }
        if (!ContinueSaved && InitialCheckpoint && !ChapterTwoTest)
        { Feedback(TEXT("Этот слот занят. Выбери пустой слот или продолжи сохранение.")); return; }
        if (!ContinueSaved) InitialCheckpoint.Reset();
        StoredPlayers = Object(InitialCheckpoint, TEXT("members"));
        ContinuationId = String(InitialCheckpoint, TEXT("run"), Token());
        const auto SavedCampaign = Object(InitialCheckpoint, TEXT("campaign"));
        StartChapter = InitialCheckpoint ? int32(Number(Object(SavedCampaign, TEXT("values")), TEXT("hospital_chapter"), 1)) :
            ChapterTwoTest ? 2 : 1;
        if (!Open(true)) { Feedback(TEXT("Не удалось открыть порт комнаты.")); return; }
        Role = ERole::Host;
        RelayActive = true;
        Room = FRoom{};
        Room.Id = Token();
        RoomRoute = Token();
        Room.Name = Name;
        Room.MaxPlayers = MaxPlayers;
        Room.Difficulty = Difficulty;
        Room.Chapter = StartChapter;
        Room.FreshHome = !InitialCheckpoint && StartChapter == 1;
        if (InitialCheckpoint)
        {
            const FString SavedDifficulty = String(SavedCampaign, TEXT("difficulty"));
            if (SavedDifficulty == TEXT("story")) Room.Difficulty = 0;
            else if (SavedDifficulty == TEXT("normal")) Room.Difficulty = 1;
            else if (SavedDifficulty == TEXT("nightmare")) Room.Difficulty = 2;
            else if (SavedDifficulty == TEXT("hardcore")) Room.Difficulty = 3;
        }
        Room.Access = Access;
        Code = Token();
        Salt = Token();
        ActivePassword = Password;
        Password.Empty();
        Members = {{Token(), Player, {}, 0, 0, false, true, {}}};
        Self = Members[0].Id;
        InviteCode = FString::Printf(TEXT("%s:%d/%s"), *LocalAddress(), Port, *Code);
        Feedback(RelayAddress.IsEmpty() ? TEXT("Комната открыта в локальной сети.") : TEXT("Комната открыта. Подключаем интернет-каталог…"));
        Show(LobbyPage);
    }

    void Browse(bool Manual = true)
    {
        if (Socket == INVALID_SOCKET && !Open(false))
        {
            Feedback(TEXT("Не удалось открыть подключение к локальной сети."));
            return;
        }
        RelayActive = RelayBrowse = true;
        if (Manual) Feedback(RelayAddress.IsEmpty() ? TEXT("Поиск комнат в локальной сети…") : TEXT("Поиск комнат в интернете и локальной сети…"));
        if (FParse::Param(FCommandLine::Get(), TEXT("EchoRelayOnly"))) return;
        DiscoveryTime = FPlatformTime::Seconds();
        DiscoveryId = Token();
        const auto Packet = Message(TEXT("discover"));
        Packet->SetStringField(TEXT("request"), DiscoveryId);
        for (int32 Index = 0; Index < PortCount; ++Index)
        {
            Send(Packet, MakeEndpoint(INADDR_BROADCAST, FirstPort + Index));
            Send(Packet, MakeEndpoint(INADDR_LOOPBACK, FirstPort + Index));
        }
    }

    void JoinFound()
    {
        const auto* Listing = Available.FindByPredicate([this](const FRoom& Entry) { return Entry.Id == SelectedRoom; });
        if (!Listing) { Feedback(TEXT("Выбери доступную комнату.")); return; }
        if (Listing->Access == EAccess::Password && JoinPassword.IsEmpty())
        { Feedback(TEXT("Введи пароль выбранной комнаты.")); return; }
        Connect(Listing->Address, Listing->Id, {});
    }

    void JoinCode()
    {
        FString Address, Secret, Ip, PortText;
        if (!Invitation.TrimStartAndEnd().Split(TEXT("/"), &Address, &Secret) || !IsToken(Secret))
        {
            Feedback(TEXT("Вставь полный код приглашения."));
            return;
        }
        if (Address.RemoveFromStart(TEXT("ECHO-"), ESearchCase::IgnoreCase))
        {
            if (Address.IsEmpty() || Address.Len() > 8 || !Address.IsNumeric() || RelayAddress.IsEmpty())
            { Feedback(TEXT("Для этого приглашения нужен доступ к интернет-серверу ЭХО.")); return; }
            const int32 Peer = FCString::Atoi(*Address);
            if (Peer <= 0 || Peer > static_cast<int32>(MaxRelayPeer))
            { Feedback(TEXT("Неверный код комнаты.")); return; }
            Connect(RelayEndpoint(Peer), {}, Secret);
            return;
        }
        if (!Address.Split(TEXT(":"), &Ip, &PortText) || PortText.Len() > 5 || !PortText.IsNumeric())
        { Feedback(TEXT("Неверный адрес комнаты в приглашении.")); return; }
        const int32 JoinPort = FCString::Atoi(*PortText);
        auto Endpoint = MakeEndpoint(INADDR_ANY, JoinPort);
        if (JoinPort < FirstPort || JoinPort >= FirstPort + PortCount ||
            inet_pton(AF_INET, TCHAR_TO_UTF8(*Ip), &Endpoint.sin_addr) != 1 ||
            Endpoint.sin_addr.s_addr == htonl(INADDR_ANY) || Endpoint.sin_addr.s_addr == htonl(INADDR_BROADCAST))
        {
            Feedback(TEXT("Неверный адрес комнаты в приглашении."));
            return;
        }
        Connect(Endpoint, {}, Secret);
    }

    bool IsReady() const
    {
        const auto* Member = Members.FindByPredicate([this](const FMember& Entry) { return Entry.Id == Self; });
        return Role == ERole::Guest ? RequestedReady : Member && Member->Ready;
    }

    bool CanStart() const
    {
        return Role == ERole::Host && Members.Num() >= 2 && !Room.Committed &&
            !Members.ContainsByPredicate([](const FMember& Entry) { return !Entry.Ready; });
    }

    int32 LocalCharacter() const
    {
        const auto* Member = Members.FindByPredicate([this](const FMember& Entry) { return Entry.Id == Self; });
        return Member ? Member->Character : INDEX_NONE;
    }

    bool CharacterOccupied(int32 Character) const
    {
        return Members.ContainsByPredicate([&](const FMember& Entry) {
            return Entry.Character == Character && Entry.Id != Self;
        });
    }

    bool CanChoose(int32 Character) const
    {
        return Role == ERole::Guest && !Room.Committed && PendingCharacter == INDEX_NONE &&
            Character > 0 && Character < WardOnlineRoster92::Count &&
            Character != LocalCharacter() && !CharacterOccupied(Character);
    }

    bool CharacterPending() const { return PendingCharacter != INDEX_NONE; }

    void ChooseCharacter(int32 Character)
    {
        if (!CanChoose(Character)) return;
        PendingCharacter = Character;
        RequestedReady = false;
        ++ReadySequence;
        ++Revision;
        SendPresence();
    }

    void ToggleReady()
    {
        if (Room.Committed || CharacterPending()) return;
        if (Role == ERole::Host)
        {
            Members[0].Ready = !Members[0].Ready;
            ++Revision;
            BroadcastState();
        }
        else if (Role == ERole::Guest)
        {
            RequestedReady = !RequestedReady;
            ++ReadySequence;
            SendPresence();
        }
    }

    void Start()
    {
        if (!CanStart()) return;
        Room.Committed = true;
        ++Revision;
        PrepareMap();
        if (DrivePages && !InMap && !FParse::Param(FCommandLine::Get(), TEXT("WardOnlineCheck80")))
            Room.Committed = false;
        BroadcastState();
    }

    void Reopen()
    {
        if (Role != ERole::Host || !Room.Committed) return;
        Room.Committed = false;
        Feedback(TEXT("Приём игроков возобновлён."));
        ++Revision;
        BroadcastState();
    }

    FString InvitationCode() const
    {
        return RelayId && RelayOnline ? FString::Printf(TEXT("ECHO-%u/%s"), RelayId, *Code) : InviteCode;
    }
    bool InternetConfigured() const { return !RelayAddress.IsEmpty(); }
    bool InternetConnected() const { return RelayOnline; }
    bool Reconnecting() const { return ReconnectUntil > 0; }
    bool TestRebind()
    {
        if (Role != ERole::Guest || !Room.Committed || !Open(false)) return false;
        StartReconnect(FPlatformTime::Seconds());
        return true;
    }
    void TestProfile(const FString& Id) { ProfileId = Id; }
    void TestDropTraffic(double Seconds) { TestMutedUntil = FPlatformTime::Seconds() + Seconds; }
    const ASkeletalMeshActor* RemoteAvatar() const
    {
        for (const auto& Pair : Avatars)
            if (const auto* Avatar = Pair.Value.Get(); Avatar && Avatar->GetSkeletalMeshComponent()->GetSkeletalMeshAsset())
                return Avatar;
        return nullptr;
    }

    void Leave()
    {
        const bool WasHost = Role == ERole::Host;
        const bool WasPlaying = InMap;
        if (WasHost && WasPlaying)
            if (const auto* Game = Owner.Get(); Game && Game->Flag(TEXT("autosave_pending"))) WriteCheckpoint(false);
        Close();
        Available.Reset();
        SelectedRoom.Empty();
        PreviewCharacter = INDEX_NONE;
        Feedback(WasHost ? TEXT("Комната закрыта.") : TEXT("Ты вышел из комнаты."));
        if (WasPlaying) Show(TEXT("title"));
        Show(HomePage);
    }

    void Close()
    {
        if (Role == ERole::Host)
        {
            const auto Packet = Message(TEXT("closed"));
            Packet->SetStringField(TEXT("room"), Room.Id);
            for (const auto& Member : Members)
            {
                if (Member.Host) continue;
                Packet->SetStringField(TEXT("member"), Member.Credential);
                Send(Packet, Member.Address);
            }
        }
        else if (Role == ERole::Guest)
        {
            const auto Packet = MemberPacket(TEXT("leave"));
            Send(Packet, Room.Address);
        }
        Role = ERole::Idle;
        ReconnectUntil = NextResume = 0;
        PendingHost = {};
        LeaveMap();
        RelayClosing |= RelayActive;
        RelayActive = RelayBrowse = false;
        RelayOnline = false;
        Members.Reset();
        ActivePassword.Empty();
        JoinPassword.Empty();
        ConnectionPassword.Empty();
        RequestedReady = false;
        ReadySequence = 0;
        PendingCharacter = INDEX_NONE;
        Credential.Empty();
        Available.Reset();
        SelectedRoom.Empty();
        if (Socket != INVALID_SOCKET) CloseSocket();
        if (Started) { WSACleanup(); Started = false; }
        ++Revision;
    }

    void Tick()
    {
        const double Now = FPlatformTime::Seconds();
        if (Now < TestMutedUntil) return;
        TickRelay(Now);
        if (Socket == INVALID_SOCKET) return;
        char Buffer[PacketLimit];
        for (int32 Count = 0; Count < 64; ++Count)
        {
            sockaddr_in Source{};
            int Length = sizeof(Source);
            const int Bytes = recvfrom(Socket, Buffer, sizeof(Buffer), 0, reinterpret_cast<sockaddr*>(&Source), &Length);
            if (Bytes == SOCKET_ERROR)
            {
                const int32 Error = WSAGetLastError();
                if (Error == WSAEMSGSIZE || Error == WSAECONNRESET) continue;
                break;
            }
            if (Bytes <= 0 || Source.sin_family != AF_INET) continue;
            const FUTF8ToTCHAR Characters(Buffer, Bytes);
            Json Packet;
            if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(FString(Characters.Length(), Characters.Get())), Packet) &&
                Packet && Number(Packet, TEXT("echo")) == Protocol)
                Receive(Packet, Source, Now);
            if (Socket == INVALID_SOCKET) return;
        }
        if (Role == ERole::Host)
        {
            if (Members.RemoveAll([this, Now](const FMember& Entry) {
                return !Entry.Host && Now - Entry.LastSeen > (InMap ? 90.0 : PeerTimeout);
            }))
            {
                if (!InMap) Room.Committed = false;
                PruneAvatars();
                ++Revision;
                Feedback(TEXT("Участник отключился."));
            }
            if (InMap)
            {
                for (auto& Member : Members)
                {
                    if (!Member.Host && !Member.Disconnected && Now - Member.LastSeen > PeerTimeout)
                    {
                        Member.Disconnected = true;
                        ++Revision;
                    }
                }
                if (auto* Game = Owner.Get(); Game && Game->HospitalStory)
                {
                    const int32 Chapter = Game->HospitalStory->Chapter();
                    if (Room.Chapter != Chapter) { Room.Chapter = Chapter; ++Revision; }
                    if (Now >= NextCheckpoint && Game->Flag(TEXT("autosave_pending")))
                    {
                        NextCheckpoint = Now + 2;
                        Game->Flag(TEXT("autosave_pending"), false);
                        if (!WriteCheckpoint(false)) Game->Flag(TEXT("autosave_pending"), true);
                    }
                }
            }
            if (Now >= NextSend)
            {
                BroadcastState();
                NextSend = Now + 0.6;
            }
        }
        else if (Role == ERole::Guest)
        {
            if (Now - LastHost > PeerTimeout && ReconnectUntil == 0)
            {
                if (!Room.Committed || !IsToken(Credential))
                { Disconnected(TEXT("Связь с хозяином потеряна.")); return; }
                StartReconnect(Now);
            }
            if (ReconnectUntil > 0)
            {
                if (Now >= ReconnectUntil)
                { Disconnected(TEXT("Сессия недоступна. Хозяин может продолжить с сетевого сохранения.")); return; }
                if (Now >= NextResume) SendResume(Now);
                return;
            }
            if (Now >= NextSend)
            {
                SendPresence();
                NextSend = Now + 0.7;
            }
        }
        else if (Role == ERole::Joining)
        {
            if (Now > JoinDeadline) { Disconnected(TEXT("Комната не ответила. Проверь приглашение и подключение к сети.")); return; }
            if (Now >= NextSend) SendJoin();
        }
        else if (auto* Game = Owner.Get(); Game && Game->Screen == BrowsePage)
        {
            if (Available.RemoveAll([Now](const FRoom& Entry) { return Now - Entry.Seen > 5.5; })) ++Revision;
            if (Now - DiscoveryTime > 2.0 && !FParse::Param(FCommandLine::Get(), TEXT("EchoRelayOnly"))) Browse(false);
        }
        TickMap(Now);
    }

private:
    SOCKET Socket = INVALID_SOCKET;
    bool DrivePages;
    bool Started = false;
    int32 Port = 0;
    FString Self, Code, Salt, ActivePassword, InviteCode, Credential;
    FString DiscoveryId, ExpectedRoom, JoinSecret, ConnectionPassword, JoinSalt, ConnectionName;
    sockaddr_in Target{};
    double NextSend = 0, DiscoveryTime = 0, LastHost = 0, JoinDeadline = 0;
    int32 ReadySequence = 0;
    bool RequestedReady = false;
    int32 PendingCharacter = INDEX_NONE;
    const FString RelayAddress = RelayUrl();
    FString RelayKey;
    uint32 RelayId = 0;
    bool RelayActive = false, RelayBrowse = false, RelayOnline = false, RelayClosing = false, PollWasClose = false;
    double NextPoll = 0;
    TFuture<FRelayReply> RelayPoll;
    mutable TArray<FRelayMessage> RelayMessages;
    TMap<FString, TWeakObjectPtr<ASkeletalMeshActor>> Avatars;
    double NextPose = 0;
    FString RoomRoute;
    double ReconnectUntil = 0;
    double NextResume = 0;
    double TestMutedUntil = 0;
    sockaddr_in PendingHost{};

#include "WardCoop83.inl"
#include "WardRescue104.inl"
#include "WardCheckpoint85.inl"
#include "WardLoadout87.inl"

    void PrepareMap()
    {
        auto* Game = Owner.Get();
        if (!DrivePages || !Game || !Game->BookWorld || !Game->Hero() ||
            FParse::Param(FCommandLine::Get(), TEXT("WardOnlineCheck80"))) return;
        static const TCHAR* Presets[] = {TEXT("story"), TEXT("normal"), TEXT("nightmare"), TEXT("hardcore")};
        InMap = true;
        Game->NewGame(Presets[Room.Difficulty]);
        LastWorld.Reset();
        PendingActions.Reset();
        NextAction = LastWorldSequence = LastAck = 0;
        WorldSequence = 0;
        IncomingWorldSequence = IncomingWorldPart = 0;
        IncomingWorld.Reset();
        WorldDeadline = 0;
        LocalPoseFrame = LastReceivedPose = 0;
        NextWorld = 0;
        NextPartyRefresh = 0;
        ReceivedRespawn = 0;
        LocalBleedUntil=0;LastRescueTick=0;LocalRescue.Reset();LocalRescueProgress=0;LocalRescuer.Reset();
        ReceivedLoadout = false;
        HeardShots.Reset();
        TeamSize = Members.Num();
        WorldWire.Reset();
        const bool Loaded = Role == ERole::Host && InitialCheckpoint;
        if ((Loaded && !Game->Restore(Object(InitialCheckpoint, TEXT("campaign")))) ||
            (!Loaded && Room.Chapter == 2 && !WardAdmin79::PrimeChapterTwo(*Game)))
        {
            Feedback(TEXT("Не удалось открыть сетевую главу."));
            InMap = false;
            return;
        }
        auto* Hero = Game->Hero();
        Game->Show(FString());
        if (Room.FreshHome) Game->HospitalStory->StartHome52();
        const FVector Start = Hero->GetActorLocation();
        CoopStart = ValidPartyPosition(*Game, Start) ? Start : Game->BookWorld->Start();
        TrackedArrivalStage = Game->HospitalStory->ArrivalStage;
        LastSavePosition = CoopStart;
        const int32 Seat = FMath::Max(0, Members.IndexOfByPredicate([&](const FMember& Member) { return Member.Id == Self; }));
        ApplyLocalWeaponFinish(*Game, Seat);
        const auto Seats = FindSeats(*Game, CoopStart);
        const FVector Position = Seat == 0 ? Start : Seats.IsValidIndex(Seat) ? Seats[Seat] : CoopStart;
        Hero->SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
        NextPose = 0;
        Game->Show(FString());
        if (!Room.FreshHome)
        {
            if (auto* Controller = Cast<APlayerController>(Hero->GetController()))
                Controller->SetViewTarget(Hero);
        }
        Hero->Flashlight->SetVisibility(false);
        if (Role == ERole::Host && !Loaded && Room.Chapter == 2)
        {
            Game->Flag(TEXT("weapon"), true);
            Hero->Rounds = 9;
            Hero->Reserve = ReservePerPlayer();
        }
        if (Role == ERole::Host)
        {
            for (auto& Member : Members)
            {
                const int32 Index = &Member - Members.GetData();
                Member.Finish = Index;
                Member.SeatPosition = Seats.IsValidIndex(Index) ? Seats[Index] : CoopStart;
                if (!Member.Host)
                {
                    ReadPersonalCheckpoint(Member);
                    if (Loaded) PlaceMember(Member, Member.SeatPosition);
                }
            }
            if (!Loaded) BalanceOpponents(*Game);
            else RestoreChapterTwoBalance(*Game);
            EnsurePartyLoadouts(*Game);
            Game->AutoSave();
        }
        Hero->UpdateFirstPerson50();
        Game->Notify(Seat ? TEXT("Спутник Беки") : TEXT("Бека · хозяин команды"), 4);
        UE_LOG(LogTemp, Display, TEXT("ONLINE83 chapter=%d entered; slot=%d; host=%d"),
            Game->HospitalStory->Chapter(), Seat, Role == ERole::Host);
    }

    void LeaveMap()
    {
        if (!InMap) return;
        if (auto* Game=Owner.Get();Game&&LocalBleedUntil>0&&Game->Hero()) ApplyRescueStance(*Game,false);
        LocalBleedUntil=0;LocalRescue.Reset();LocalRescuer.Reset();LocalRescueProgress=0;
        InMap = false;
        for (const auto& Pair : Avatars)
            if (auto* Avatar = Pair.Value.Get()) Avatar->Destroy();
        Avatars.Reset();
        AvatarWeapons.Reset();
        MobGuests.Reset();
        PendingActions.Reset();
        LastWorld.Reset();
        HeardShots.Reset();
        TeamSize = 0;
        if (auto* Game = Owner.Get())
        {
            ClearLocalWeaponFinish(*Game);
            RestoreOpponents(*Game);
            Game->CampaignStarted = false;
            if (Game->HospitalStory) Game->HospitalStory->SetActorTickEnabled(true);
        }
    }

    void PruneAvatars()
    {
        for (auto It = Avatars.CreateIterator(); It; ++It)
        {
            if (Members.ContainsByPredicate([&](const FMember& Member) { return Member.Id == It.Key(); })) continue;
            if (auto* Avatar = It.Value().Get()) Avatar->Destroy();
            AvatarWeapons.Remove(It.Key());
            It.RemoveCurrent();
        }
    }

    static bool ReadPose(const Json& Packet, FMember& Member, double Now)
    {
        const auto& At = Array(Packet, TEXT("at"));
        if (At.Num() != 3 || At.ContainsByPredicate([](const auto& Component) {
            return Component->Type != EJson::Number || !FMath::IsFinite(Component->AsNumber()) ||
                FMath::Abs(Component->AsNumber()) > 150000;
        })) return false;
        const double Yaw = Number(Packet, TEXT("yaw"));
        if (!FMath::IsFinite(Yaw) || FMath::Abs(Yaw) > 360) return false;
        Member.Position = FVector(At[0]->AsNumber(), At[1]->AsNumber(), At[2]->AsNumber());
        Member.Yaw = static_cast<float>(Yaw);
        Member.Moving = Boolean(Packet, TEXT("moving"));
        Member.LastPose = Now;
        return true;
    }

    static void WritePose(const Json& Packet, const FVector& At, float Yaw, bool Moving)
    {
        Packet->SetArrayField(TEXT("at"), {MakeShared<FJsonValueNumber>(At.X),
            MakeShared<FJsonValueNumber>(At.Y), MakeShared<FJsonValueNumber>(At.Z)});
        Packet->SetNumberField(TEXT("yaw"), FRotator::NormalizeAxis(Yaw));
        Packet->SetBoolField(TEXT("moving"), Moving);
    }

    void TickMap(double Now)
    {
        auto* Game = Owner.Get();
        if (!InMap || !Game || !Game->Hero()) return;
        auto* Hero = Game->Hero();
        if (Role == ERole::Host)
        {
            UpdatePrologueSeats(*Game);
            EnsurePartyLoadouts(*Game);
            SyncHostRescue(*Game,Now);
            UpdateParty(*Game, Now);
        }
        if (Now >= NextPose)
        {
            NextPose = Now + 0.1;
            const FVector At = Hero->GetActorLocation();
            const float Yaw = Hero->GetActorRotation().Yaw;
            const bool Moving = Hero->GetVelocity().SizeSquared2D() > 900;
            if (Role == ERole::Host && !Members.IsEmpty())
            {
                Members[0].Position = At;
                Members[0].Yaw = Yaw;
                Members[0].Moving = Moving;
                Members[0].HeldWeapon = IsLocalDown()?ECarriedWeapon::None:LocalCarriedWeapon(*Game);
                Members[0].Aiming = Hero->Aiming;
                Members[0].Pitch = FRotator::NormalizeAxis(Hero->GetControlRotation().Pitch);
                Members[0].LastPose = Now;
                const auto Packet = Message(TEXT("poses"));
                Packet->SetStringField(TEXT("room"), Room.Id);
                Packet->SetNumberField(TEXT("frame"), ++LocalPoseFrame);
                TArray<TSharedPtr<FJsonValue>> Poses;
                for (auto& Member : Members)
                {
                    if (Member.LastPose <= 0) continue;
                    const auto Pose = MakeShared<FJsonObject>();
                    Pose->SetStringField(TEXT("id"), Member.Id);
                    WritePose(Pose, Member.Position, Member.Yaw, Member.Moving);
                    Pose->SetNumberField(TEXT("held_weapon"), int32(Member.HeldWeapon));
                    Pose->SetNumberField(TEXT("finish"), Member.Finish);
                    Pose->SetBoolField(TEXT("aim"), Member.Aiming);
                    Pose->SetNumberField(TEXT("pitch"), Member.Pitch);
                    Pose->SetBoolField(TEXT("down"), Member.Health <= 0);
                    if(Member.BleedUntil>0)
                    {
                        Pose->SetNumberField(TEXT("bleed"),FMath::Max(.01,Member.BleedUntil-Now));
                        Pose->SetNumberField(TEXT("revive"),Member.RescueProgress);
                        Pose->SetStringField(TEXT("rescuer"),Member.Rescuer);
                    }
                    if (Member.ShotsFired > 0)
                    {
                        Pose->SetNumberField(TEXT("shots"), Member.ShotsFired);
                        Pose->SetBoolField(TEXT("shot_long"), Member.LastShotLong);
                        Pose->SetArrayField(TEXT("shot_at"), Position(Member.ShotOrigin));
                    }
                    Poses.Add(MakeShared<FJsonValueObject>(Pose));
                }
                Packet->SetArrayField(TEXT("poses"), Poses);
                AdvanceWorldSnapshot(*Game);
                for (const auto& Member : Members)
                {
                    if (Member.Host || Member.Disconnected) continue;
                    Packet->SetStringField(TEXT("to"), Member.Id);
                    Packet->SetStringField(TEXT("member"), Member.Credential);
                    Packet->SetObjectField(TEXT("personal"), Personal(Member));
                    AppendWorld(Packet, Member);
                    Send(Packet, Member.Address);
                }
            }
            else if (Role == ERole::Guest)
            {
                const auto Packet = MemberPacket(TEXT("pose"));
                Packet->SetNumberField(TEXT("frame"), ++LocalPoseFrame);
                WritePose(Packet, At, Yaw, Moving);
                Packet->SetNumberField(TEXT("pitch"), FRotator::NormalizeAxis(Hero->GetControlRotation().Pitch));
                Packet->SetNumberField(TEXT("held_weapon"), int32(LocalCarriedWeapon(*Game)));
                Packet->SetBoolField(TEXT("aim"), Hero->Aiming);
                Packet->SetBoolField(TEXT("crouched"), Hero->bIsCrouched);
                if(const auto Intent=RescueInput(*Game);!Intent.IsEmpty())Packet->SetStringField(TEXT("aid"),Intent);
                Packet->SetNumberField(TEXT("respawn_ack"), ReceivedRespawn);
                WriteWorldAck(Packet);
                Packet->SetArrayField(TEXT("actions"), PendingActions);
                Send(Packet, Room.Address);
            }
        }
        for (const auto& Member : Members)
        {
            if (Member.Id == Self || Member.LastPose <= 0) continue;
            auto& Entry = Avatars.FindOrAdd(Member.Id);
            auto* Avatar = Entry.Get();
            const int32 Arrival = Game->HospitalStory->ArrivalStage;
            const bool Fallen=Member.BleedUntil>0;
            if (Arrival == 0 || Arrival == 1 || (Member.Health <= 0&&!Fallen) || Member.Disconnected || Now - Member.LastPose > 2.3)
            {
                if (Avatar) Avatar->SetActorHiddenInGame(true);
                continue;
            }
            FTransform Transform = Hero->MeshRest * FTransform(FRotator(0,Member.Yaw,0),Member.Position);
            if (Fallen)
            {
                const double Wave=Member.Moving?FMath::Sin(Now*9.):0.;
                FTransform Rest=Hero->MeshRest;
                Rest.SetLocation(FVector::ZeroVector);
                const FRotator Forward(0,Member.Yaw,0);
                const FVector Chest=Member.Position+FVector(0,0,-WardRescue104::CrawlHalfHeight+27+FMath::Abs(Wave)*2)-Forward.Vector()*35;
                Transform=Rest*FTransform(FRotator(-72,Member.Yaw,Wave*7),Chest);
            }
            if (!Avatar)
            {
                Avatar = Game->GetWorld()->SpawnActor<ASkeletalMeshActor>(Transform.GetLocation(), Transform.Rotator());
                if (!Avatar) continue;
                Entry = Avatar;
                Avatar->SetActorEnableCollision(false);
                auto* Mesh = Avatar->GetSkeletalMeshComponent();
                Mesh->SetSkeletalMesh(Hero->GetMesh()->GetSkeletalMeshAsset());
                Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Mesh->SetGenerateOverlapEvents(false);
                Mesh->SetLightingChannels(true, true, false);
                Mesh->SetCastShadow(true);
                Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
                Mesh->bEnableUpdateRateOptimizations = true;
                Avatar->SetActorScale3D(Transform.GetScale3D());
                if (Avatars.Num() == 1)
                    UE_LOG(LogTemp, Display, TEXT("ONLINE82 remote player visible"));
            }
            Avatar->SetActorHiddenInGame(false);
            const FVector Delta = Transform.GetLocation() - Avatar->GetActorLocation();
            Avatar->SetActorLocationAndRotation(Delta.SizeSquared() > FMath::Square(750.f) ? Transform.GetLocation() :
                Avatar->GetActorLocation() + Delta * .24, Transform.GetRotation());
            const bool Moving = Member.Moving && Now - Member.LastPose < .8;
            auto* Mesh = Avatar->GetSkeletalMeshComponent();
            static const TStrongObjectPtr<UAnimSequence> Idle(LoadObject<UAnimSequence>(nullptr,
                TEXT("/Game/Prototype/Character/A_Idle.A_Idle")));
            static const TStrongObjectPtr<UAnimSequence> Walk(LoadObject<UAnimSequence>(nullptr,
                TEXT("/Game/Prototype/Character/A_Walk.A_Walk")));
            static const TStrongObjectPtr<UAnimSequence> Low(LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Campaign/Character/A_Crouch.A_Crouch")));
            auto* Clip = Fallen && Low.IsValid() ? Low.Get() : Moving ? Walk.Get() : Idle.Get();
            if (Clip && Mesh->AnimationData.AnimToPlay != Clip) Mesh->PlayAnimation(Clip, true);
            Mesh->SetPlayRate(Fallen?(Moving?.65f:0.f):1.f);
            UpdateAvatarWeapon(*Game, Member, *Avatar);
        }
    }

    void TickRelay(double Now)
    {
        if (RelayAddress.IsEmpty()) return;
        if (RelayPoll.IsValid())
        {
            if (!RelayPoll.IsReady()) return;
            const FRelayReply Reply = RelayPoll.Get();
            RelayPoll = {};
            if (!PollWasClose && !RelayClosing)
            {
                const auto Payload = Reply.Status == 200 ? ReadWire(Reply.Wire) : Json();
                const uint32 Peer = Payload ? ReadPeer(Payload, TEXT("id")) : 0;
                const FString Key = String(Payload, TEXT("key"));
                const bool Connected = Peer && Key.Len() == 48 && (!RelayId || RelayId == Peer);
                if (DrivePages && Role == ERole::Host && Connected != RelayOnline && !InMap)
                    Feedback(Connected ? TEXT("Комната доступна через интернет и локальную сеть.") :
                        TEXT("Связь с интернет-каталогом прервана. Локальная сеть доступна."));
                RelayOnline = Connected;
                if (Connected)
                {
                    RelayId = Peer;
                    RelayKey = Key;
                    if (Reconnecting())
                    {
                        const uint32 LiveHost = ReadPeer(Payload, TEXT("route"));
                        if (LiveHost) PendingHost = RelayEndpoint(LiveHost);
                    }
                    if (Role == ERole::Idle && RelayBrowse)
                    {
                        TSet<uint32> Visible;
                        for (const auto& Entry : Array(Payload, TEXT("listings")))
                        {
                            const auto Row = Entry->AsObject();
                            if (!Row) continue;
                            const auto Host = ReadPeer(Row, TEXT("host"));
                            if (!Host) continue;
                            Visible.Add(Host);
                            Row->SetStringField(TEXT("type"), TEXT("room"));
                            Row->SetStringField(TEXT("request"), DiscoveryId);
                            Receive(Row, RelayEndpoint(Host), Now, Reply.Ping);
                        }
                        if (Available.RemoveAll([&](const FRoom& Entry) {
                            const uint32 Host = RelayPeer(Entry.Address);
                            return Host && !Visible.Contains(Host);
                        })) ++Revision;
                    }
                    for (const auto& Entry : Array(Payload, TEXT("inbox")))
                    {
                        const auto Row = Entry->AsObject();
                        const auto From = ReadPeer(Row, TEXT("from"));
                        if (From && From != Peer)
                        {
                            const auto Packet = ReadWire(String(Row, TEXT("wire")));
                            if (Packet && Number(Packet, TEXT("echo")) == Protocol)
                                Receive(Packet, RelayEndpoint(From), Now);
                        }
                        if (RelayClosing) break;
                    }
                }
                else if (Reply.Status == 401)
                {
                    RelayId = 0;
                    RelayKey.Empty();
                    RelayMessages.Reset();
                    if (Role == ERole::Guest && Room.Committed && RelayPeer(Target))
                        StartReconnect(Now);
                    else if ((Role == ERole::Guest || Role == ERole::Joining) && RelayPeer(Target))
                        Disconnected(TEXT("Интернет-соединение с комнатой истекло. Подключись заново."));
                    else if (Role == ERole::Host && !InMap &&
                        Members.RemoveAll([](const FMember& Member) { return RelayPeer(Member.Address) != 0; }))
                    {
                        PruneAvatars();
                        ++Revision;
                    }
                }
                NextPoll = Now + (Connected ? (InMap ? .14 : .22) : 1.0);
            }
        }
        if (!RelayClosing && (!RelayActive || Now < NextPoll)) return;
        PollWasClose = RelayClosing;
        RelayClosing = false;
        if (PollWasClose && !RelayId)
        {
            RelayMessages.Reset();
            return;
        }
        const auto Request = MakeShared<FJsonObject>();
        Request->SetNumberField(TEXT("id"), RelayId);
        Request->SetStringField(TEXT("key"), RelayKey);
        Request->SetBoolField(TEXT("browse"), RelayBrowse && Role == ERole::Idle && !PollWasClose);
        Request->SetBoolField(TEXT("close"), PollWasClose);
        if (Role == ERole::Host && !PollWasClose)
        {
            const auto Listing = MakeShared<FJsonObject>();
            Room.Players = Members.Num();
            Room.Write(Listing);
            Request->SetObjectField(TEXT("listing"), Listing);
            if (InMap)
            {
                Request->SetStringField(TEXT("room_route"), Room.Id);
                Request->SetStringField(TEXT("route_key"), RoomRoute);
            }
        }
        if (Reconnecting() && RelayPeer(Target))
            Request->SetStringField(TEXT("resolve_room"), Room.Id);
        TArray<TSharedPtr<FJsonValue>> Messages;
        for (const auto& Message : RelayMessages)
        {
            auto Row = MakeShared<FJsonObject>();
            Row->SetNumberField(TEXT("to"), Message.To);
            Row->SetStringField(TEXT("wire"), Message.Wire);
            Messages.Add(MakeShared<FJsonValueObject>(Row));
        }
        RelayMessages.Reset();
        Request->SetArrayField(TEXT("messages"), Messages);
        if (PollWasClose)
        {
            RelayId = 0;
            RelayKey.Empty();
            RelayOnline = false;
        }
        FString Wire;
        if (WriteWire(Request, Wire))
            RelayPoll = Async(EAsyncExecution::ThreadPool, [Url = RelayAddress, Wire = MoveTemp(Wire)] { return PollRelay(Url, Wire); });
    }

    void Show(const FString& Page)
    {
        if (auto* Game = Owner.Get(); Game && DrivePages) Game->Show(Page);
    }

    void CloseSocket()
    {
        if (Socket != INVALID_SOCKET) closesocket(Socket);
        Socket = INVALID_SOCKET;
    }

    void Send(const Json& Packet, const sockaddr_in& Destination) const
    {
        if (Socket == INVALID_SOCKET || FPlatformTime::Seconds() < TestMutedUntil) return;
        FString Wire;
        if (!WriteWire(Packet, Wire)) return;
        const FTCHARToUTF8 Bytes(*Wire);
        if (Bytes.Length() >= PacketLimit)
        {
            UE_LOG(LogTemp, Warning, TEXT("ONLINE84 oversize %s: %d"), *String(Packet, TEXT("type")), Bytes.Length());
            return;
        }
        if (const uint32 Peer = RelayPeer(Destination))
        {
            const FString Type = String(Packet, TEXT("type"));
            if (Type == TEXT("state") || Type == TEXT("pose") || Type == TEXT("poses") ||
                Type == TEXT("presence") || Type == TEXT("join"))
                RelayMessages.RemoveAll([&](const FRelayMessage& Pending) { return Pending.To == Peer && Pending.Type == Type; });
            if (RelayMessages.Num() == 32) RelayMessages.RemoveAt(0);
            RelayMessages.Add({Peer, Type, MoveTemp(Wire)});
            return;
        }
        sendto(Socket, Bytes.Get(), Bytes.Length(), 0, reinterpret_cast<const sockaddr*>(&Destination), sizeof(Destination));
    }

    void Connect(const sockaddr_in& Endpoint, const FString& Expected, const FString& Secret)
    {
        const FString Name = Clean(PlayerName, 24);
        if (Name.IsEmpty()) { Feedback(TEXT("Введи имя игрока.")); return; }
        if (Socket == INVALID_SOCKET && !Open(false)) { Feedback(TEXT("Не удалось открыть подключение.")); return; }
        Role = ERole::Joining;
        LastHost = 0;
        ReconnectUntil = NextResume = 0;
        RelayActive |= RelayPeer(Endpoint) != 0;
        Target = Endpoint;
        ExpectedRoom = Expected;
        JoinSecret = Secret;
        JoinSalt.Empty();
        ConnectionName = Name;
        ConnectionPassword = Clean(JoinPassword, 32);
        Self = Token();
        JoinDeadline = FPlatformTime::Seconds() + (RelayPeer(Endpoint) ? 12.0 : 6.0);
        Feedback(TEXT("Подключение к комнате…"));
        SendJoin();
    }

    void SendJoin()
    {
        const auto Packet = Message(TEXT("join"));
        Packet->SetStringField(TEXT("player"), Self);
        Packet->SetStringField(TEXT("name"), ConnectionName);
        Packet->SetStringField(TEXT("room"), ExpectedRoom);
        Packet->SetNumberField(TEXT("coop_build"), CoopBuild);
        if (DrivePages || !ProfileId.IsEmpty()) Packet->SetStringField(TEXT("profile"), LocalProfile());
        if (!JoinSecret.IsEmpty()) Packet->SetStringField(TEXT("invite"), JoinSecret);
        if (!JoinSalt.IsEmpty()) Packet->SetStringField(TEXT("proof"), Proof(ConnectionPassword, JoinSalt, Self));
        Send(Packet, Target);
        NextSend = FPlatformTime::Seconds() + 0.65;
    }

    Json MemberPacket(const TCHAR* Type) const
    {
        const auto Packet = Message(Type);
        Packet->SetStringField(TEXT("room"), Room.Id);
        Packet->SetStringField(TEXT("player"), Self);
        Packet->SetStringField(TEXT("member"), Credential);
        return Packet;
    }

    void SendPresence()
    {
        const auto Packet = MemberPacket(TEXT("presence"));
        Packet->SetBoolField(TEXT("ready"), RequestedReady);
        Packet->SetNumberField(TEXT("sequence"), ReadySequence);
        if (PendingCharacter != INDEX_NONE) Packet->SetNumberField(TEXT("character"), PendingCharacter);
        Send(Packet, Room.Address);
    }

    void StartReconnect(double Now)
    {
        if (ReconnectUntil > 0) return;
        ReconnectUntil = Now + 70;
        NextResume = 0;
        PendingHost = Target;
        if (auto* Game = Owner.Get(); Game && InMap && Game->Hero())
            Game->Notify(TEXT("Связь потеряна · возвращаемся в комнату…"), 5);
    }

    void SendResume(double Now)
    {
        const auto Packet = MemberPacket(TEXT("join"));
        Packet->SetNumberField(TEXT("coop_build"), CoopBuild);
        Send(Packet, PendingHost);
        NextResume = Now + .65;
    }

    void BroadcastState()
    {
        Room.Players = Members.Num();
        const auto Packet = Message(TEXT("state"));
        Room.Write(Packet);
        Packet->SetNumberField(TEXT("revision"), Revision);
        TArray<TSharedPtr<FJsonValue>> Rows;
        for (const auto& Member : Members)
        {
            auto Row = MakeShared<FJsonObject>();
            Row->SetStringField(TEXT("id"), Member.Id);
            Row->SetStringField(TEXT("name"), Member.Name);
            Row->SetBoolField(TEXT("ready"), Member.Ready);
            Row->SetBoolField(TEXT("host"), Member.Host);
            Row->SetBoolField(TEXT("offline"), Member.Disconnected);
            Row->SetNumberField(TEXT("character"), Member.Character);
            Row->SetNumberField(TEXT("presence_ack"), Member.ReadySequence);
            Rows.Add(MakeShared<FJsonValueObject>(Row));
        }
        Packet->SetArrayField(TEXT("roster"), Rows);
        for (const auto& Member : Members)
        {
            if (Member.Host) continue;
            Packet->SetStringField(TEXT("to"), Member.Id);
            Packet->SetStringField(TEXT("member"), Member.Credential);
            Send(Packet, Member.Address);
        }
    }

    void Reject(const FString& Client, const sockaddr_in& Source, const FString& Reason)
    {
        const auto Reply = Message(TEXT("reject"));
        Reply->SetStringField(TEXT("to"), Client);
        Reply->SetStringField(TEXT("reason"), Reason);
        Send(Reply, Source);
    }

    void ReceiveHost(Json Packet, const sockaddr_in& Source, double Now, const FString& Type)
    {
        if (Type == TEXT("discover"))
        {
            const FString Request = String(Packet, TEXT("request"));
            if (Room.Access == EAccess::Invite || Members.Num() >= Room.MaxPlayers || Room.Committed || !IsToken(Request)) return;
            Room.Players = Members.Num();
            const auto Reply = Message(TEXT("room"));
            Room.Write(Reply);
            Reply->SetStringField(TEXT("request"), Request);
            Send(Reply, Source);
            return;
        }
        const FString Client = String(Packet, TEXT("player"));
        if (!IsToken(Client)) return;
        const FString Session = String(Packet, TEXT("room"));
        const int32 Existing = Members.IndexOfByPredicate([&](const FMember& Entry) { return !Entry.Host && Entry.Id == Client; });
        if (Type == TEXT("join"))
        {
            if (!Session.IsEmpty() && Session != Room.Id) return;
            const FString Invite = String(Packet, TEXT("invite"));
            if ((Session.IsEmpty() || !Invite.IsEmpty()) && Invite != Code)
            { Reject(Client, Source, TEXT("Этот код приглашения больше не действует.")); return; }
            if (Number(Packet, TEXT("coop_build")) != CoopBuild)
            { Reject(Client, Source, TEXT("У всех участников должна быть бета b104.")); return; }
            if (Existing != INDEX_NONE)
            {
                auto& Former = Members[Existing];
                const FString Ticket = String(Packet, TEXT("member"));
                if (Room.Committed && !Ticket.IsEmpty() && Ticket == Former.Credential && Session == Room.Id)
                {
                    Former.Address = Source;
                    Former.LastSeen = Now;
                    Former.Disconnected = false;
                    ++Revision;
                    BroadcastState();
                }
                else if (!Room.Committed && SameEndpoint(Former.Address, Source))
                { Former.LastSeen = Now; BroadcastState(); }
                return;
            }
            const FString Name = Clean(String(Packet, TEXT("name")), 24);
            if (Name.IsEmpty()) return;
            const bool Invited = Invite == Code;
            if (Room.Access == EAccess::Invite && !Invited) { Reject(Client, Source, TEXT("Нужно приглашение хозяина.")); return; }
            if (Members.Num() >= Room.MaxPlayers) { Reject(Client, Source, TEXT("Комната заполнена.")); return; }
            if (Room.Committed) { Reject(Client, Source, TEXT("Игра в этой комнате уже началась.")); return; }
            if (Room.Access == EAccess::Password)
            {
                const FString Answer = String(Packet, TEXT("proof"));
                if (Answer.IsEmpty())
                {
                    auto Challenge = Message(TEXT("challenge"));
                    Challenge->SetStringField(TEXT("to"), Client);
                    Challenge->SetStringField(TEXT("room"), Room.Id);
                    Challenge->SetStringField(TEXT("salt"), Salt);
                    Send(Challenge, Source);
                    return;
                }
                if (Answer != Proof(ActivePassword, Salt, Client)) { Reject(Client, Source, TEXT("Неверный пароль комнаты.")); return; }
            }
            Members.Add({Client, Name, Source, Now, 0, false, false, Token()});
            auto& Added = Members.Last();
            for (int32 Index = 1; Index < WardOnlineRoster92::Count; ++Index)
            {
                if (CharacterOccupied(Index)) continue;
                Added.Character = Index;
                break;
            }
            const FString Profile = String(Packet, TEXT("profile"));
            if (IsToken(Profile) && !Members.ContainsByPredicate([&](const FMember& Known) { return Known.Profile == Profile; }))
                Added.Profile = Profile;
            ++Revision;
            BroadcastState();
            return;
        }
        if (Existing == INDEX_NONE || Session != Room.Id) return;
        auto& Member = Members[Existing];
        if (!SameEndpoint(Member.Address, Source) || String(Packet, TEXT("member")) != Member.Credential) return;
        if (Type == TEXT("leave"))
        {
            Members.RemoveAt(Existing);
            if (!InMap) Room.Committed = false;
            PruneAvatars();
            ++Revision;
            BroadcastState();
        }
        else if (Type == TEXT("pose") && InMap)
        {
            ReadWorldAck(Member, Packet);
            const double Frame = Number(Packet, TEXT("frame"));
            if (!FMath::IsFinite(Frame) || Frame <= Member.PoseFrame || Frame > MAX_int32) return;
            const double RespawnAck = Number(Packet, TEXT("respawn_ack"));
            if (RespawnAck != Member.RespawnSerial || (Member.Health<=0&&!Fallen(Member))) return;
            const auto& Coordinates = Array(Packet, TEXT("at"));
            if (Coordinates.Num() == 3 && Coordinates[2]->Type == EJson::Number &&
                Coordinates[2]->AsNumber() < -2500 && Coordinates[2]->AsNumber() > -150000)
            {
                if(auto* Game=Owner.Get())ExpireDown(*Game,Member,Now);
                return;
            }
            const auto Previous = Member.Position;
            const auto Last = Member.LastPose;
            if (!ReadPose(Packet, Member, Now)) return;
            if (auto* Game = Owner.Get(); Game && Game->BookWorld &&
                ValidPartyPosition(*Game, Member.Position) && (!Fallen(Member)||CrawlPositionValid(*Game,Member,Previous,Last,Now)) &&
                ((Game->HospitalStory->ArrivalStage >= 0 && Game->HospitalStory->ArrivalStage <= 1) ||
                    Last <= 0 || FVector::Dist(Member.Position, Previous) < 350 + 1050 * FMath::Min(3., Now - Last)))
            {
                const double Pitch = Number(Packet, TEXT("pitch"));
                Member.Pitch = FMath::IsFinite(Pitch) ? FMath::Clamp(float(Pitch), -89.f, 89.f) : 0.f;
                const auto Held = ReadCarriedWeapon(Packet);
                Member.HeldWeapon = (Member.Health>0&&((Held == ECarriedWeapon::Pistol && Member.Pistol) ||
                    (Held == ECarriedWeapon::Shotgun && Member.Shotgun))) ? Held : ECarriedWeapon::None;
                Member.Aiming = Member.HeldWeapon != ECarriedWeapon::None && Boolean(Packet, TEXT("aim"));
                Member.PoseFrame = static_cast<int32>(Frame);
                Member.ConfirmedRespawn = Member.RespawnSerial;
                Member.LastSeen = Now;
                Member.RescueIntent.Reset();
                if(Member.Health>0)
                {
                    const auto Aid=String(Packet,TEXT("aid"));
                    if(Aid.Len()>0&&Aid.Len()<96){Member.RescueIntent=Aid;Member.RescueLease=Now;}
                }
                const double Elapsed=Now-Last;
                const float Travel=FVector::Dist2D(Member.Position,Previous);
                if (Last>0 && Elapsed>.01 && Elapsed<.4 && Member.Moving && Travel>.5f && Travel<100 &&
                    FMath::Abs(Member.Position.Z-Previous.Z)<50 && Game->HospitalStory->HomeStage52<0 && !Game->HospitalStory->ArrivalCinematic())
                {
                    const bool Crouched=Fallen(Member)||Boolean(Packet,TEXT("crouched"));
                    const bool Sprint=Travel/Elapsed>380;
                    const float Stride=Crouched?40.f:Sprint?130.f:80.f;
                    Member.StepTravel+=Travel;
                    if(Member.StepTravel>=Stride)
                    {
                        Member.StepTravel=FMath::Fmod(Member.StepTravel,Stride);
                        WardHorror99::Footstep(*Game,Member.Position,Crouched?90:Sprint?1350:360);
                    }
                }
                else if(Elapsed>=.4||Travel>=100) Member.StepTravel=0;
                ProcessActions(*Game, Member, Packet, Now);
            }
            else { Member.Position = Previous; Member.LastPose = Last; }
        }
        else if (Type == TEXT("presence"))
        {
            Member.LastSeen = Now;
            if (Member.Disconnected) { Member.Disconnected = false; ++Revision; }
            const double Sequence = Number(Packet, TEXT("sequence"));
            if (Sequence > Member.ReadySequence && Sequence <= MAX_int32 && FMath::FloorToDouble(Sequence) == Sequence)
            {
                Member.ReadySequence = static_cast<int32>(Sequence);
                if (!Room.Committed)
                {
                    double Requested = -1;
                    if (Packet->TryGetNumberField(TEXT("character"), Requested))
                    {
                        if (Requested > 0 && Requested < WardOnlineRoster92::Count && FMath::FloorToDouble(Requested) == Requested)
                        {
                            const int32 Character = static_cast<int32>(Requested);
                            if (!Members.ContainsByPredicate([&](const FMember& Other) {
                                return Other.Id != Member.Id && Other.Character == Character;
                            }))
                            {
                                Member.Character = Character;
                                Member.Ready = false;
                            }
                        }
                    }
                    else Member.Ready = Boolean(Packet, TEXT("ready"));
                }
                ++Revision;
                BroadcastState();
            }
        }
    }

    void Disconnected(const FString& Reason)
    {
        const bool WasPlaying = InMap;
        Close();
        if (WasPlaying) Show(TEXT("title"));
        Show(BrowsePage);
        Browse();
        Feedback(Reason);
    }

    void Receive(Json Packet, const sockaddr_in& Source, double Now, int32 RelayPing = 0)
    {
        const FString Type = String(Packet, TEXT("type"));
        if (Role == ERole::Host) { ReceiveHost(Packet, Source, Now, Type); return; }
        if (Role == ERole::Idle && Type == TEXT("room") && String(Packet, TEXT("request")) == DiscoveryId)
        {
            FRoom Listing;
            if (!Listing.Read(Packet, Source) || Listing.Access == EAccess::Invite || Listing.Committed || Listing.Players >= Listing.MaxPlayers) return;
            Listing.Ping = RelayPing ? RelayPing : FMath::Max(1, FMath::RoundToInt((Now - DiscoveryTime) * 1000.0));
            auto* Before = Available.FindByPredicate([&](const FRoom& Entry) { return Entry.Id == Listing.Id; });
            if (!Before) { Available.Add(Listing); ++Revision; return; }
            if (!RelayPeer(Before->Address) && RelayPeer(Source)) return;
            if (Before->Players != Listing.Players || Before->Chapter != Listing.Chapter ||
                FMath::Abs(Before->Ping - Listing.Ping) >= 12) ++Revision;
            *Before = Listing;
            return;
        }
        if ((Role != ERole::Joining && Role != ERole::Guest) ||
            (!SameEndpoint(Source, Target) && !(Reconnecting() && Type == TEXT("state") && SameEndpoint(Source, PendingHost))))
            return;
        const FString To = String(Packet, TEXT("to"));
        if (Type == TEXT("reject") && Role == ERole::Joining && To == Self)
        {
            const FString Reason = Clean(String(Packet, TEXT("reason")), 120);
            UE_LOG(LogTemp, Display, TEXT("Online room join rejected: %s"), *Reason);
            Disconnected(Reason);
            return;
        }
        if (Type == TEXT("challenge") && Role == ERole::Joining && To == Self)
        {
            const FString Session = String(Packet, TEXT("room"));
            const FString Challenge = String(Packet, TEXT("salt"));
            if (!IsToken(Session) || !IsToken(Challenge) || (!ExpectedRoom.IsEmpty() && Session != ExpectedRoom)) return;
            if (ConnectionPassword.IsEmpty()) { Disconnected(TEXT("Для этой комнаты нужен пароль. Введи его и повтори вход.")); return; }
            ExpectedRoom = Session;
            JoinSalt = Challenge;
            SendJoin();
            return;
        }
        if (Type == TEXT("closed") && Role == ERole::Guest && String(Packet, TEXT("room")) == Room.Id &&
            String(Packet, TEXT("member")) == Credential)
        { Disconnected(TEXT("Хозяин закрыл комнату.")); return; }
        if (Type == TEXT("poses") && (InMap || !DrivePages) && Role == ERole::Guest && To == Self &&
            String(Packet, TEXT("room")) == Room.Id && String(Packet, TEXT("member")) == Credential)
        {
            const double Frame = Number(Packet, TEXT("frame"));
            if (!FMath::IsFinite(Frame) || Frame <= LastReceivedPose || Frame > MAX_int32) return;
            LastReceivedPose = static_cast<int32>(Frame);
            LastHost = Now;
            ReceiveWorldPiece(Packet);
            if (!DrivePages) { ProbeReceive(Packet); return; }
            if (auto* Game = Owner.Get()) ApplyWorld(*Game, Packet);
            for (const auto& Value : Array(Packet, TEXT("poses")))
            {
                const auto Pose = Value->AsObject();
                const auto Id = String(Pose, TEXT("id"));
                if (auto* Member = Members.FindByPredicate([&](const FMember& Entry) { return Entry.Id == Id && Id != Self; }))
                {
                    ReadPose(Pose, *Member, Now);
                    Member->HeldWeapon = ReadCarriedWeapon(Pose);
                    Member->Finish = int32(FMath::Clamp(Number(Pose, TEXT("finish")), 0., 9.));
                    Member->Aiming = Boolean(Pose, TEXT("aim"));
                    Member->Pitch = float(FMath::Clamp(Number(Pose, TEXT("pitch")), -89., 89.));
                    Member->Health = Boolean(Pose, TEXT("down")) ? 0 : 100;
                    const float Remaining=FMath::Clamp(float(Number(Pose,TEXT("bleed"))),0.f,WardRescue104::BleedSeconds);
                    Member->BleedUntil=Remaining>0?Now+Remaining:0;
                    Member->RescueProgress=FMath::Clamp(float(Number(Pose,TEXT("revive"))),0.f,1.f);
                    Member->Rescuer=String(Pose,TEXT("rescuer"));
                    const int32 Heard = HeardShots.FindRef(Id);
                    const int32 Reported = static_cast<int32>(Number(Pose, TEXT("shots")));
                    if (Reported > Heard && Array(Pose, TEXT("shot_at")).Num() == 3)
                    {
                        if (auto* Game = Owner.Get())
                            PlayRemoteShot(*Game, Vector(Array(Pose, TEXT("shot_at"))), Boolean(Pose, TEXT("shot_long")));
                        HeardShots.Add(Id, Reported);
                    }
                }
            }
            return;
        }
        if (Type != TEXT("state") || To != Self) return;
        if (Number(Packet, TEXT("coop_build")) != CoopBuild)
        {
            Disconnected(TEXT("У всех участников должна быть бета b104."));
            return;
        }
        FRoom Updated;
        if (!Updated.Read(Packet, Source) || (!ExpectedRoom.IsEmpty() && Updated.Id != ExpectedRoom)) return;
        const FString MemberKey = String(Packet, TEXT("member"));
        if (!IsToken(MemberKey) || (Role == ERole::Guest && MemberKey != Credential)) return;
        const int32 ReceivedRevision = int32(Number(Packet, TEXT("revision"), -1));
        if (ReceivedRevision < 0 || (Role == ERole::Guest && ReceivedRevision < HostRevision)) return;
        const auto& Rows = Array(Packet, TEXT("roster"));
        if (Rows.Num() != Updated.Players) return;
        TArray<FMember> UpdatedMembers;
        TSet<int32> Assigned;
        bool Included = false;
        bool Changed = Updated.Committed != Room.Committed || Rows.Num() != Members.Num();
        for (const auto& Value : Rows)
        {
            const auto Row = Value->AsObject();
            if (!Row) return;
            FMember Entry;
            Entry.Id = String(Row, TEXT("id"));
            Entry.Name = Clean(String(Row, TEXT("name")), 24);
            Entry.Ready = Boolean(Row, TEXT("ready"));
            Entry.Host = Boolean(Row, TEXT("host"));
            Entry.Disconnected = Boolean(Row, TEXT("offline"));
            if (!IsToken(Entry.Id) || Entry.Name.IsEmpty()) return;
            const double Character = Number(Row, TEXT("character"), -1);
            const double Ack = Number(Row, TEXT("presence_ack"), -1);
            if (Character < 0 || Character >= WardOnlineRoster92::Count || FMath::FloorToDouble(Character) != Character ||
                Ack < 0 || Ack > MAX_int32 || FMath::FloorToDouble(Ack) != Ack) return;
            Entry.Character = static_cast<int32>(Character);
            Entry.ReadySequence = static_cast<int32>(Ack);
            if (Assigned.Contains(Entry.Character) || (Entry.Character == 0) != Entry.Host) return;
            Assigned.Add(Entry.Character);
            Included |= Entry.Id == Self;
            const auto* Before = Members.FindByPredicate([&](const FMember& Previous) { return Previous.Id == Entry.Id; });
            Changed |= !Before || Before->Ready != Entry.Ready || Before->Character != Entry.Character ||
                Before->Disconnected != Entry.Disconnected;
            if (Before)
            {
                Entry.Position = Before->Position;
                Entry.Yaw = Before->Yaw;
                Entry.LastPose = Before->LastPose;
                Entry.Moving = Before->Moving;
                Entry.Health = Before->Health;
                Entry.BleedUntil=Before->BleedUntil;
                Entry.RescueProgress=Before->RescueProgress;
                Entry.Rescuer=Before->Rescuer;
                Entry.HeldWeapon = Before->HeldWeapon;
                Entry.Finish = Before->Finish;
                Entry.Aiming = Before->Aiming;
                Entry.Pitch = Before->Pitch;
            }
            UpdatedMembers.Add(MoveTemp(Entry));
        }
        if (!Included) return;
        if (Reconnecting())
        {
            Target = Source;
            ReconnectUntil = NextResume = 0;
            PendingHost = {};
            if (auto* Game = Owner.Get(); Game && InMap) Game->Notify(TEXT("Связь восстановлена."), 3);
        }
        LastHost = Now;
        HostRevision = ReceivedRevision;
        Credential = MemberKey;
        ExpectedRoom = Updated.Id;
        Room = MoveTemp(Updated);
        Members = MoveTemp(UpdatedMembers);
        const auto* Me = Members.FindByPredicate([this](const FMember& Entry) { return Entry.Id == Self; });
        if (Me && Me->ReadySequence >= ReadySequence)
        {
            RequestedReady = Me->Ready;
            if (PendingCharacter != INDEX_NONE)
            {
                Feedback(Me->Character == PendingCharacter ? TEXT("Персонаж выбран.") :
                    TEXT("Персонажа уже заняли или игра началась."));
                PendingCharacter = INDEX_NONE;
                Changed = true;
            }
        }
        if (Changed) ++Revision;
        if (Role == ERole::Joining)
        {
            Role = ERole::Guest;
            JoinPassword.Empty();
            ConnectionPassword.Empty();
            Feedback(TEXT("Ты в комнате. Отметь готовность, когда будешь готов."));
            Show(LobbyPage);
            SendPresence();
        }
        if (Room.Committed && !InMap) PrepareMap();
        PruneAvatars();
    }

    int32 HostRevision = 0;
};

TMap<TWeakObjectPtr<AEWardCampaignMode>, TSharedPtr<FOnline>> Sessions;

TSharedRef<FOnline> Session(AEWardCampaignMode& Game)
{
    auto& Entry = Sessions.FindOrAdd(&Game);
    if (!Entry)
    {
        Entry = MakeShared<FOnline>(Game);
        const TWeakPtr<FOnline> Weak(Entry);
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak](float) {
            if (auto Lobby = Weak.Pin(); Lobby && Lobby->Owner.IsValid())
            {
                Lobby->Tick();
                return true;
            }
            return false;
        }), 0.035f);
    }
    return Entry.ToSharedRef();
}

const FLinearColor White(.89f, .88f, .81f);
const FLinearColor Mist(.57f, .68f, .66f);
const FLinearColor Quiet(.44f, .53f, .50f);
const FLinearColor Accent(.85f, .45f, .39f);
const FLinearColor ReadyTint(.64f, .79f, .72f);
const FSlateColorBrush Glass(FLinearColor(.005f, .009f, .009f, .35f));
const FSlateColorBrush Edge(FLinearColor(.11f, .15f, .14f, .28f));
const FSlateColorBrush RuleBrush(FLinearColor(.32f, .37f, .34f, .42f));
const FSlateColorBrush RedRule(FLinearColor(.53f, .045f, .052f, .72f));

FSlateFontInfo Font(int32 Size, bool Serif = false)
{
    return FSlateFontInfo(FPaths::ProjectContentDir() / (Serif ?
        TEXT("CampaignData/Fonts/Literata.ttf") : TEXT("CampaignData/Fonts/NotoSans.ttf")), Size);
}

TSharedRef<STextBlock> Text(const FString& Value, int32 Size = 18, FLinearColor Color = White, bool Serif = false)
{
    return SNew(STextBlock).Text(FText::FromString(Value)).Font(Font(Size, Serif))
        .ColorAndOpacity(Color).Justification(ETextJustify::Left).AutoWrapText(true).LineHeightPercentage(1.25f);
}

TSharedRef<SWidget> Rule(bool Red = false)
{
    return SNew(SBox).HeightOverride(1)
        [SNew(SBorder).BorderImage(Red ? &RedRule : &RuleBrush).Padding(0)];
}

TSharedRef<SWidget> Framed(TSharedRef<SWidget> Content, FMargin Padding)
{
    return SNew(SBorder).BorderImage(&Edge).Padding(1)
        [SNew(SBorder).BorderImage(&Glass).Padding(Padding)[Content]];
}

template<typename Callback>
TSharedRef<SWidget> Act(const TSharedRef<FOnline>& Lobby, const FString& Label, Callback CallbackFn,
    bool Primary = false, TAttribute<bool> Enabled = TAttribute<bool>(true))
{
    return WardTitleMenu::Action(Lobby->Owner, FOnClicked::CreateLambda([CallbackFn] {
        CallbackFn();
        return FReply::Handled();
    }), Text(Label, Primary ? 22 : 18, White, true), Enabled);
}

template<typename Callback>
TSharedRef<SWidget> Entry(const FString& Current, const FString& Hint, Callback Update, int32 Limit, bool Private = false)
{
    return SNew(SBox).HeightOverride(47)
        [SNew(SEditableTextBox).Font(Font(17)).IsPassword(Private)
            .Text(FText::FromString(Current)).HintText(FText::FromString(Hint))
            .OnTextChanged_Lambda([Update, Limit](const FText& Value) { Update(Value.ToString().Left(Limit)); })];
}

template<typename Callback>
TSharedRef<SWidget> Choice(const TArray<FString>& Texts, int32 Current, Callback Changed)
{
    using FOption = TSharedPtr<FString>;
    auto Options = MakeShared<TArray<FOption>>();
    for (const auto& Name : Texts) Options->Add(MakeShared<FString>(Name));
    auto Selection = MakeShared<FOption>((*Options)[FMath::Clamp(Current, 0, Options->Num() - 1)]);
    return SNew(SBox).HeightOverride(47)
        [SNew(SComboBox<FOption>).OptionsSource(&Options.Get()).InitiallySelectedItem(*Selection)
            .OnGenerateWidget_Lambda([](FOption Option) -> TSharedRef<SWidget> { return Text(*Option, 17); })
            .OnSelectionChanged_Lambda([Options, Selection, Changed](FOption Option, ESelectInfo::Type) {
                const int32 Index = Options->IndexOfByKey(Option);
                if (Index >= 0) { *Selection = Option; Changed(Index); }
            })
            [SNew(STextBlock).Font(Font(17)).ColorAndOpacity(White).Text_Lambda([Selection] {
                return FText::FromString(**Selection);
            })]];
}

void Field(SVerticalBox& Column, const FString& Label, TSharedRef<SWidget> Control)
{
    Column.AddSlot().AutoHeight().Padding(0, 0, 0, 5)[Text(Label, 15, Mist)];
    Column.AddSlot().AutoHeight().Padding(0, 0, 0, 17)[Control];
}

class SOnlinePanel final : public SCompoundWidget
{
public:
    static inline TWeakPtr<SOnlinePanel> AssessmentPanel;
    WardOnlineRoster92::FStage* InspectionForAssessment() const { return Stage.Get(); }

    SLATE_BEGIN_ARGS(SOnlinePanel) {}
        SLATE_ARGUMENT(TSharedPtr<FOnline>, Lobby)
        SLATE_ARGUMENT(FString, Page)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args)
    {
        Lobby = Args._Lobby;
        Page = Args._Page;
        const auto SessionRef = Lobby.ToSharedRef();
        const bool Hub = Page == HomePage;
        const bool WaitingRoom = Page == LobbyPage;
        const FString Title = Page == CreatePage ? TEXT("Создать сервер") : Page == BrowsePage ?
            TEXT("Присоединиться") : WaitingRoom ? TEXT("Комната ожидания") : TEXT("Онлайн");
        const FString Back = Page == HomePage ? TEXT("‹   Главное меню") :
            Page == LobbyPage ? (Lobby->Role == ERole::Host ? TEXT("Закрыть комнату") : TEXT("Покинуть комнату")) : TEXT("‹   Онлайн");
        auto BackAction = Act(SessionRef, Back, [SessionRef] {
            if (auto* Game = SessionRef->Owner.Get()) Game->Command(TEXT("online_back"));
        });

        auto Content = SNew(SVerticalBox);
        auto Mark = SNew(SVerticalBox);
        Mark->AddSlot().AutoHeight().HAlign(HAlign_Left)
            [SNew(SBox).WidthOverride(Hub ? 552 : WaitingRoom ? 242 : 330).HeightOverride(Hub ? 186 : WaitingRoom ? 77 : 112)
                [WardTitleMenu::Logo(Lobby->Owner)]];
        const FString Subtitle = Lobby->Owner.IsValid() ?
            Lobby->Owner->Tr(TEXT("ОТДЕЛЕНИЕ 04 · ПОСЛЕДНЯЯ СМЕНА")) : TEXT("ОТДЕЛЕНИЕ 04 · ПОСЛЕДНЯЯ СМЕНА");
        Mark->AddSlot().AutoHeight().Padding(4, 4, 0, 0)[Text(Subtitle, 12, Mist, true)];
        Content->AddSlot().AutoHeight()[Mark];
        auto Heading = SNew(SHorizontalBox);
        Heading->AddSlot().FillWidth(1).VAlign(VAlign_Center)[Text(Title, Hub ? 49 : 42, White, true)];
        if (!Hub) Heading->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(28, 0, 0, 0)[BackAction];
        Content->AddSlot().AutoHeight().Padding(0, Hub ? 28 : 13, 0, Hub ? 13 : 10)[Heading];
        if (Hub)
        {
            Content->AddSlot().AutoHeight()[Home()];
            Content->AddSlot().AutoHeight().Padding(0, 11, 0, 0)[SNew(SBox).WidthOverride(430).HAlign(HAlign_Left)[BackAction]];
        }
        else
        {
            Content->AddSlot().AutoHeight().Padding(0, 0, 0, 16)[Rule(true)];
            TSharedRef<SWidget> Body = Page == CreatePage ? Create() : Page == BrowsePage ? Browser() : LobbyBody();
            Content->AddSlot().FillHeight(1)[Body];
        }
        if (!WaitingRoom)
            Content->AddSlot().AutoHeight().Padding(0, 16, 0, 0)
                [SNew(STextBlock).Font(Font(15)).ColorAndOpacity(Mist).AutoWrapText(true)
                    .Text_Lambda([SessionRef] { return FText::FromString(SessionRef->Status); })];

        const float Width = Hub ? 595 : WaitingRoom ? 1545 : Page == BrowsePage ? 1260 : 1115;
        auto Frame = SNew(SOverlay);
        Frame->AddSlot()[WardTitleMenu::Shade(Lobby->Owner)];
        Frame->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(88, Hub ? 118 : 55, 0, 0)
            [SNew(SBox).WidthOverride(Width).HeightOverride(Hub ? 735 : 956)[Content]];
        ChildSlot[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
            [SNew(SBox).WidthOverride(1920).HeightOverride(1080)[Frame]]];
        RefreshRows();
        if (WaitingRoom && FParse::Param(FCommandLine::Get(), TEXT("WardOnlineCheck80")))
            AssessmentPanel = SharedThis(this);
    }

    void Tick(const FGeometry& Geometry, double Now, float Delta) override
    {
        SCompoundWidget::Tick(Geometry, Now, Delta);
        if (Lobby->Revision != LastRevision || Lobby->SelectedRoom != LastSelection) RefreshRows();
        if (Stage) Stage->Tick(Delta);
    }

private:
    TSharedPtr<FOnline> Lobby;
    FString Page;
    FString LastSelection;
    int32 LastRevision = INDEX_NONE;
    TSharedPtr<SVerticalBox> Rows;
    TUniquePtr<WardOnlineRoster92::FStage> Stage;

    TSharedRef<SWidget> Home()
    {
        const auto S = Lobby.ToSharedRef();
        auto Body = SNew(SVerticalBox);
        Body->AddSlot().AutoHeight().Padding(4, 0, 0, 21)[Text(S->InternetConfigured() ?
            TEXT("Интернет и локальная сеть · 2–10 игроков") : TEXT("Локальные комнаты · 2–10 игроков"), 16, Mist)];
        auto Option = [S](const FString& Label, const FString& Action) {
            return SNew(SBox).WidthOverride(430).HeightOverride(57)
                [Act(S, Label, [S, Action] {
                    if (auto* Game = S->Owner.Get()) Game->Command(Action);
                }, true)];
        };
        Body->AddSlot().AutoHeight().HAlign(HAlign_Left).Padding(0, 0, 0, 5)
            [Option(TEXT("Создать сервер"), TEXT("online_create"))];
        Body->AddSlot().AutoHeight().HAlign(HAlign_Left)
            [Option(TEXT("Присоединиться"), TEXT("online_browse"))];
        return Body;
    }

    TSharedRef<SWidget> Create()
    {
        const auto S = Lobby.ToSharedRef();
        auto Left = SNew(SVerticalBox);
        auto Right = SNew(SVerticalBox);
        Field(*Left, TEXT("Название комнаты"), Entry(S->RoomName, TEXT("До 40 символов"), [S](FString Value) { S->RoomName = MoveTemp(Value); }, 40));
        Field(*Left, TEXT("Имя игрока"), Entry(S->PlayerName, TEXT("Как тебя увидит команда"), [S](FString Value) { S->PlayerName = MoveTemp(Value); }, 24));
        TArray<FString> Counts;
        for (int32 Number = 2; Number <= 10; ++Number) Counts.Add(FString::FromInt(Number));
        Field(*Left, TEXT("Лимит игроков · включая хозяина"), Choice(Counts, S->MaxPlayers - 2, [S](int32 Index) { S->MaxPlayers = Index + 2; }));
        Field(*Left, TEXT("Доступ"), Choice(AccessTypes, static_cast<int32>(S->Access), [S](int32 Index) { S->Access = static_cast<EAccess>(Index); }));
        auto PasswordFields = SNew(SVerticalBox);
        Field(*PasswordFields, TEXT("Пароль комнаты"), Entry(S->Password, TEXT("От 4 до 32 символов"), [S](FString Value) { S->Password = Clean(Value, 32); }, 32, true));
        Left->AddSlot().AutoHeight()[SNew(SBox).Visibility_Lambda([S] {
            return S->Access == EAccess::Password ? EVisibility::Visible : EVisibility::Collapsed;
        })[PasswordFields]];

        Field(*Right, TEXT("Карта"), Framed(Text(MapName, 19), FMargin(12, 10)));
        Field(*Right, TEXT("Сложность"), Choice(Difficulties, S->Difficulty, [S](int32 Index) { S->Difficulty = Index; }));
        TArray<FString> Slots;
        for (int32 Slot = 0; Slot < 3; ++Slot) Slots.Add(S->SlotLabel(Slot));
        Field(*Right, TEXT("Сетевой слот"), Choice(Slots, S->SaveSlot(), [S](int32 Index) { S->SelectSaveSlot(Index); }));
        Field(*Right, TEXT("Режим"), Choice({TEXT("Новая кампания"), TEXT("Продолжить со слота")}, S->ContinueSaved ? 1 : 0,
            [S](int32 Index) { S->ContinueSaved = Index == 1; S->ChapterTwoTest = false; }));
        Right->AddSlot().AutoHeight()[Act(S, TEXT("Создать сервер"), [S] { S->Host(); }, true)];

        auto Columns = SNew(SHorizontalBox);
        Columns->AddSlot().FillWidth(1).Padding(0, 0, 28, 0)[Framed(Left, FMargin(23))];
        Columns->AddSlot().FillWidth(1)[Framed(Right, FMargin(23))];
        return SNew(SScrollBox)+SScrollBox::Slot()[Columns];
    }

    TSharedRef<SWidget> Browser()
    {
        const auto S = Lobby.ToSharedRef();
        auto Body = SNew(SVerticalBox);
        auto Lead = SNew(SHorizontalBox);
        Lead->AddSlot().FillWidth(1).VAlign(VAlign_Center)
            [Text(S->InternetConfigured() ? TEXT("Доступные комнаты · интернет и локальная сеть") :
                TEXT("Доступные комнаты · локальная сеть"), 19, Mist)];
        Lead->AddSlot().AutoWidth()[Act(S, TEXT("Обновить"), [S] { S->Browse(); }, false,
            TAttribute<bool>::CreateLambda([S] { return S->Role == ERole::Idle; }))];
        Body->AddSlot().AutoHeight().Padding(0, 0, 0, 12)[Lead];
        Body->AddSlot().AutoHeight().Padding(40, 0, 30, 0)
            [Columns(TEXT("Комната"), TEXT("Карта"), TEXT("Глава"), TEXT("Сложность"), TEXT("Пинг"), TEXT("Места"), true)];
        Body->AddSlot().FillHeight(1)[Framed(SNew(SScrollBox)+SScrollBox::Slot()[SAssignNew(Rows, SVerticalBox)], FMargin(12))];
        auto Inputs = SNew(SHorizontalBox);
        auto JoinBlock = SNew(SVerticalBox);
        Field(*JoinBlock, TEXT("Имя игрока"), Entry(S->PlayerName, TEXT("Как тебя увидит команда"), [S](FString Value) { S->PlayerName = MoveTemp(Value); }, 24));
        Field(*JoinBlock, TEXT("Пароль · для закрытых комнат"), Entry(S->JoinPassword, TEXT("Пароль комнаты"), [S](FString Value) { S->JoinPassword = MoveTemp(Value); }, 32, true));
        auto CodeBlock = SNew(SVerticalBox);
        Field(*CodeBlock, TEXT("Код приглашения"), Entry(S->Invitation, TEXT("Вставь код от хозяина"), [S](FString Value) { S->Invitation = MoveTemp(Value); }, 100));
        CodeBlock->AddSlot().AutoHeight()[Act(S, TEXT("Войти по приглашению"), [S] { S->JoinCode(); }, false,
            TAttribute<bool>::CreateLambda([S] { return S->Role == ERole::Idle; }))];
        Inputs->AddSlot().FillWidth(1).Padding(0, 0, 22, 0)[JoinBlock];
        Inputs->AddSlot().FillWidth(1)[CodeBlock];
        Body->AddSlot().AutoHeight().Padding(0, 17, 0, 0)[Inputs];
        Body->AddSlot().AutoHeight()[Act(S, TEXT("Присоединиться к выбранной комнате"), [S] { S->JoinFound(); }, true,
            TAttribute<bool>::CreateLambda([S] {
                return S->Role == ERole::Idle && S->Available.ContainsByPredicate([S](const FRoom& Room) { return Room.Id == S->SelectedRoom; });
            }))];
        return Body;
    }

    TSharedRef<SWidget> Columns(const FString& Name, const FString& Map, const FString& Chapter,
        const FString& Difficulty, const FString& Ping, const FString& Capacity, bool Header) const
    {
        auto Line = SNew(SHorizontalBox);
        const auto Color = Header ? Mist : White;
        const int32 Size = Header ? 15 : 17;
        auto Add = [&Line, Size, Color](const FString& Value, float Weight) {
            Line->AddSlot().FillWidth(Weight).Padding(4, 5).VAlign(VAlign_Center)[Text(Value, Size, Color)];
        };
        Add(Name, 2.15f); Add(Map, 1.55f); Add(Chapter, .55f);
        Add(Difficulty, 1); Add(Ping, .59f); Add(Capacity, .57f);
        return Line;
    }

    TSharedRef<SWidget> LobbyBody()
    {
        const auto S = Lobby.ToSharedRef();
        auto Showcase = SNew(SVerticalBox);
        auto Scene = SNew(SOverlay);
        if (auto* Game = S->Owner.Get())
        {
            Stage = MakeUnique<WardOnlineRoster92::FStage>();
            const auto* Hero = Game->Hero();
            if (Stage->Create(*Game->GetWorld(), Hero ? Hero->BekaBody.Get() : nullptr,
                Hero ? Hero->BekaOutfit.Get() : nullptr))
                Scene->AddSlot()[SNew(WardOnlineRoster92::SOrbitImage).PreviewStage(Stage.Get())];
            else Stage.Reset();
        }
        Scene->AddSlot().VAlign(VAlign_Top).HAlign(HAlign_Left).Padding(18, 13)
            [SNew(STextBlock).Font(Font(15, true)).ColorAndOpacity(White).Visibility(EVisibility::HitTestInvisible)
                .Text_Lambda([S] {
                    return FText::FromString(S->PreviewCharacter == INDEX_NONE ? TEXT("КПП 04  /  НОЧНАЯ СМЕНА") :
                        TEXT("ОСМОТР 360°"));
                })];
        Scene->AddSlot().VAlign(VAlign_Top).HAlign(HAlign_Right).Padding(12, 9)
            [SNew(SBox).Visibility_Lambda([S] {
                return S->PreviewCharacter != INDEX_NONE ? EVisibility::Visible : EVisibility::Collapsed;
            })[Act(S, TEXT("Вся команда"), [S] {
                S->PreviewCharacter = INDEX_NONE;
                ++S->Revision;
            })]];
        Scene->AddSlot().VAlign(VAlign_Bottom).HAlign(HAlign_Left).Padding(18, 10)
            [SNew(STextBlock).Font(Font(16)).ColorAndOpacity(White).Visibility(EVisibility::HitTestInvisible).Text_Lambda([S] {
                const bool Inspecting = S->PreviewCharacter != INDEX_NONE;
                const FString Label = Inspecting ? TEXT("Осмотр · ") : TEXT("Твой персонаж · ");
                const int32 Character = Inspecting ? S->PreviewCharacter : S->LocalCharacter();
                return FText::FromString(Character >= 0 ? Label + WardOnlineRoster92::Characters[Character].Name : FString());
            })];
        auto ViewControls = SNew(SHorizontalBox);
        ViewControls->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
            [SNew(STextBlock).Font(Font(13)).ColorAndOpacity(White)
                .Text(FText::FromString(TEXT("Перетаскивание · вращение   Колесо · масштаб")))];
        ViewControls->AddSlot().AutoWidth()
            [Act(S, TEXT("Сбросить вид"), [this] { if (Stage) Stage->ResetOrbit(); })];
        Scene->AddSlot().VAlign(VAlign_Bottom).HAlign(HAlign_Right).Padding(13, 8)
            [SNew(SBox).Visibility_Lambda([S] {
                return S->PreviewCharacter != INDEX_NONE ? EVisibility::Visible : EVisibility::Collapsed;
            })[ViewControls]];
        Showcase->AddSlot().AutoHeight()[Framed(SNew(SBox).HeightOverride(486)[Scene], FMargin(0))];
        auto ChooseHeading = SNew(SHorizontalBox);
        ChooseHeading->AddSlot().FillWidth(1).VAlign(VAlign_Center)[Text(TEXT("Выбор персонажа"), 24, White, true)];
        if (S->Role == ERole::Guest)
            ChooseHeading->AddSlot().AutoWidth()[Act(S, TEXT("Выбрать"), [S] {
                S->ChooseCharacter(S->PreviewCharacter);
                S->PreviewCharacter = INDEX_NONE;
            }, false, TAttribute<bool>::CreateLambda([S] { return S->CanChoose(S->PreviewCharacter); }))];
        Showcase->AddSlot().AutoHeight().Padding(0, 15, 0, 6)[ChooseHeading];
        for (int32 Start = 0; Start < WardOnlineRoster92::Count; Start += 5)
        {
            auto Cards = SNew(SHorizontalBox);
            for (int32 Index = Start; Index < Start + 5; ++Index)
            {
                auto Label = SNew(STextBlock).Font(Font(16, true)).ColorAndOpacity_Lambda([S, Index] {
                    return FSlateColor(Index == S->LocalCharacter() || Index == S->PreviewCharacter ? Accent :
                        S->CharacterOccupied(Index) ? Mist : White);
                }).Text_Lambda([S, Index] {
                    FString Name = WardOnlineRoster92::Characters[Index].Name;
                    if (Index == S->LocalCharacter()) Name = TEXT("✓  ") + Name;
                    else if (S->CharacterOccupied(Index)) Name = TEXT("•  ") + Name;
                    else if (Index == S->PreviewCharacter) Name = TEXT("›  ") + Name;
                    return FText::FromString(Name);
                });
                Cards->AddSlot().FillWidth(1).Padding(0, 0, 6, 0)
                    [WardTitleMenu::Action(S->Owner, FOnClicked::CreateLambda([S, Index] {
                        S->PreviewCharacter = S->PreviewCharacter == Index ? INDEX_NONE : Index;
                        ++S->Revision;
                        return FReply::Handled();
                    }), Label, TAttribute<bool>(true))];
            }
            Showcase->AddSlot().AutoHeight().Padding(0, 3)[Cards];
        }
        if (S->Role == ERole::Host)
            Showcase->AddSlot().AutoHeight().Padding(0, 8, 0, 0)
                [Text(TEXT("Ты играешь за Беку. Спутники выбирают своих персонажей."), 14, Mist)];
        else
            Showcase->AddSlot().AutoHeight().Padding(0, 8, 0, 0)
                [Text(TEXT("Точка рядом с именем означает, что персонажа уже выбрали."), 14, Mist)];

        auto Left = SNew(SVerticalBox);
        auto Right = SNew(SVerticalBox);
        Left->AddSlot().AutoHeight()[Text(S->Room.Name, 34, White, true)];
        Left->AddSlot().AutoHeight().Padding(0, 6, 0, 13)[SNew(STextBlock).Font(Font(17)).ColorAndOpacity(Mist)
            .Text_Lambda([S] { return FText::FromString(FString::Printf(TEXT("Участники   %d / %d"), S->Members.Num(), S->Room.MaxPlayers)); })];
        Left->AddSlot().AutoHeight()[Rule()];
        Left->AddSlot().AutoHeight().Padding(0, 8)[SNew(SBox).HeightOverride(160)
            [SNew(SScrollBox) + SScrollBox::Slot()[SAssignNew(Rows, SVerticalBox)]]];
        auto ReadyLabel = SNew(STextBlock).Font(Font(18, true)).ColorAndOpacity(White).Text_Lambda([S] {
                return FText::FromString(S->IsReady() ? TEXT("Снять готовность") : TEXT("Я готов"));
            });
        Left->AddSlot().AutoHeight()[WardTitleMenu::Action(S->Owner, FOnClicked::CreateLambda([S] {
            S->ToggleReady();
            return FReply::Handled();
        }), ReadyLabel, TAttribute<bool>::CreateLambda([S] { return !S->Room.Committed && !S->CharacterPending(); }))];

        auto Detail = [&Right](const FString& Key, const FString& Value) {
            auto Pair = SNew(SHorizontalBox);
            Pair->AddSlot().FillWidth(1)[Text(Key, 16, Mist)];
            Pair->AddSlot().FillWidth(1)[Text(Value, 17)];
            Right->AddSlot().AutoHeight().Padding(0, 8, 0, 0)[Pair];
        };
        Detail(FString::Printf(TEXT("Глава %02d"), S->Room.Chapter), Difficulties[S->Room.Difficulty]);
        if (S->Role == ERole::Host)
        {
            Detail(TEXT("Режим"), S->ContinueSaved ? TEXT("Продолжить кампанию") : TEXT("Новая кампания"));
            Right->AddSlot().AutoHeight().Padding(0, 10, 0, 5)[Act(S, TEXT("Копировать приглашение"), [S] {
                FPlatformApplicationMisc::ClipboardCopy(*S->InvitationCode());
                S->Feedback(S->InternetConnected() ? TEXT("Интернет-приглашение скопировано.") :
                    TEXT("Приглашение для локальной сети скопировано."));
            })];
            auto StartLabel = SNew(STextBlock).Font(Font(22, true)).ColorAndOpacity(White).Text_Lambda([S] {
                    return FText::FromString(S->Room.Committed ? TEXT("Открыть приём игроков") : TEXT("Начать"));
                });
            Right->AddSlot().AutoHeight()[WardTitleMenu::Action(S->Owner, FOnClicked::CreateLambda([S] {
                    if (S->Room.Committed) S->Reopen(); else S->Start();
                    return FReply::Handled();
                }), StartLabel, TAttribute<bool>::CreateLambda([S] { return S->CanStart() || S->Room.Committed; }))];
        }
        else Right->AddSlot().AutoHeight().Padding(0, 24, 0, 0)[Text(TEXT("Запуск подтверждает хозяин комнаты."), 16, Mist)];

        auto ColumnsBox = SNew(SHorizontalBox);
        Left->AddSlot().AutoHeight().Padding(0, 9, 0, 0)[Right];
        Left->AddSlot().AutoHeight().Padding(0, 10, 0, 0)
            [SNew(STextBlock).Font(Font(13)).ColorAndOpacity(Mist).AutoWrapText(true)
                .Text_Lambda([S] { return FText::FromString(S->Status); })];
        ColumnsBox->AddSlot().FillWidth(2.3f).Padding(0, 0, 20, 0)[Showcase];
        ColumnsBox->AddSlot().FillWidth(1)[Framed(Left, FMargin(17))];
        return ColumnsBox;
    }

    void ArrangeStage()
    {
        if (!Stage) return;
        TArray<int32> Selection;
        for (const auto& Member : Lobby->Members)
            if (!Member.Disconnected) Selection.Add(Member.Character);
        Stage->Arrange(Selection, Lobby->PreviewCharacter);
    }

    void RefreshRows()
    {
        LastRevision = Lobby->Revision;
        LastSelection = Lobby->SelectedRoom;
        if (!Rows) return;
        Rows->ClearChildren();
        if (Page == BrowsePage)
        {
            if (Lobby->Available.IsEmpty())
            {
                Rows->AddSlot().AutoHeight().Padding(14, 14)
                    [Text(TEXT("Свободных комнат пока нет. Создай свою или попроси у друга приглашение."), 18, Mist)];
                return;
            }
            const auto S = Lobby.ToSharedRef();
            for (const auto& Room : Lobby->Available)
            {
                const FString Name = Room.Name + (Room.Access == EAccess::Password ? TEXT("   ·   пароль") : TEXT(""));
                const FString Delay = RelayPeer(Room.Address) ? FString::Printf(TEXT("%d мс*"), Room.Ping) :
                    FString::Printf(TEXT("%d мс"), Room.Ping);
                auto Row = WardTitleMenu::Action(S->Owner, FOnClicked::CreateLambda([S, Id = Room.Id] {
                    S->SelectedRoom = Id;
                    return FReply::Handled();
                }), Columns((Room.Id == S->SelectedRoom ? TEXT("›  ") : TEXT("")) + Name, MapName,
                    FString::FromInt(Room.Chapter), Difficulties[Room.Difficulty], Delay,
                    FString::Printf(TEXT("%d/%d"), Room.Players, Room.MaxPlayers), false));
                Rows->AddSlot().AutoHeight().Padding(0, 2)[Row];
            }
        }
        else if (Page == LobbyPage)
        {
            ArrangeStage();
            for (const auto& Member : Lobby->Members)
            {
                auto Line = SNew(SHorizontalBox);
                Line->AddSlot().FillWidth(1).VAlign(VAlign_Center)
                    [Text(Member.Name + TEXT("  ·  ") + WardOnlineRoster92::Characters[Member.Character].Name, 16)];
                const TCHAR* Status = Member.Disconnected ? TEXT("Нет связи") : Member.Ready ? TEXT("Готов") : TEXT("Не готов");
                Line->AddSlot().AutoWidth().VAlign(VAlign_Center)
                    [Text(Status, 14, Member.Ready && !Member.Disconnected ? ReadyTint : Mist)];
                Rows->AddSlot().AutoHeight().Padding(0, 4)[Line];
                Rows->AddSlot().AutoHeight()[Rule()];
            }
        }
    }
};
}

bool IsPage(const FString& Page)
{
    return Page == HomePage || Page == CreatePage || Page == BrowsePage || Page == LobbyPage;
}

bool PlayingMap(AEWardCampaignMode& Game)
{
    const auto* Entry = Sessions.Find(&Game);
    return Entry && (*Entry)->InMap;
}

bool Reconnecting(AEWardCampaignMode& Game)
{
    const auto* Entry = Sessions.Find(&Game);
    return Entry && (*Entry)->Reconnecting();
}

bool SaveCheckpoint(AEWardCampaignMode& Game, bool Manual)
{
    const auto* Entry = Sessions.Find(&Game);
    return Entry && (*Entry)->SaveFromMenu(Manual);
}

bool Guest(AEWardCampaignMode& Game)
{
    const auto* Entry = Sessions.Find(&Game);
    return Entry && (*Entry)->InMap && (*Entry)->Role == ERole::Guest;
}

bool Interact(AEWardCampaignMode& Game)
{
    if (Reconnecting(Game)) return true;
    if (!Guest(Game) || !Game.BookWorld || !Game.HospitalStory) return false;
    if (!Game.Screen.IsEmpty() || Game.Hero()->Dead) return true;
    const auto Lobby = Session(Game);
    const auto At = Game.Hero()->GetActorLocation();
    const auto& Story = *Game.HospitalStory;
    if (Story.HomeStage52 >= 0) return Lobby->QueueAction(TEXT("home"));
    if (Story.ArrivalStage <= 1 && Story.ArrivalStage >= 0)
        return Story.ArrivalStage == 0 || Lobby->QueueAction(TEXT("car_exit"));
    if (Story.ArrivalStage == 2 && FVector::DistSquared(At,
        Vector(Array(Story.ArrivalData, TEXT("checkpoint"))) * 100) <= FMath::Square(210.))
        return Lobby->QueueAction(TEXT("checkpoint"));
    if (Game.BookWorld->InsideLift(At) || (FMath::Abs(At.X - 5000) < 250 && At.Y >= 1000 && At.Y < 1350))
    {
        const int32 Floor = Game.BookWorld->InsideLift(At) ? Game.BookWorld->LiftSelection :
            FMath::RoundToInt((At.Z - 98 + 960) / 480);
        return Lobby->QueueAction(TEXT("lift"), {}, Floor);
    }
    const int32 Index = WardGameplay76::Door(*Game.BookWorld);
    if (Game.BookWorld->Portals.IsValidIndex(Index))
        return Lobby->QueueAction(TEXT("door"), {}, Index, !Game.BookWorld->Portals[Index].Open);
    const auto Id = StoryTarget(*Game.HospitalStory);
    if (!Id.IsEmpty())
    {
        Lobby->QueuePickup(Id, !Game.Completed.Contains(Id) && String(Story.Definitions.FindRef(Id), TEXT("kind")) == TEXT("key"));
        return true;
    }
    const int32 Cart = WardContact98::AimedCart(*Game.BookWorld, *Game.Hero());
    if (Cart != INDEX_NONE)
    {
        const auto* PC = Cast<APlayerController>(Game.Hero()->GetController());
        const bool Pull = PC && PC->IsInputKeyDown(EKeys::LeftShift);
        if (Lobby->QueueAction(TEXT("cart"), {}, Cart, Pull)) WardContact98::PushCart(*Game.BookWorld, Cart, At, Pull, true);
    }
    return true;
}

bool PersonalAction(AEWardCampaignMode& Game, const TCHAR* Action)
{
    if (Reconnecting(Game)) return true;
    if (WardRescue104::Downed(Game)) return true;
    if (!Guest(Game)) return false;
    auto* Hero = Game.Hero();
    if (!Hero || Hero->Dead) return true;
    if (FCString::Strcmp(Action, TEXT("fire")) == 0)
    {
        const bool Long = Game.Shotgun;
        if (Game.Screen.IsEmpty() && Hero->Aiming && WardGameplay76::Armed(Game) &&
            (Long ? Game.LoadedShells : Hero->Rounds) > 0 && Game.ShotCooldown <= 0 && Hero->WeaponHasClearance())
        {
            Session(Game)->QueueAction(Long ? TEXT("fire_long") : Action);
            Game.ShotCooldown = Long ? .85f : .32f;
            Hero->WeaponKick = FMath::Min(7.f, Hero->WeaponKick + (Long ? 4.f : 1.8f));
            Hero->MuzzleRemaining = .055f;
            WardCombat63::Shot(Game);
        }
        return true;
    }
    if (FCString::Strcmp(Action, TEXT("reload")) != 0 || (Game.Screen.IsEmpty() && WardGameplay76::Armed(Game)))
        Session(Game)->QueueAction(FCString::Strcmp(Action, TEXT("reload")) == 0 && Game.Shotgun ? TEXT("reload_long") : Action);
    return true;
}

bool Rescue104::Downed(const AEWardCampaignMode& Game)
{
    const auto* Entry=Sessions.Find(const_cast<AEWardCampaignMode*>(&Game));
    return Entry&&(*Entry)->IsLocalDown();
}

bool Rescue104::DownHost(AEWardCampaignMode& Game,bool FellOut)
{
    const auto* Entry=Sessions.Find(&Game);
    return Entry&&(*Entry)->HurtDownHost(Game,FellOut);
}

void Rescue104::HostHit(AEWardCampaignMode& Game)
{
    if(const auto* Entry=Sessions.Find(&Game))(*Entry)->RegisterRescueHostHit();
}

bool Rescue104::Begin(AEWardCampaignMode& Game)
{
    const auto* Entry=Sessions.Find(&Game);
    return Entry&&(*Entry)->BeginRescue(Game);
}

void Rescue104::End(AEWardCampaignMode& Game)
{
    if(const auto* Entry=Sessions.Find(&Game))(*Entry)->EndRescue();
}

Rescue104::FPrompt Rescue104::Prompt(AEWardCampaignMode& Game)
{
    if(const auto* Entry=Sessions.Find(&Game))return (*Entry)->RescuePrompt(Game);
    return {};
}

bool SelectLift(AEWardCampaignMode& Game, int32 Direction)
{
    if (!Guest(Game)) return false;
    if (!Reconnecting(Game) && Game.BookWorld && Game.Screen.IsEmpty()) Game.BookWorld->SelectLift(Direction);
    return true;
}

FVector RespawnPoint(AEWardCampaignMode& Game)
{
    const auto* Entry = Sessions.Find(&Game);
    return Entry ? (*Entry)->LastCheckpointPosition() : Game.BookWorld->Start();
}

bool DoorwayOccupied(AEWardCampaignMode& Game, FVector Door, bool NS, float Width)
{
    const auto* Entry = Sessions.Find(&Game);
    return Entry && (*Entry)->Occupied(Door, NS, Width);
}

int32 PlayersOutsideEntrance(AEWardCampaignMode& Game)
{
    const auto* Entry = Sessions.Find(&Game);
    return Entry ? (*Entry)->EntranceWait(Game) : 0;
}

FVector MobPlayer(AEWardHospitalMob& Mob, bool Select)
{
    auto& Game = *Mob.Runtime->Mode;
    if (PlayingMap(Game))
        if (auto* Member = Session(Game)->Threat(Mob, Select)) return Member->Position;
    return Game.Hero()->GetActorLocation();
}

bool MobTargetsGuest(AEWardHospitalMob& Mob)
{
    auto& Game = *Mob.Runtime->Mode;
    return PlayingMap(Game) && Session(Game)->Threat(Mob, false) != nullptr;
}

bool DamageGuest(AEWardHospitalMob& Mob, float Damage)
{
    auto& Game = *Mob.Runtime->Mode;
    return PlayingMap(Game) && Session(Game)->HitRemote(Mob, Damage);
}

double PartyDistanceSquared(AEWardCampaignMode& Game, FVector At)
{
    const auto* Entry = Sessions.Find(&Game);
    return Entry && (*Entry)->InMap ? (*Entry)->DistanceToParty(Game, At) :
        FVector::DistSquared(Game.Hero()->GetActorLocation(), At);
}

float RecoveryScale(AEWardCampaignMode& Game)
{
    const auto* Entry = Sessions.Find(&Game);
    const int32 Players = Entry && (*Entry)->InMap ? (*Entry)->StartingTeamSize() : 0;
    return Players <= 2 ? 1.f : Players <= 5 ? .83f : .68f;
}

int32 SupplyAmount(AEWardCampaignMode& Game, const FString& Id, int32 Amount)
{
    const auto* Entry = Sessions.Find(&Game);
    if (!Entry || !(*Entry)->InMap || !Id.StartsWith(TEXT("SUPPLY_2F_"))) return Amount;
    const int32 Players = (*Entry)->StartingTeamSize();
    const int32 Bonus = Players <= 2 ? 0 : Players <= 5 ? 1 : 2;
    if (Id.EndsWith(TEXT("_MED"))) return Amount + Bonus;
    if (Id.EndsWith(TEXT("_AMMO"))) return Amount + 5 * Bonus;
    return Amount;
}

void RecordShot(AEWardCampaignMode& Game, FVector Origin)
{
    const auto* Entry = Sessions.Find(&Game);
    if (Entry) (*Entry)->LocalShot(Origin, Game.Shotgun);
}

bool ParallelPickup(AEWardCampaignMode& Game, const FString& Id)
{
    return PlayingMap(Game) && Game.HospitalStory && Game.HospitalStory->Chapter() == 2 &&
        (Id.StartsWith(TEXT("NOTE_02_")) || Id.StartsWith(TEXT("SUPPLY_2F_")));
}

bool Command(AEWardCampaignMode& Game, const FString& Action)
{
    if (PlayingMap(Game))
    {
        if (Guest(Game) && Action.StartsWith(TEXT("story35_"))) return true;
        if (Action == TEXT("page_title") || Action == TEXT("page_online"))
        {
            Session(Game)->Leave();
            if (Action == TEXT("page_title")) Game.Show(TEXT("title"));
            return true;
        }
        if (Action.StartsWith(TEXT("new_")) || Action.StartsWith(TEXT("load_")) || Action == TEXT("latest") ||
            Action == TEXT("page_game") || (Guest(Game) && (Action.StartsWith(TEXT("drop_")) ||
                Action.StartsWith(TEXT("deposit_")) || Action.StartsWith(TEXT("withdraw_"))))) return true;
    }
    if (Action == TEXT("online_exit_map") && PlayingMap(Game))
    {
        Session(Game)->Leave();
        return true;
    }
    if (Action == TEXT("page_online"))
    {
        if (Game.TitleScene && Game.TitleScene->Transitioning) return true;
        auto Lobby = Session(Game);
        Lobby->Feedback(Lobby->InternetConfigured() ? TEXT("Найди комнату или собери свою группу.") :
            TEXT("Локальная сеть доступна. Адрес интернет-каталога пока не задан."));
        Game.Show(HomePage);
        return true;
    }
    if (!IsPage(Game.Screen)) return false;
    if (Action == TEXT("back") || Action == TEXT("online_back"))
    {
        auto Lobby = Session(Game);
        if (Game.Screen == LobbyPage) Lobby->Leave();
        else if (Game.Screen == HomePage)
        {
            Lobby->Close();
            Lobby->Feedback({});
            Game.Show(TEXT("title"));
        }
        else
        {
            Lobby->Close();
            Lobby->Feedback({});
            Game.Show(HomePage);
        }
        return true;
    }
    if (Action == TEXT("online_create"))
    {
        auto Lobby = Session(Game);
        Lobby->Feedback(TEXT("Создай комнату и отправь приглашение друзьям."));
        Game.Show(CreatePage);
        return true;
    }
    if (Action == TEXT("online_browse"))
    {
        auto Lobby = Session(Game);
        Game.Show(BrowsePage);
        Lobby->Browse();
        return true;
    }
    return false;
}

void Shutdown(AEWardCampaignMode& Game)
{
    if (auto* Lobby = Sessions.Find(&Game)) (*Lobby)->Close();
    Sessions.Remove(&Game);
}

TSharedRef<SWidget> Build(AEWardCampaignMode& Game)
{
    Game.Interface->TitleActions.Reset();
    Game.Interface->TitleMotion.Reset();
    return SNew(SOnlinePanel).Lobby(Session(Game)).Page(Game.Screen);
}

#include "WardPartyCheck84.inl"
#include "WardRecoveryCheck85.inl"
#include "WardPrologueCheck86.inl"

void StartAssessment(AEWardCampaignMode& Game)
{
    FString RescueRole;
    if(FParse::Value(FCommandLine::Get(),TEXT("WardRescueCheck104="),RescueRole))
    {
        const bool Host=RescueRole==TEXT("host");
        const auto Lobby=Session(Game);
        const double Started=FPlatformTime::Seconds();
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
            [Lobby,Host,Started,Stage=0,Time=0.,Checks=TMap<FString,bool>(),DownAt=FVector::ZeroVector,Crawl=FVector::ZeroVector](float) mutable
        {
            auto* G=Lobby->Owner.Get();
            if(!G||!G->Interface||!G->HospitalStory||!G->Hero())return true;
            const double Now=FPlatformTime::Seconds();
            const auto Check=[&](const TCHAR* Name,bool Passed){Checks.Add(Name,Passed);UE_LOG(LogTemp,Display,TEXT("RESCUE104 %s %s"),Passed?TEXT("PASS"):TEXT("FAIL"),Name);};
            auto* Hero=G->Hero();
            if(Stage==0&&Now-Started>.6)
            {
                G->Command(TEXT("page_online"));
                if(Host)
                {
                    G->Command(TEXT("online_create"));Lobby->RoomName=TEXT("ECHO rescue test 104");Lobby->MaxPlayers=2;
                    Lobby->ChapterTwoTest=true;Lobby->Host();
                }
                else G->Command(TEXT("online_browse"));
                Stage=1;
            }
            if(Stage==1&&!Host&&Lobby->Role==ERole::Idle)
                if(const auto* Room=Lobby->Available.FindByPredicate([](const auto& Entry){return Entry.Name==TEXT("ECHO rescue test 104");}))
                {Lobby->SelectedRoom=Room->Id;Lobby->JoinFound();}
            if(Stage==1&&Lobby->Role==(Host?ERole::Host:ERole::Guest)){Lobby->ToggleReady();Stage=2;}
            if(Stage==2&&Host&&Lobby->CanStart())Lobby->Start();
            if(Lobby->InMap&&Stage>=2&&Stage<90)
            {
                auto& Story=*G->HospitalStory;Story.FreezeAI=true;
                if(Host)
                {
                    auto* Other=Lobby->TestingGuest();
                    if(Other)
                    {
                        if(Stage==2)
                        {
                            const auto* Mob=Story.Mobs.FindByPredicate([](const auto& Entry){return Entry->Species==TEXT("patients");});
                            if(Mob)
                            {
                                const auto& HostMember=Lobby->Members[0];
                                Lobby->TestingAimAtGuest(**Mob,*Other);DownAt=Other->Position;
                                Check(TEXT("host_can_down_guest_without_killing_or_respawning"),DamageGuest(**Mob,180.f)&&Other->Health==0&&Other->BleedUntil>Now&&Other->RespawnAt==0&&HostMember.Health>0);
                                Stage=3;Time=Now;
                            }
                        }
                        else if(Stage==3&&(FVector::Dist2D(Other->Position,DownAt)>31||Now-Time>6))
                        {
                            Check(TEXT("host_receives_crawl_displacement_from_downed_guest"),FVector::Dist2D(Other->Position,DownAt)>31&&Other->BleedUntil>Now);
                            const FVector Standing(Other->Position.X,Other->Position.Y,Other->Position.Z-WardRescue104::CrawlHalfHeight+96);
                            for(const FVector Axis:{FVector(1,0,0),FVector(-1,0,0),FVector(0,1,0),FVector(0,-1,0)})
                            {
                                const FVector Candidate=Standing+Axis*106;
                                FCollisionQueryParams Q(SCENE_QUERY_STAT(RescueTest104),false,Hero);
                                if(G->BookWorld->ValidPosition(Candidate)&&!G->GetWorld()->OverlapBlockingTestByChannel(Candidate,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(32,87),Q)&&
                                    Lobby->TestCanAid(*G,Candidate,Other->Position)){Hero->SetActorLocation(Candidate,false);break;}
                            }
                            Lobby->TestRescueHold(Other->Id);Stage=4;Time=Now;
                        }
                        else if(Stage==4&&(Other->RescueProgress>.21f||Now-Time>4))
                        {
                            Check(TEXT("held_rescue_progress_is_authoritative"),Other->RescueProgress>.21f&&Other->Health==0);
                            Lobby->EndRescue();Stage=5;Time=Now;
                        }
                        else if(Stage==5&&Now-Time>.52)
                        {
                            Check(TEXT("releasing_rescue_cancels_progress"),Other->RescueProgress==0&&Other->Health==0);
                            Lobby->TestRescueHold(Other->Id);Stage=6;Time=Now;
                        }
                        else if(Stage==6&&(Other->Health>0||Now-Time>6))
                        {
                            Check(TEXT("host_revives_guest_at_current_position"),FMath::IsNearlyEqual(Other->Health,40.f)&&Other->BleedUntil==0&&Other->RespawnAt==0);
                            Lobby->EndRescue();Stage=7;Time=Now;
                        }
                        else if(Stage==7&&Now-Time>1.05)
                        {
                            G->Invulnerable=0;G->DamageHero(200,nullptr);
                            Check(TEXT("host_uses_same_down_state_as_guest"),WardRescue104::Downed(*G)&&!Hero->Dead&&Hero->Health==0);
                            Stage=8;Time=Now;
                        }
                        else if(Stage==8&&(!WardRescue104::Downed(*G)||Now-Time>8))
                        {
                            Check(TEXT("guest_hold_request_revives_host_over_network"),!WardRescue104::Downed(*G)&&!Hero->Dead&&FMath::IsNearlyEqual(Hero->Health,40.f));
                            const auto* Mob=Story.Mobs.FindByPredicate([](const auto& Entry){return Entry->Species==TEXT("patients");});
                            if(Mob)
                            {
                                auto& Creature=**Mob;Creature.ResetMob();Creature.Active=true;Creature.ChangeState59(TEXT("patrol"));
                                const FVector At=Hero->GetActorLocation(), NoiseAt=At+FVector(100,0,0);
                                Creature.SetActorLocation(At-FVector(250,0,0));
                                G->BroadcastNoise(NoiseAt,2200);
                                Check(TEXT("normal_distraction_noise_diverts_monster_without_moving_players"),Creature.State59==TEXT("investigate")&&Creature.Target.Equals(NoiseAt,1)&&Hero->GetActorLocation().Equals(At,1));
                            }
                            Stage=90;Time=Now;
                        }
                    }
                }
                else
                {
                    if(Stage==2&&WardRescue104::Downed(*G))
                    {
                        Check(TEXT("guest_sees_downed_state_while_controls_remain_active"),!Hero->Dead&&Hero->Health==0&&G->Screen.IsEmpty());
                        DownAt=Hero->GetActorLocation();
                        const auto* Capsule=Hero->GetCapsuleComponent();
                        FCollisionQueryParams Q(SCENE_QUERY_STAT(CrawlTest104),false,Hero);
                        for(const FVector Axis:{FVector(1,0,0),FVector(-1,0,0),FVector(0,1,0),FVector(0,-1,0)})
                        {
                            FHitResult Hit;
                            if(!G->GetWorld()->SweepSingleByChannel(Hit,DownAt,DownAt+Axis*95,FQuat::Identity,ECC_Pawn,
                                FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius()-2,Capsule->GetScaledCapsuleHalfHeight()-3),Q))
                            {Crawl=Axis;break;}
                        }
                        Stage=3;Time=Now;
                    }
                    if(Stage==3)
                    {
                        if(FVector::Dist2D(Hero->GetActorLocation(),DownAt)<54&&Now-Time<3.1)Hero->AddMovementInput(Crawl,1.f);
                        if(!WardRescue104::Downed(*G)&&Hero->Health>0)
                        {
                            Check(TEXT("guest_sees_revival_without_teleport"),Hero->Health==40&&FVector::Dist2D(Hero->GetActorLocation(),DownAt)>28);
                            Stage=4;
                        }
                    }
                    if(Stage==4&&!Lobby->Members.IsEmpty()&&Lobby->Members[0].BleedUntil>Now)
                    {
                        Check(TEXT("guest_can_see_and_target_wounded_host"),Lobby->Members[0].Health==0&&FVector::Dist2D(Hero->GetActorLocation(),Lobby->Members[0].Position)<153);
                        Lobby->TestRescueHold(Lobby->Members[0].Id);Stage=5;Time=Now;
                    }
                    if(Stage==5&&(!Lobby->Members.IsEmpty()&&Lobby->Members[0].Health>0))
                    {Check(TEXT("guest_receives_host_standing_again"),Lobby->Members[0].BleedUntil==0);Lobby->EndRescue();Stage=90;Time=Now;}
                }
            }
            if((Stage==90&&Now-Time>1.)||Now-Started>175.)
            {
                const auto Report=MakeShared<FJsonObject>(), Rows=MakeShared<FJsonObject>();
                bool Passed=Stage==90&&Checks.Num()>=(Host?8:4);
                for(const auto& Entry:Checks){Passed&=Entry.Value;Rows->SetBoolField(Entry.Key,Entry.Value);}
                Report->SetBoolField(TEXT("success"),Passed);Report->SetObjectField(TEXT("checks"),Rows);
                Report->SetNumberField(TEXT("stage"),Stage);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/rescue104.json"),Report);
                FPlatformMisc::RequestExit(false);return false;
            }
            return true;
        }),.05f);
        return;
    }
    FString PrologueRole;
    if (FParse::Value(FCommandLine::Get(), TEXT("WardPrologueCheck86="), PrologueRole))
    {
        const auto Assessment = MakeShared<FPrologueCheck86>();
        Assessment->Game = &Game;
        Assessment->Host = PrologueRole == TEXT("host");
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Assessment](float) { return Assessment->Tick(); }), .065f);
        return;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("WardRecoveryCheck85")))
    {
        const auto Assessment = MakeShared<FRecoveryCheck85>();
        Assessment->Game = &Game;
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Assessment](float) { return Assessment->Tick(); }), .065f);
        return;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("WardPartyCheck84")))
    {
        auto Assessment = MakeShared<FPartyCheck84>();
        Assessment->Game = &Game;
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Assessment](float) { return Assessment->Tick(); }), .06f);
        return;
    }
    FString CoopCheck;
    if (FParse::Value(FCommandLine::Get(), TEXT("WardCoopCheck83="), CoopCheck))
    {
        const bool Hosting = CoopCheck == TEXT("host");
        const auto Lobby = Session(Game);
        const auto StartedAt = FPlatformTime::Seconds();
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
            [Lobby, Hosting, StartedAt, Stage = 0, FinishedAt = 0.0, MapAt = 0.0, Next = 0.0,
                PausedMenu = false, AmbushApplied = false, ArmedChoice = false, AnchorDoor = int32(INDEX_NONE),
                RacePlaced = false, TriedPickup = false, ShotSent = false, RaceCheck = false,
                HitGuest = false, HeardScene = false, ActiveSince = 0.0, NextPickup = 0.0](float) mutable
        {
            auto* Mode = Lobby->Owner.Get();
            if (!Mode || !Mode->Interface || !Mode->HospitalStory) return true;
            auto& Story = *Mode->HospitalStory;
            const auto Now = FPlatformTime::Seconds();
            if (Stage == 0 && Now - StartedAt > 1)
            {
                Mode->Command(TEXT("page_online"));
                if (Hosting)
                {
                    Mode->Command(TEXT("online_create"));
                    Lobby->RoomName = TEXT("ECHO coop check 83");
                    Lobby->ChapterTwoTest = true;
                    Lobby->RoomName += FParse::Param(FCommandLine::Get(), TEXT("EchoRelayOnly")) ? TEXT(" HTTPS") : TEXT(" LAN");
                    Lobby->PlayerName = TEXT("Бека");
                    Lobby->MaxPlayers = 2;
                    Lobby->Host();
                }
                else
                {
                    Mode->Command(TEXT("online_browse"));
                    Lobby->PlayerName = TEXT("Спутник");
                }
                Stage = 1;
            }
            if (Stage == 1 && !Hosting && Lobby->Role == ERole::Idle)
            {
                const bool Relay = FParse::Param(FCommandLine::Get(), TEXT("EchoRelayOnly"));
                const auto Name = FString(TEXT("ECHO coop check 83")) + (Relay ? TEXT(" HTTPS") : TEXT(" LAN"));
                if (const auto* Listing = Lobby->Available.FindByPredicate([&](const FRoom& Room) { return Room.Name == Name; }))
                {
                    Lobby->SelectedRoom = Listing->Id;
                    Lobby->JoinFound();
                }
            }
            if (Stage == 1 && Lobby->Role == (Hosting ? ERole::Host : ERole::Guest))
            {
                Lobby->ToggleReady();
                Stage = 2;
            }
            if (Stage == 2 && Hosting && Lobby->CanStart()) Lobby->Start();
            if (Lobby->InMap)
            {
                if (MapAt == 0) { MapAt = Now; Next = Now + 12; Stage = 3; }
                constexpr const TCHAR* RaceId = TEXT("SUPPLY_2F_0_MED");
                if (Hosting)
                {
                    if (auto* Other = Lobby->TestingGuest())
                    {
                        if (!RacePlaced)
                            if (auto* Prop = Story.Props.FindRef(RaceId))
                            {
                                Prop->SetActorLocation(Other->Position + FVector(0, 0, 40));
                                RacePlaced = true;
                            }
                        if (!TriedPickup && Other->Items.FindRef(TEXT("medkit")) == 1)
                        {
                            const int32 Before = Mode->Items.FindRef(TEXT("medkit"));
                            Story.UseId(RaceId);
                            RaceCheck = Mode->Items.FindRef(TEXT("medkit")) == Before;
                            TriedPickup = true;
                        }
                        if (!HitGuest && Now - MapAt > 4)
                        {
                            if (const auto* Mob = Story.Mobs.FindByPredicate([](const auto& M) {
                                return M && M->Active && !M->Dead && M->FirstChapter == 2;
                            }))
                            {
                                const auto HeroHealth = Mode->Hero()->Health;
                                Lobby->TestingAimAtGuest(**Mob, *Other);
                                HitGuest = DamageGuest(**Mob, 13.0f) && Other->Health < 100 &&
                                    Mode->Hero()->Health == HeroHealth;
                            }
                        }
                    }
                }
                else
                {
                    HeardScene |= !Story.DialogueKey62.IsEmpty();
                    if (Now - MapAt > 2.5 && Now >= NextPickup && !Mode->Completed.Contains(RaceId))
                    {
                        Lobby->QueueAction(TEXT("use"), RaceId);
                        if (!TriedPickup) Lobby->QueueAction(TEXT("use"), RaceId);
                        TriedPickup = true;
                        NextPickup = Now + 1.1;
                    }
                    if (!ShotSent && Now - MapAt > 4.0)
                    {
                        Lobby->QueueAction(Mode->Shotgun ? TEXT("fire_long") : TEXT("fire"));
                        ShotSent = true;
                    }
                }
                if (!Hosting && !PausedMenu && Now - MapAt > 6)
                {
                    Mode->Show(TEXT("inventory"));
                    PausedMenu = Mode->Screen == TEXT("inventory") && !Mode->GetWorld()->IsPaused();
                }
                if (!Hosting && PausedMenu && Now - MapAt > 22 && Mode->Screen == TEXT("inventory"))
                    Mode->Show(FString());
                if (Hosting && Now >= Next && Story.Chapter() == 2 && (TriedPickup || Now - MapAt > 24))
                {
                    Next = Now + .35;
                    if (!ArmedChoice)
                    {
                        const FVector Here = Mode->Hero()->GetActorLocation();
                        auto& Portals = Mode->BookWorld->Portals;
                        AnchorDoor = Portals.IndexOfByPredicate([&](const auto& Door) {
                            return Door.Type == TEXT("ordinary") && Door.Open && !Door.Locked && !Door.InsideOnly &&
                                FVector::DistSquared2D(Here, Door.At) > FMath::Square(320.) &&
                                FVector::DistSquared2D(Here, Door.At) < FMath::Square(2500.) &&
                                FMath::Abs(Here.Z - Door.At.Z - 95) < 120;
                        });
                        if (Portals.IsValidIndex(AnchorDoor))
                        {
                            const auto& P = Portals[AnchorDoor];
                            const auto At = P.At + ((P.Side == TEXT("north") || P.Side == TEXT("south")) ?
                                FVector(0, 180, 95) : FVector(180, 0, 95));
                            OperateDoor(*Mode->BookWorld, AnchorDoor, At, false);
                        }
                        ArmedChoice = true;
                    }
                    if (!AmbushApplied)
                    {
                        for (const auto& Mob : Story.Mobs)
                        {
                            if (!Mob || !Mob->Active || Mob->Dead || Mob->FirstChapter != 2) continue;
                            FDamageEvent Event;
                            Mob->TakeDamage(12.f, Event, Mode->Hero()->GetController(), Mode->Hero());
                            AmbushApplied = true;
                            break;
                        }
                    }
                    const TGuardValue<bool> TestStory(Story.NarrativeTesting35, true);
                    if (Story.ActiveScene35.IsEmpty())
                    {
                        const int32 Step = Mode->Values.FindRef(TEXT("hospital_step"));
                        constexpr const TCHAR* Scenes[] = {TEXT("SCENE_02_01"), TEXT("SCENE_02_02"),
                            TEXT("SCENE_02_03"), TEXT("SCENE_02_04")};
                        if (Step >= 0 && Step < UE_ARRAY_COUNT(Scenes))
                        {
                            const FString Id(Scenes[Step]);
                            FVector At;
                            if (Story.FindNarrativeStanding35(Id, At))
                                Mode->Hero()->SetActorLocation(At, false, nullptr, ETeleportType::TeleportPhysics);
                            Story.UseId(Id);
                            ActiveSince = Now;
                            UE_LOG(LogTemp, Display, TEXT("ONLINE83 host started %s; actual=%s"), *Id, *Story.ActiveScene35);
                        }
                    }
                    else
                    {
                        const auto& Choices = Array(Story.NarrativeScenes35.FindRef(Story.ActiveScene35), TEXT("choices"));
                        if (!Choices.IsEmpty() && Story.SelectedChoice35.IsEmpty() && WardStaging64::ChoiceReady(Story))
                            Mode->Command(TEXT("story35_choice_") + String(Choices[0]->AsObject(), TEXT("id")));
                        if (Now - ActiveSince > 1.25 && WardStaging64::CanFinish(Story))
                            Mode->Command(TEXT("story35_continue64"));
                    }
                }
                if (Story.Chapter() == 3)
                {
                    if (!Hosting && Mode->Screen == TEXT("inventory")) Mode->Show(FString());
                    if (!FinishedAt) { FinishedAt = Now; if (Hosting) Story.FreezeAI = true; }
                    if (Now - FinishedAt > 1 && Stage == 3)
                    {
                        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Prototype/coop83.png"), true, false);
                        Stage = 4;
                    }
                }
            }
            if ((FinishedAt && Now - FinishedAt > (Hosting ? 8 : 4)) || Now - StartedAt > 140)
            {
                const auto Report = MakeShared<FJsonObject>();
                const bool Complete = Stage == 4 && Story.Chapter() == 3;
                Report->SetBoolField(TEXT("success"), Complete && (Hosting || PausedMenu));
                Report->SetNumberField(TEXT("chapter"), Story.Chapter());
                Report->SetNumberField(TEXT("step"), Mode->Values.FindRef(TEXT("hospital_step")));
                Report->SetBoolField(TEXT("guest_inventory_kept_world_running"), PausedMenu);
                Report->SetBoolField(TEXT("host_hit_monster"), AmbushApplied);
                Report->SetBoolField(TEXT("host_unchanged_after_duplicate_pickup"), RaceCheck);
                Report->SetBoolField(TEXT("guest_had_personal_damage"), Hosting ? HitGuest : Mode->Hero()->Health < 100);
                Report->SetBoolField(TEXT("guest_received_dialogue"), HeardScene);
                Report->SetNumberField(TEXT("medkits"), Mode->Items.FindRef(TEXT("medkit")));
                Report->SetNumberField(TEXT("rounds"), Mode->Hero()->Rounds);
                Report->SetNumberField(TEXT("shells"), Mode->LoadedShells);
                Report->SetStringField(TEXT("status"), Lobby->Status);
                Report->SetObjectField(TEXT("state"), Lobby->CheckWorld(*Mode));
                Write(FPaths::ProjectSavedDir() / TEXT("Prototype/coop83.json"), Report);
                UE_LOG(LogTemp, Display, TEXT("ONLINE83 chapter transition %s (%s)"), Complete ? TEXT("PASS") : TEXT("FAIL"),
                    Hosting ? TEXT("host") : TEXT("guest"));
                FPlatformMisc::RequestExit(false);
                return false;
            }
            return true;
        }), .055f);
        return;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("WardOnlineCapacityCheck82")))
    {
        const auto Lobby = Session(Game);
        const auto Start = FPlatformTime::Seconds();
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
            [Lobby, Start, Guests = TArray<TSharedPtr<FOnline>>(), NextGuest = 0.0](float) mutable
        {
            auto* Mode = Lobby->Owner.Get();
            if (!Mode || !Mode->Interface) return true;
            const double Now = FPlatformTime::Seconds();
            if (Lobby->Role == ERole::Idle && Now - Start > 1)
            {
                Mode->Command(TEXT("page_online"));
                Mode->Command(TEXT("online_create"));
                Lobby->RoomName = TEXT("ECHO capacity check 82");
                Lobby->MaxPlayers = 10;
                Lobby->Host();
            }
            for (const auto& Guest : Guests) Guest->Tick();
            if (Lobby->InternetConnected() && Guests.Num() < 10 && Now >= NextGuest)
            {
                auto Guest = MakeShared<FOnline>(*Mode, false);
                Guest->PlayerName = FString::Printf(TEXT("Test %02d"), Guests.Num() + 2);
                Guest->Invitation = Lobby->InvitationCode();
                Guest->JoinCode();
                Guests.Add(Guest);
                NextGuest = Now + .35;
            }
            int32 Entered = 0;
            int32 Rejected = 0;
            for (const auto& Guest : Guests)
            {
                Entered += Guest->Role == ERole::Guest && Guest->Members.Num() == 10;
                Rejected += Guest->Role == ERole::Idle && Guest->Status.Contains(TEXT("Комната заполнена"));
            }
            const bool Passed = Guests.Num() == 10 && Lobby->Members.Num() == 10 && Entered == 9 && Rejected == 1;
            if (Passed || Now - Start >= 39)
            {
                const auto Result = MakeShared<FJsonObject>();
                Result->SetBoolField(TEXT("success"), Passed);
                Result->SetNumberField(TEXT("players_including_host"), Lobby->Members.Num());
                Result->SetNumberField(TEXT("connected_guests"), Entered);
                Result->SetNumberField(TEXT("rejected_room_full"), Rejected);
                Write(FPaths::ProjectSavedDir() / TEXT("Prototype/online82_capacity.json"), Result);
                UE_LOG(LogTemp, Display, TEXT("ONLINE82 capacity: host=%d connected=%d rejected=%d; %s"),
                    Lobby->Members.Num(), Entered, Rejected, Passed ? TEXT("PASS") : TEXT("FAIL"));
                FPlatformMisc::RequestExit(false);
                return false;
            }
            return true;
        }), .045f);
        return;
    }
    FString MapCheck;
    if (FParse::Value(FCommandLine::Get(), TEXT("WardOnlineMapCheck82="), MapCheck))
    {
        const bool Hosting = MapCheck == TEXT("host");
        const auto Lobby = Session(Game);
        const auto Start = FPlatformTime::Seconds();
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
            [Lobby, Hosting, Start, MapSince = 0.0, CaptureFrame = uint64(0), Step = 0, Scanned = false, Moved = false,
                StartAt = FVector::ZeroVector](float) mutable
        {
            auto* Mode = Lobby->Owner.Get();
            if (!Mode) return false;
            const double Now = FPlatformTime::Seconds();
            if (Step == 0 && Now - Start > 1.0 && Mode->Interface)
            {
                Mode->Command(TEXT("page_online"));
                if (Hosting)
                {
                    Mode->Command(TEXT("online_create"));
                    Lobby->RoomName = TEXT("ECHO map check 82");
                    Lobby->PlayerName = TEXT("Бека");
                    Lobby->Host();
                }
                else
                {
                    Mode->Command(TEXT("online_browse"));
                    Lobby->PlayerName = TEXT("Аружан");
                }
                Step = 1;
            }
            if (Step == 1 && !Hosting && Lobby->Role == ERole::Idle)
            {
                if (const auto* Listing = Lobby->Available.FindByPredicate([](const FRoom& Entry) {
                    return Entry.Name == TEXT("ECHO map check 82") && RelayPeer(Entry.Address);
                }))
                {
                    Lobby->SelectedRoom = Listing->Id;
                    Scanned = true;
                    Lobby->JoinFound();
                }
            }
            if (Step == 1 && Lobby->Role == (Hosting ? ERole::Host : ERole::Guest))
            {
                Lobby->ToggleReady();
                Step = 2;
            }
            if (Step == 2 && Hosting && Lobby->CanStart()) Lobby->Start();
            if (Lobby->InMap && Mode->Hero() && Mode->Screen.IsEmpty())
            {
                const auto Here = Mode->Hero()->GetActorLocation();
                if (MapSince == 0) { MapSince = Now; StartAt = Here; }
                if (Now - MapSince < 1.6) Mode->Hero()->AddMovementInput(FVector(0, 1, 0));
                Moved |= FVector::Dist2D(Here, StartAt) > 45;
                if (const auto* Avatar = Lobby->RemoteAvatar(); Step == 2 && Avatar && Moved)
                {
                    if (auto* Controller = Cast<APlayerController>(Mode->Hero()->GetController()))
                    {
                        const FVector Direction = (Avatar->GetActorLocation() + FVector(0, 0, 135)) -
                            Mode->Hero()->GetPawnViewLocation();
                        Controller->SetControlRotation(Direction.Rotation());
                    }
                    CaptureFrame = GFrameCounter + 8;
                    Step = 3;
                }
                if (Step == 3 && GFrameCounter >= CaptureFrame)
                {
                    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Prototype/online82_map.png"), true, false);
                    CaptureFrame = GFrameCounter + 5;
                    Step = 4;
                }
            }
            if ((Step == 4 && GFrameCounter >= CaptureFrame) || Now - Start > 78)
            {
                const bool Passed = Step == 4 && (Hosting || Scanned);
                const auto Result = MakeShared<FJsonObject>();
                Result->SetBoolField(TEXT("success"), Passed);
                Result->SetBoolField(TEXT("map_loaded"), Lobby->InMap);
                Result->SetBoolField(TEXT("remote_model_visible"), Lobby->RemoteAvatar() != nullptr);
                Result->SetBoolField(TEXT("local_character_moved"), Moved);
                Result->SetStringField(TEXT("status"), Lobby->Status);
                Write(FPaths::ProjectSavedDir() / TEXT("Prototype/online82_map.json"), Result);
                UE_LOG(LogTemp, Display, TEXT("ONLINE82 separate process map test %s"), Passed ? TEXT("PASS") : TEXT("FAIL"));
                FPlatformMisc::RequestExit(false);
                return false;
            }
            return true;
        }), .045f);
        return;
    }
    if (!FParse::Param(FCommandLine::Get(), TEXT("WardOnlineCheck80"))) return;
    struct FAssessment
    {
        TWeakObjectPtr<AEWardCampaignMode> Game;
        TArray<TSharedPtr<FOnline>> Guests;
        TArray<TSharedPtr<FJsonValue>> Checks;
        int32 Step = 0;
        double Due = 0;
        double Deadline = 0;
        bool Passed = true;
        bool ChoiceSent = false;
        bool ReadinessSet = false;
        int32 FullRoomShots = 0;

        void Check(const TCHAR* Id, bool Result)
        {
            Json Row = MakeShared<FJsonObject>();
            Row->SetStringField(TEXT("id"), Id);
            Row->SetBoolField(TEXT("passed"), Result);
            Checks.Add(MakeShared<FJsonValueObject>(Row));
            Passed &= Result;
            UE_LOG(LogTemp, Display, TEXT("ONLINE80 %s %s"), Result ? TEXT("PASS") : TEXT("FAIL"), Id);
        }

        void Advance(double After = 0.45)
        {
            ++Step;
            const bool RelayOnly = FParse::Param(FCommandLine::Get(), TEXT("EchoRelayOnly"));
            Due = FPlatformTime::Seconds() + After * (RelayOnly ? 2.0 : 1.0);
            Deadline = Due + (RelayOnly ? 12.0 : 0.0);
        }

        bool Waiting(bool Complete) const { return !Complete && FPlatformTime::Seconds() < Deadline; }

        void Capture(const TCHAR* Name)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Prototype") / Name, true, false);
        }

        bool Tick()
        {
            auto* Mode = Game.Get();
            if (!Mode) return false;
            for (auto& Guest : Guests) Guest->Tick();
            if (FPlatformTime::Seconds() < Due || !Mode->Interface) return true;
            auto Host = Session(*Mode);
            switch (Step)
            {
            case 0:
                Check(TEXT("title_online_button"), Mode->Interface->TitleActions.Contains(TEXT("page_online")));
                Mode->Command(TEXT("page_online"));
                Check(TEXT("online_hub"), Mode->Screen == HomePage);
                Capture(TEXT("online80_home.png"));
                Advance();
                break;
            case 1:
                Mode->Command(TEXT("online_create"));
                Check(TEXT("create_screen"), Mode->Screen == CreatePage);
                Capture(TEXT("online80_create.png"));
                Advance();
                break;
            case 2:
                Host->RoomName = TEXT("Ночная смена");
                Host->PlayerName = TEXT("Бека");
                Host->MaxPlayers = 10;
                Host->Access = EAccess::Password;
                Host->Password = TEXT("test-password");
                Host->Host();
                Check(TEXT("host_opens_lobby_with_limit_and_private_code"), Mode->Screen == LobbyPage &&
                    Host->Role == ERole::Host && Host->Room.MaxPlayers == 10 && !Host->InvitationCode().IsEmpty());
                Check(TEXT("solo_cannot_start"), !Host->CanStart());
                Guests.Add(MakeShared<FOnline>(*Mode, false));
                Guests[0]->PlayerName = TEXT("Аружан");
                Guests[0]->Browse();
                Advance(0.85);
                break;
            case 3:
                if (Waiting(Guests[0]->Available.Num() == 1)) return true;
                Check(TEXT("room_discovered_with_map_capacity_chapter_and_ping"), Guests[0]->Available.Num() == 1 &&
                    Guests[0]->Available[0].Name == Host->RoomName && Guests[0]->Available[0].Players == 1 &&
                    Guests[0]->Available[0].Chapter == Host->Room.Chapter && Guests[0]->Available[0].Ping > 0 &&
                    Guests[0]->Available[0].Access == EAccess::Password);
                Guests[0]->SelectedRoom = Host->Room.Id;
                Guests[0]->JoinPassword = TEXT("incorrect");
                Guests[0]->JoinFound();
                Advance(0.8);
                break;
            case 4:
                if (Waiting(Guests[0]->Role == ERole::Idle)) return true;
                Check(TEXT("incorrect_password_rejected"), Guests[0]->Role == ERole::Idle && Host->Members.Num() == 1 &&
                    Guests[0]->Status.Contains(TEXT("Неверный пароль")));
                Guests[0]->JoinPassword = TEXT("test-password");
                Guests[0]->Invitation = Host->InvitationCode();
                Guests[0]->JoinCode();
                Advance(1.0);
                break;
            case 5:
                if (Waiting(Guests[0]->Role == ERole::Guest && Host->Members.Num() == 2)) return true;
                UE_LOG(LogTemp, Display, TEXT("ONLINE80 join: host=%d guest=%d role=%d status=%s"),
                    Host->Members.Num(), Guests[0]->Members.Num(), int32(Guests[0]->Role), *Guests[0]->Status);
                Check(TEXT("guest_join_synchronizes_roster"), Guests[0]->Role == ERole::Guest &&
                    Host->Members.Num() == 2 && Guests[0]->Members.Num() == 2);
                Check(TEXT("host_is_beka_and_companion_has_default"), Host->LocalCharacter() == 0 &&
                    Guests[0]->LocalCharacter() == 1 && !Guests[0]->CanChoose(0));
                Guests[0]->ToggleReady();
                Advance(1.3);
                break;
            case 6:
                if (!ChoiceSent)
                {
                    if (Waiting(Host->Members.Num() == 2 && Host->Members[1].Ready)) return true;
                    Check(TEXT("ready_companion_can_change_choice"), Host->Members.Num() == 2 && Host->Members[1].Ready);
                    Guests[0]->ChooseCharacter(5);
                    ChoiceSent = true;
                    Due = FPlatformTime::Seconds() + 1.3;
                    Deadline = Due + 4.0;
                    return true;
                }
                if (!ReadinessSet)
                {
                    if (Waiting(!Guests[0]->CharacterPending())) return true;
                    Check(TEXT("companion_choice_confirmed_by_both_peers"), !Guests[0]->CharacterPending() &&
                        Guests[0]->LocalCharacter() == 5 && Host->Members.Num() == 2 && Host->Members[1].Character == 5);
                    Check(TEXT("changing_character_clears_ready"), !Guests[0]->IsReady() && !Host->CanStart());
                    Host->ToggleReady();
                    Guests[0]->ToggleReady();
                    ReadinessSet = true;
                    Due = FPlatformTime::Seconds() + 1.0;
                    return true;
                }
                if (Waiting(Host->CanStart())) return true;
                Check(TEXT("two_ready_players_can_start_room_of_ten"), Host->CanStart());
                Capture(TEXT("online80_lobby.png"));
                Advance();
                break;
            case 7:
                Host->Start();
                Advance();
                break;
            case 8:
                if (Waiting(Host->Room.Committed && Guests[0]->Room.Committed)) return true;
                Check(TEXT("start_commitment_propagates_to_guest"), Host->Room.Committed && Guests[0]->Room.Committed);
                Host->Reopen();
                for (int32 Index = 1; Index <= 9; ++Index)
                {
                    auto Guest = MakeShared<FOnline>(*Mode, false);
                    Guest->PlayerName = FString::Printf(TEXT("Игрок %02d"), Index + 2);
                    Guest->Invitation = Host->InvitationCode();
                    Guest->JoinPassword = TEXT("test-password");
                    Guest->JoinCode();
                    Guests.Add(Guest);
                }
                Advance(1.2);
                break;
            case 9:
            {
                if (FullRoomShots > 0)
                {
                    const int32 Cue = FullRoomShots++;
                    if (Cue <= 2)
                    {
                        Capture(Cue == 1 ? TEXT("online95_waiting.png") : TEXT("online95_waiting_later.png"));
                        Due = FPlatformTime::Seconds() + (Cue == 1 ? .82 : .26);
                    }
                    else if (Cue < 3 + WardOnlineRoster92::Count * 2)
                    {
                        const int32 Index = (Cue - 3) / 2;
                        if ((Cue - 3) % 2 == 0)
                        {
                            Host->PreviewCharacter = Index;
                            ++Host->Revision;
                            Due = FPlatformTime::Seconds() + 1.15;
                        }
                        else
                        {
                            const FString Name = Index == 0 ? TEXT("online95_beka.png") : Index == 5 ?
                                TEXT("online96_sherry.png") : FString::Printf(TEXT("online96_character_%02d.png"), Index);
                            const auto Panel = SOnlinePanel::AssessmentPanel.Pin();
                            auto* Inspection = Panel ? Panel->InspectionForAssessment() : nullptr;
                            if (Inspection && Index > 0)
                            {
                                Inspection->ZoomOrbit(3);
                                const auto Shot = Name;
                                FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Shot](float) {
                                    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() /
                                        TEXT("Prototype") / Shot, true, false);
                                    return false;
                                }), .55f);
                            }
                            else Capture(*Name);
                            Due = FPlatformTime::Seconds() + .95;
                        }
                    }
                    else if (Cue == 23)
                    {
                        Host->PreviewCharacter = 0;
                        ++Host->Revision;
                        Due = FPlatformTime::Seconds() + .8;
                    }
                    else if (Cue == 24)
                    {
                        const auto Panel = SOnlinePanel::AssessmentPanel.Pin();
                        auto* Inspection = Panel ? Panel->InspectionForAssessment() : nullptr;
                        if (Inspection) Inspection->DragOrbit(-220, -7);
                        Due = FPlatformTime::Seconds() + .65;
                    }
                    else if (Cue == 25)
                    {
                        Capture(TEXT("online95_side.png"));
                        Due = FPlatformTime::Seconds() + .25;
                    }
                    else if (Cue == 26)
                    {
                        const auto Panel = SOnlinePanel::AssessmentPanel.Pin();
                        auto* Inspection = Panel ? Panel->InspectionForAssessment() : nullptr;
                        if (Inspection)
                        {
                            Inspection->DragOrbit(-199, 7);
                            Inspection->ZoomOrbit(2);
                        }
                        Due = FPlatformTime::Seconds() + 1.1;
                    }
                    else if (Cue == 27)
                    {
                        const auto Panel = SOnlinePanel::AssessmentPanel.Pin();
                        const auto* Inspection = Panel ? Panel->InspectionForAssessment() : nullptr;
                        const FVector2D View = Inspection ? Inspection->OrbitView() : FVector2D::ZeroVector;
                        Check(TEXT("lobby_camera_reaches_rear_and_zooms"), View.X > 162 && View.X < 179 &&
                            View.Y > 430 && View.Y < 510);
                        UE_LOG(LogTemp, Display, TEXT("ONLINE95_ORBIT yaw=%.1f distance=%.1f"), View.X, View.Y);
                        Capture(TEXT("online95_rear.png"));
                        Due = FPlatformTime::Seconds() + .38;
                    }
                    else
                    {
                        Host->Leave();
                        Advance();
                    }
                    break;
                }
                int32 Connected = 0;
                int32 Rejected = 0;
                for (const auto& Guest : Guests)
                {
                    Connected += Guest->Role == ERole::Guest && Guest->Members.Num() == 10;
                    Rejected += Guest->Role == ERole::Idle && Guest->Status.Contains(TEXT("Комната заполнена"));
                }
                if (Waiting(Host->Members.Num() == 10 && Connected == 9 && Rejected == 1)) return true;
                UE_LOG(LogTemp, Display, TEXT("ONLINE80 capacity: host=%d connected=%d rejected=%d"), Host->Members.Num(), Connected, Rejected);
                Check(TEXT("capacity_ten_and_eleventh_rejected"), Host->Members.Num() == 10 && Connected == 9 && Rejected == 1);
                TSet<int32> Assigned;
                for (const auto& Member : Host->Members) Assigned.Add(Member.Character);
                Check(TEXT("ten_unique_characters_confirmed_and_occupied_choice_disabled"), Assigned.Num() == WardOnlineRoster92::Count &&
                    !Guests[0]->CanChoose(1));
                FullRoomShots = 1;
                Due = FPlatformTime::Seconds() + .9;
                break;
            }
            case 10:
                if (Waiting(!Guests.ContainsByPredicate([](const TSharedPtr<FOnline>& Guest) { return Guest->Role == ERole::Guest; }))) return true;
                Check(TEXT("closing_host_disconnects_guests"), !Guests.ContainsByPredicate(
                    [](const TSharedPtr<FOnline>& Guest) { return Guest->Role == ERole::Guest; }));
                Guests.Reset();
                Host->Access = EAccess::Invite;
                Host->Host();
                Guests.Add(MakeShared<FOnline>(*Mode, false));
                Guests[0]->Browse();
                Advance();
                break;
            case 11:
                Check(TEXT("invitation_only_room_hidden_from_browser"), Guests[0]->Available.IsEmpty());
                Guests[0]->Invitation = Host->InvitationCode();
                Guests[0]->JoinCode();
                Advance();
                break;
            case 12:
                if (Waiting(Guests[0]->Role == ERole::Guest && Host->Members.Num() == 2)) return true;
                Check(TEXT("private_invitation_joins"), Guests[0]->Role == ERole::Guest && Host->Members.Num() == 2);
                Host->Leave();
                Guests.Reset();
                Mode->Command(TEXT("online_browse"));
                Check(TEXT("browser_empty_state"), Mode->Screen == BrowsePage && Host->Available.IsEmpty());
                Capture(TEXT("online80_browser.png"));
                Advance();
                break;
            default:
                Mode->Command(TEXT("online_back"));
                Mode->Command(TEXT("online_back"));
                Check(TEXT("back_returns_to_main_menu"), Mode->Screen == TEXT("title"));
                Json Result = MakeShared<FJsonObject>();
                Result->SetBoolField(TEXT("success"), Passed);
                Result->SetArrayField(TEXT("checks"), Checks);
                Write(FPaths::ProjectSavedDir() / TEXT("Prototype/online80_check.json"), Result);
                FPlatformMisc::RequestExit(false);
                return false;
            }
            return true;
        }
    };
    auto Assessment = MakeShared<FAssessment>();
    Assessment->Game = &Game;
    Assessment->Due = FPlatformTime::Seconds() + 2.5;
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Assessment](float) { return Assessment->Tick(); }), .06f);
}
}
