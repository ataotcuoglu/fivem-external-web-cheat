// ============================================================
//  GameSDK  —  GTA V / FiveM game structures, offsets, Ped
// ============================================================

#include <SimpleMath.h>
#pragma comment(lib, "winhttp.lib")
#include <winhttp.h>

// "grcWindow" string'i bellekte düz literal olarak görünmemesi için
// runtime'da karakter karakter inşa ediliyor.
static const char* BuildClassName() {
    static char cls[10] = {};
    // g r c W i n d o w \0
    cls[0]='g'; cls[1]='r'; cls[2]='c';
    cls[3]='W'; cls[4]='i'; cls[5]='n';
    cls[6]='d'; cls[7]='o'; cls[8]='w'; cls[9]='\0';
    return cls;
}

static struct GameStruct {
    DWORD       pID         = 0;
    HANDLE      hProcess    = nullptr;
    HWND        hWnd        = nullptr;
    std::string Path;
    std::string Version;
    RECT        lpRect      = {};
    POINT       lpPoint     = {};
} Game;

struct OffsetsStruct {
    uintptr_t GameBase              = 0;
    uintptr_t CitizenPlayernamesBase= 0;
    uintptr_t NetBase               = 0;
    uintptr_t GameWorld             = 0;
    uintptr_t ReplayInterface       = 0;
    uintptr_t ViewPort              = 0;
    uintptr_t Camera                = 0;
    uintptr_t BoneList              = 0;
    uintptr_t WeaponManager         = 0;
    uintptr_t HandleBullet          = 0;
    uintptr_t VisibleFlag           = 0;
    uintptr_t BlipList              = 0;
    uintptr_t Vehicle               = 0;
    uintptr_t Waypoint              = 0;
    uintptr_t LocalPlayer           = 0;
    uintptr_t PlayerInfo            = 0;
    uintptr_t Id                    = 0;
    uintptr_t Health                = 0;
    uintptr_t MaxHealth             = 0;
    uintptr_t Armor                 = 0;
} Offsets;

#include "Memory.hpp"

// ============================================================
//  Helpers
// ============================================================
float GetDistance(Vector3 a, Vector3 b) {
    float x = a.x - b.x, y = a.y - b.y, z = a.z - b.z;
    return sqrtf(x*x + y*y + z*z);
}

bool Vec3Empty(const Vector3& v) { return v == Vector3(0.f, 0.f, 0.f); }

Vector3 Normalize(const Vector3& v) {
    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len == 0.f) return {0,0,0};
    return {v.x/len, v.y/len, v.z/len};
}

Vector3 Vec3Transform(Vector3* vIn, Matrix* mIn) {
    return {
        vIn->x*mIn->_11 + vIn->y*mIn->_21 + vIn->z*mIn->_31 + mIn->_41,
        vIn->x*mIn->_12 + vIn->y*mIn->_22 + vIn->z*mIn->_32 + mIn->_42,
        vIn->x*mIn->_13 + vIn->y*mIn->_23 + vIn->z*mIn->_33 + mIn->_43
    };
}

// ============================================================
//  Bone enum
// ============================================================
enum Bone : int { Head=0, LeftFoot, RightFoot, LeftAnkle, RightAnkle,
                  LeftHand, RightHand, Neck, Hip };

struct Bones {
    Vector3 Head;       char p0[4]{};
    Vector3 LeftFoot;   char p1[4]{};
    Vector3 RightFoot;  char p2[4]{};
    Vector3 LeftAnkle;  char p3[4]{};
    Vector3 RightAnkle; char p4[4]{};
    Vector3 LeftHand;   char p5[4]{};
    Vector3 RightHand;  char p6[4]{};
    Vector3 Neck;       char p7[4]{};
    Vector3 Hip;
};

// ============================================================
//  Friend list
// ============================================================
struct Friend { int id; std::string alias; }; // alias: bos ise gercek isim gosterilir
std::vector<Friend> friendList;
// Oyunculara ozel isim alias'lari (ID -> alias)
std::unordered_map<int,std::string> g_playerAliases;

bool IsFriend(int searchId) {
    if (searchId < 0 || friendList.empty()) return false;
    for (const Friend& f : friendList)
        if (f.id == searchId) return true;
    return false;
}
void RemoveFriend(int removeId) {
    friendList.erase(std::remove_if(friendList.begin(), friendList.end(),
        [removeId](const Friend& f){ return f.id == removeId; }), friendList.end());
}

// ============================================================
//  Vehicle struct
// ============================================================
struct Vehicle {
    uintptr_t Pointer;
    Vector2   Location;
    float     Health;
    float     Distance;
};

// ============================================================
//  Player name — citizen-playernames-five.dll
//  Sabit offset: +0x30D38 (namesList ptr), +0x30D40 (count)
//  Her node: +0x00=next, +0x10=uint16 netId, +0x18=std::string (SSO)
// ============================================================

// Name cache (ID -> name), mutex ile korunan
static std::unordered_map<int, std::string> g_nameCache;
static std::mutex                            g_nameMutex;
static DWORD                                 g_nameCacheTime = 0;
static bool                                  g_nameReady     = false;

// MSVC SSO std::string okuma (reference implementasyondan alinmistir)
static std::string ReadStdString(uintptr_t address) {
    uint64_t chunk0 = ReadMemory<uint64_t>(address + 0x00);
    uint64_t chunk1 = ReadMemory<uint64_t>(address + 0x08);
    uint64_t length = ReadMemory<uint64_t>(address + 0x10);
    if (length == 0 || length > 64) return "";
    if (length <= 15) {
        // SSO: dizi icinde saklanir
        char buf[16] = {};
        memcpy(buf, &chunk0, std::min((uint64_t)8, length));
        if (length > 8) memcpy(buf + 8, &chunk1, std::min(length - 8, (uint64_t)8));
        buf[length] = '\0';
        // ASCII kontrolu
        for (size_t i = 0; i < length; ++i)
            if ((unsigned char)buf[i] < 0x20 || (unsigned char)buf[i] > 0x7E) return "";
        return std::string(buf, length);
    } else {
        // Heap string: chunk0 is pointer
        char buf[72] = {};
        SIZE_T got = 0;
        ReadProcessMemory(Game.hProcess, (LPVOID)chunk0, buf,
                          (DWORD)std::min(length, (uint64_t)64), &got);
        if (got < 2) return "";
        for (size_t i = 0; i < got; ++i)
            if ((unsigned char)buf[i] < 0x20 || (unsigned char)buf[i] > 0x7E) return "";
        return std::string(buf, got);
    }
}

// citizen-playernames-five.dll'den tum isimleri cache'e yukle
static void RefreshNameCache() {
    if (!Offsets.CitizenPlayernamesBase) return;

    uintptr_t namesList = ReadMemory<uintptr_t>(Offsets.CitizenPlayernamesBase + 0x30D38);
    int       listSize  = ReadMemory<int>       (Offsets.CitizenPlayernamesBase + 0x30D40);

    // Her 10 saniyede bir debug ciktisi
    static DWORD s_lastDbg = 0;
    DWORD nowT = GetTickCount();
    if (nowT - s_lastDbg > 10000) {
        s_lastDbg = nowT;
        char dbgBuf[160];
        size_t cached = 0;
        { std::lock_guard<std::mutex> lk(g_nameMutex); cached = g_nameCache.size(); }
        sprintf(dbgBuf, "[Names] DLL=0x%llX list=0x%llX size=%d cached=%zu",
                (unsigned long long)Offsets.CitizenPlayernamesBase,
                (unsigned long long)namesList, listSize, cached);
        Debug(dbgBuf, LOG_INFO);
    }

    if (!namesList || listSize <= 0 || listSize > 2048) return;

    std::unordered_map<int, std::string> newCache;
    newCache.reserve((size_t)listSize);

    uintptr_t current = ReadMemory<uintptr_t>(namesList);
    for (int i = 0; i < listSize && current; ++i) {
        uint16_t netId = ReadMemory<uint16_t>(current + 0x10);
        if (netId > 0 && netId < 2048) {
            std::string name = ReadStdString(current + 0x18);
            if (!name.empty())
                newCache[(int)netId] = std::move(name);
        }
        uintptr_t next = ReadMemory<uintptr_t>(current);
        if (!next || next == current) break;
        current = next;
    }

    {
        std::lock_guard<std::mutex> lk(g_nameMutex);
        // Merge — mevcut isimleri silme, yenileri ekle/guncelle
        for (auto& kv : newCache)
            g_nameCache.insert_or_assign(kv.first, kv.second);
        g_nameCacheTime = GetTickCount();
        g_nameReady     = !g_nameCache.empty();
    }
}

// ============================================================
//  HTTP players.json — FiveM sunucusundan oyuncu isimleri
// ============================================================
static std::string g_srvIp;
static int         g_srvPort = 0;

// FiveM crashometry dosyasindan son sunucu adresini oku
static bool ParseCrashometry() {
    if (!g_srvIp.empty() && g_srvPort > 0) return true;

    HKEY hKey = nullptr;
    WCHAR wBuf[MAX_PATH] = {};
    DWORD sz = sizeof(wBuf);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\CitizenFX\\FiveM", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        Debug("[Names] Registry key not found: SOFTWARE\\CitizenFX\\FiveM", LOG_INFO);
        return false;
    }
    bool ok = (RegQueryValueExW(hKey, L"Last Run Location", nullptr, nullptr, (LPBYTE)wBuf, &sz) == ERROR_SUCCESS);
    RegCloseKey(hKey);
    if (!ok) {
        Debug("[Names] 'Last Run Location' value missing in registry", LOG_INFO);
        return false;
    }

    char aPath[MAX_PATH] = {};
    WideCharToMultiByte(CP_ACP, 0, wBuf, -1, aPath, MAX_PATH, nullptr, nullptr);
    std::string basePath(aPath);
    if (!basePath.empty() && basePath.back() != '\\') basePath += '\\';

    std::string crashPath = basePath + "data\\cache\\crashometry";
    HANDLE hFile = CreateFileA(crashPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        char dbg[MAX_PATH + 40];
        sprintf_s(dbg, "[Names] Crashometry not found: %s", crashPath.c_str());
        Debug(dbg, LOG_INFO);
        return false;
    }

    DWORD fileSize = GetFileSize(hFile, nullptr);
    std::string raw;
    if (fileSize > 0 && fileSize < 8 * 1024 * 1024) {
        raw.resize(fileSize);
        DWORD nRead = 0;
        ReadFile(hFile, &raw[0], fileSize, &nRead, nullptr);
        raw.resize(nRead);
    }
    CloseHandle(hFile);
    if (raw.empty()) return false;

    // Crashometry'de "last_server_url" sonra "last_server" anahtar karsisi ip:port
    size_t p1 = raw.find("last_server_url");
    if (p1 == std::string::npos) return false;
    // "last_server_url"dan sonra "last_server" bul (ama "last_server_url" degil)
    size_t p2 = raw.find("last_server", p1 + 15);
    if (p2 == std::string::npos) return false;
    // "last_server" (11 karakter) sonrasinda non-printable'lari atla
    size_t start = p2 + 11;
    while (start < raw.size() && (unsigned char)raw[start] < 0x20) ++start;

    // Printable karakterleri oku
    std::string addrStr;
    for (size_t i = start; i < raw.size() && i < start + 64; ++i) {
        char c = raw[i];
        if (c == '\0' || (unsigned char)c < 0x20) break;
        addrStr += c;
    }

    // ip:port ayir
    auto colonPos = addrStr.rfind(':');
    if (colonPos == std::string::npos || colonPos == 0) return false;
    std::string ip   = addrStr.substr(0, colonPos);
    std::string portStr = addrStr.substr(colonPos + 1);
    int portNum = 0;
    for (char c : portStr) { if (!isdigit((unsigned char)c)) break; portNum = portNum*10 + (c-'0'); }
    if (ip.empty() || portNum <= 0 || portNum > 65535) {
        char dbg[128];
        sprintf_s(dbg, "[Names] Crashometry parse failed: addr='%s' port=%d", addrStr.c_str(), portNum);
        Debug(dbg, LOG_INFO);
        return false;
    }

    g_srvIp   = ip;
    g_srvPort = portNum;
    char dbg[80];
    sprintf(dbg, "[Names] Server: %s:%d", ip.c_str(), portNum);
    Debug(dbg, LOG_INFO);
    return true;
}

// WinHTTP ile GET istegi
static std::string WinHttpGet(const std::string& ip, int port, const char* path) {
    std::wstring wIp(ip.begin(), ip.end());
    HINTERNET hS = WinHttpOpen(L"a", WINHTTP_ACCESS_TYPE_NO_PROXY,
                               WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hS) return "";
    DWORD t = 3000;
    WinHttpSetOption(hS, WINHTTP_OPTION_CONNECT_TIMEOUT, &t, sizeof t);
    WinHttpSetOption(hS, WINHTTP_OPTION_RECEIVE_TIMEOUT, &t, sizeof t);
    WinHttpSetOption(hS, WINHTTP_OPTION_SEND_TIMEOUT,    &t, sizeof t);
    HINTERNET hC = WinHttpConnect(hS, wIp.c_str(), (INTERNET_PORT)port, 0);
    if (!hC) { WinHttpCloseHandle(hS); return ""; }
    std::wstring wPath(path, path + strlen(path));
    HINTERNET hR = WinHttpOpenRequest(hC, L"GET", wPath.c_str(), nullptr,
                                      WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hR) { WinHttpCloseHandle(hC); WinHttpCloseHandle(hS); return ""; }
    bool ok = WinHttpSendRequest(hR, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
           && WinHttpReceiveResponse(hR, nullptr);
    std::string result;
    if (ok) {
        DWORD avail = 0;
        while (WinHttpQueryDataAvailable(hR, &avail) && avail > 0) {
            std::vector<char> tmp(avail + 1, 0);
            DWORD got = 0;
            if (!WinHttpReadData(hR, tmp.data(), avail, &got)) break;
            result.append(tmp.data(), got);
            if (result.size() > 256 * 1024) break;
        }
    }
    WinHttpCloseHandle(hR); WinHttpCloseHandle(hC); WinHttpCloseHandle(hS);
    return result;
}

// players.json parse et ve cache'e ekle
static void FetchNamesFromHttp() {
    if (!ParseCrashometry()) return;

    std::string json = WinHttpGet(g_srvIp, g_srvPort, "/players.json");
    if (json.size() < 2 || json[0] != '[') {
        g_srvIp.clear();  // adres yanlis, tekrar kesfet
        return;
    }

    std::unordered_map<int, std::string> additions;
    size_t pos = 0;
    while (pos < json.size()) {
        size_t s = json.find('{', pos);
        if (s == std::string::npos) break;
        size_t e = json.find('}', s);
        if (e == std::string::npos) break;
        pos = e + 1;

        // "id": N
        auto ip2 = json.find("\"id\":", s);
        if (ip2 == std::string::npos || ip2 > e) continue;
        ip2 += 5;
        while (ip2 < e && json[ip2] == ' ') ++ip2;
        int id = 0; bool hd = false;
        while (ip2 < e && isdigit((unsigned char)json[ip2])) { id = id*10+(json[ip2++]-'0'); hd=true; }
        if (!hd || id <= 0 || id >= 2048) continue;

        // "name": "..."
        auto np = json.find("\"name\":", s);
        if (np == std::string::npos || np > e) continue;
        np += 7;
        while (np < e && json[np] != '"') ++np;
        if (np >= e) continue;
        ++np;  // opening "
        std::string name;
        while (np < e && json[np] != '"') {
            char c = json[np++];
            if (c == '\\') { if (np < e) ++np; continue; }
            name += c;
        }
        if (!name.empty()) additions[id] = std::move(name);
    }

    if (!additions.empty()) {
        std::lock_guard<std::mutex> lk(g_nameMutex);
        for (auto& kv : additions)
            g_nameCache.insert_or_assign(kv.first, kv.second);
        g_nameReady = true;
        char dbg[64];
        sprintf(dbg, "[Names] HTTP: %zu isim alindi", additions.size());
        Debug(dbg, LOG_INFO);
    }
}

// Arka planda surekli yenile (her 2 saniyede bir DLL, her 10 saniyede bir HTTP)
static void NameCacheThread() {
    int tick = 0;
    while (true) {
        RefreshNameCache();
        if (++tick >= 5) {
            tick = 0;
            FetchNamesFromHttp();
        }
        Sleep(2000);
    }
}

// Geri uyumluluk: GetPedName(id)
static bool IsValidName(const char* buf, size_t maxlen = 32) {
    size_t len = strnlen(buf, maxlen);
    if (len < 2 || len > 40) return false;
    for (size_t i = 0; i < len; ++i)
        if ((unsigned char)buf[i] < 0x20 || (unsigned char)buf[i] > 0x7E) return false;
    return true;
}

std::string GetPedName(int pedId) {
    if (pedId <= 0) return "";
    // Kullanici alias'i varsa once onu don
    {
        std::lock_guard<std::mutex> lk(g_nameMutex);
        auto ait = g_playerAliases.find(pedId);
        if (ait != g_playerAliases.end() && !ait->second.empty())
            return ait->second;
    }
    // Cache'ten bak
    {
        std::lock_guard<std::mutex> lk(g_nameMutex);
        auto it = g_nameCache.find(pedId);
        if (it != g_nameCache.end()) return it->second;
    }
    return "";
}

// Artik kullanilmiyor — eski koda uyumluluk icin bos stub
void ScanCitizenNames(int) {}

// Artik kullanilmiyor — stub
struct CitNameCache { std::unordered_map<int,std::string> names; bool ready=false; bool scanning=false; DWORD cacheTime=0; } g_cit;

// FALLBACK — PlayerInfo pointer'indan isim okuma girisimi (genellikle basarisiz)
static std::string TryReadNameFromPlayerInfo(uintptr_t pedPtr) {
    if (!pedPtr) return "";
    uintptr_t pi = ReadMemory<uintptr_t>(pedPtr + Offsets.PlayerInfo);
    if (pi < 0x100000ULL) return "";
    static const int kOff[] = {0xA0, 0x20, 0x78, 0x80, 0x90, 0x98, 0x170, 0x28, 0xB0, 0xC0};
    for (int off : kOff) {
        char buf[32] = {};
        if (!ReadProcessMemory(Game.hProcess, (LPVOID)(pi + off), buf, 28, nullptr)) continue;
        if (IsValidName(buf, 28)) return std::string(buf, strnlen(buf, 28));
        uintptr_t ptr = *reinterpret_cast<uintptr_t*>(buf);
        if (ptr >= 0x100000ULL && ptr <= 0x7FFFFFFFFFFFULL) {
            char buf2[32] = {};
            if (ReadProcessMemory(Game.hProcess, (LPVOID)ptr, buf2, 28, nullptr) && IsValidName(buf2, 28))
                return std::string(buf2, strnlen(buf2, 28));
        }
    }
    return "";
}

// ESKI KOD — artik kullanilmiyor, derleme uyumlulugu icin stub
std::string GetPedName_unused(int pedId) {
    if (pedId < 0) return "P#" + std::to_string(pedId);
    auto ait = g_playerAliases.find(pedId);
    if (ait != g_playerAliases.end() && !ait->second.empty())
        return ait->second;
    if (!Offsets.CitizenPlayernamesBase || !g_cit.ready)
        return "P#" + std::to_string(pedId);

    DWORD now = GetTickCount();
    if (now - g_cit.cacheTime > 4000) { g_cit.names.clear(); }
    auto it = g_cit.names.find(pedId);
    if (it != g_cit.names.end()) return it->second;

    return "P#" + std::to_string(pedId);
}

// ============================================================
//  Ped class
// ============================================================
class Ped {
public:
    uintptr_t Pointer       = 0;
    uintptr_t PlayerInfo    = 0;
    uintptr_t WeaponManager = 0;
    Vector3   Position      = {};
    Matrix    BoneMatrix    = {};
    Vector3   BoneList[9]   = {};
    bool      IsFriend_     = false;

    bool GetPlayer(uintptr_t& Base) {
        Pointer = Base;
        return Pointer != 0;
    }

    bool Update() {
        if (!Pointer) return false;
        PlayerInfo    = ReadMemory<uintptr_t>(Pointer + Offsets.PlayerInfo);
        // Require a valid PlayerInfo (proves this is a real networked player, not garbage)
        if (!PlayerInfo || PlayerInfo < 0x100000ULL) return false;
        Position      = ReadMemory<Vector3>(Pointer + 0x90);
        BoneMatrix    = ReadMemory<Matrix>(Pointer + 0x60);
        UpdateBones();
        return true;
    }

    void UpdateBones() {
        Bones b = ReadMemory<Bones>(Pointer + Offsets.BoneList);
        BoneList[Head]       = Vec3Transform(&b.Head,       &BoneMatrix);
        BoneList[LeftFoot]   = Vec3Transform(&b.LeftFoot,   &BoneMatrix);
        BoneList[RightFoot]  = Vec3Transform(&b.RightFoot,  &BoneMatrix);
        BoneList[LeftAnkle]  = Vec3Transform(&b.LeftAnkle,  &BoneMatrix);
        BoneList[RightAnkle] = Vec3Transform(&b.RightAnkle, &BoneMatrix);
        BoneList[LeftHand]   = Vec3Transform(&b.LeftHand,   &BoneMatrix);
        BoneList[RightHand]  = Vec3Transform(&b.RightHand,  &BoneMatrix);
        BoneList[Neck]       = Vec3Transform(&b.Neck,       &BoneMatrix);
        BoneList[Hip]        = Vec3Transform(&b.Hip,        &BoneMatrix);

        // Eğer kemik pozisyonu (0,0,0) geliyorsa (offset hatalı veya animasyon verisi
        // henüz hazır değil), kafa pozisyonundan tahmin yoluyla doldur.
        // GTA V Z-yukarı koordinat sistemi; kafa en yüksek Z değerinde.
        const Vector3& h = BoneList[Head];
        if (!Vec3Empty(h)) {
            if (Vec3Empty(BoneList[Neck]))
                BoneList[Neck]      = { h.x,        h.y, h.z - 0.12f };
            if (Vec3Empty(BoneList[Hip]))        // "Chest" seçeneği bu kemiği kullanır
                BoneList[Hip]       = { h.x,        h.y, h.z - 0.42f };
            if (Vec3Empty(BoneList[LeftHand]))
                BoneList[LeftHand]  = { h.x - 0.38f, h.y, h.z - 0.52f };
            if (Vec3Empty(BoneList[RightHand]))
                BoneList[RightHand] = { h.x + 0.38f, h.y, h.z - 0.52f };
            if (Vec3Empty(BoneList[LeftFoot]))
                BoneList[LeftFoot]  = { h.x - 0.13f, h.y, h.z - 1.58f };
            if (Vec3Empty(BoneList[RightFoot]))
                BoneList[RightFoot] = { h.x + 0.13f, h.y, h.z - 1.58f };
            if (Vec3Empty(BoneList[LeftAnkle]))
                BoneList[LeftAnkle]  = { h.x - 0.11f, h.y, h.z - 1.48f };
            if (Vec3Empty(BoneList[RightAnkle]))
                BoneList[RightAnkle] = { h.x + 0.11f, h.y, h.z - 1.48f };
        }
    }

    int GetId() {
        uintptr_t pi = ReadMemory<uintptr_t>(Pointer + Offsets.PlayerInfo);
        if (!pi) return -1;
        return ReadMemory<int>(pi + Offsets.Id);
    }

    std::string GetName() {
        int id = GetId();
        std::string n = GetPedName(id);
        if (!n.empty()) return n;
        // Cache miss — try reading directly from PlayerInfo as fallback
        std::string direct = TryReadNameFromPlayerInfo(Pointer);
        if (!direct.empty()) {
            if (id > 0) {
                std::lock_guard<std::mutex> lk(g_nameMutex);
                g_nameCache[id] = direct;
            }
            return direct;
        }
        return "P#" + std::to_string(id);
    }

    std::string GetWeaponName() {
        uintptr_t wm   = ReadMemory<uintptr_t>(Pointer + Offsets.WeaponManager);
        uintptr_t info = ReadMemory<uintptr_t>(wm + 0x20);
        return ReadString(ReadMemory<uintptr_t>(info + 0x5F0));
    }

    float GetHealth() { return ReadMemory<float>(Pointer + Offsets.Health); }
    float GetArmor()  { return ReadMemory<float>(Pointer + Offsets.Armor);  }

    bool IsPlayer() {
        if (GetId() <= 0) return false;
        return PlayerInfo != 0;
    }

    bool IsDead()    { return Vec3Empty(Position); }

    bool IsVisible() {
        BYTE f = ReadMemory<BYTE>(Pointer + Offsets.VisibleFlag);
        return !(f == 36 || f == 0 || f == 4 || f == 2 ||
                 (f & 256) || (f & 512) || (f & 1024) || (f & 2048));
    }
};

// ============================================================
//  DemoUser  —  local player info shown in the menu
// ============================================================
struct DemoUserStruct {
    int         Id         = 0;
    std::string Name       = "Player";
    std::string WeaponName = "";
    float       Health     = 0.f;
    float       Armor      = 0.f;
} DemoUser;

// ============================================================
//  Attach — External: FiveM penceresini bul, handle aç.
// ============================================================
bool AttachToGameProcess() {
    Game.hWnd = FindWindowA(BuildClassName(), nullptr);
    if (!Game.hWnd) return false;
    GetWindowThreadProcessId(Game.hWnd, &Game.pID);
    if (!Game.pID) return false;
    if (Game.hProcess) { CloseHandle(Game.hProcess); Game.hProcess = nullptr; }
    Game.hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Game.pID);
    return Game.hProcess != nullptr;
}

// ============================================================
//  Fetch game path & build version from CitizenFX.ini
// ============================================================
bool FetchGamePathAndVersion() {
    if (Game.Path.empty() && Game.hProcess) {
        char path[MAX_PATH] = {};
        if (GetModuleFileNameExA(Game.hProcess, NULL, path, MAX_PATH) > 0)
            Game.Path = path;
    }
    if (Game.Version.empty() && !Game.Path.empty()) {
        size_t pos = Game.Path.find(XorString("data\\cache\\subprocess"));
        if (pos != std::string::npos) {
            std::string ini = Game.Path.substr(0, pos) + XorString("CitizenFX.ini");
            char ver[32] = {};
            if (GetPrivateProfileStringA(XorString("Game"), XorString("SavedBuildNumber"),
                                         "", ver, sizeof(ver), ini.c_str()) > 0)
                Game.Version = ver;
        }
    }
    return !Game.Path.empty();
}

// ============================================================
//  Detect version & fill offsets
// ============================================================
bool FetchOffsetsAndVersion() {
    // External: FiveM base adresi için TlHelp32 taraması (30sn timeout)
    for (int i = 0; i < 300 && !Offsets.GameBase; ++i) {
        Offsets.GameBase = GetBaseAddress();
        if (!Offsets.GameBase) Sleep(100);
    }
    if (!Offsets.GameBase) return false;

    // Bu DLL'ler opsiyonel — bulunamazsa isim sistemi çalışmaz ama hile çalışır
    for (int i = 0; i < 50 && !Offsets.CitizenPlayernamesBase; i++) {
        Offsets.CitizenPlayernamesBase = GetBaseAddress(XorString("citizen-playernames-five.dll"));
        Sleep(100);
    }

    for (int i = 0; i < 50 && !Offsets.NetBase; i++) {
        Offsets.NetBase = GetBaseAddress(XorString("net.dll"));
        Sleep(100);
    }

    FetchGamePathAndVersion();

    // ----- version blocks -----
    struct VersionEntry {
        const char* ver;
        uintptr_t GW,RI,VP,CAM,BL,WM,HB,VF,BL2,VEH,WP,LP,PI,ID,HP,MHP,ARM;
    };
    // ver  |  GW        RI        VP        CAM       BL    WM     HB        VF     BL2       VEH   WP        LP PI     ID   HP    MHP   ARM
    static const VersionEntry versions[] = {
        // b3570 — en guncel (World 0x25EC580)
        {"3570",0x25EC580,0x1FB0418,0x2058BA0,0x2059A48,0x410,0x10B8,0x102D550,0x145C,0x2061870,0x0D10,0x2047D50,0x8,0x10A8,0xE8,0x280,0x284,0x150C},
        // b3407
        {"3407",0x25D7108,0x1F9A9D8,0x20431C0,0x20440C8,0x410,0x10B8,0x102FF8C,0x145C,0x20440C8,0x0D10,0x2047D50,0x8,0x10A8,0xE8,0x280,0x284,0x150C},
        // b3323
        {"3323",0x25C15B0,0x1F85458,0x202DC50,0x202E878,0x410,0x10B8,0x1026CB0,0x145C,0x2002888,0x0D10,0x2022DE0,0x8,0x10A8,0xE8,0x280,0x284,0x150C},
        // b3258
        {"3258",0x25B14B0,0x1FBD4F0,0x201DBA0,0x201E7D0,0x410,0x10B8,0x101A660,0x145C,0x2002FA0,0x0D10,0x2023400,0x8,0x10A8,0xE8,0x280,0x284,0x150C},
        // b3095
        {"3095",0x2593320,0x1F58B58,0x20019E0,0x20025B8,0x410,0x10B8,0x100F5A4,0x145C,0x2002888,0x0D10,0x2002FA0,0x8,0x10A8,0xE8,0x280,0x284,0x150C},
        // b2944
        {"2944",0x257BEA0,0x1F42068,0x1FEAAC0,0x1FEB968,0x410,0x10B8,0x1003F80,0x145C,0x1FEB968,0x0D10,0x1FF3130,0x8,0x10A8,0xE8,0x280,0x284,0x150C},
        // b2802
        {"2802",0x254D448,0x1F5B820,0x1FBC100,0x1FBCCD8,0x410,0x10B8,0xFF716C, 0x145C,0x1FBCFA8,0x0D10,0x1FBD6E0,0x8,0x10A8,0xE8,0x280,0x284,0x150C},
        // b2699
        {"2699",0x26684D8,0x20304C8,0x20D8C90,0x20D9868,0x430,0x10D8,0xFF9D90, 0x147C,0x20D9B38,0xD30, 0x20E1420,0x8,0x10C8,0x88,0x280,0x2A0, 0x1530},
        // b2612
        {"2612",0x2567DB0,0x1F77EF0,0x1FD8570,0x1FD9418,0x430,0x10D8,0xFF340C, 0x147C,0x1FD9418,0xD30, 0x1FDDD20,0x8,0x10C8,0x88,0x280,0x2A0, 0x1530},
        // b2545
        {"2545",0x25667E8,0x1F2E7A8,0x1FD6F70,0x1FD7E18,0x430,0x10D8,0xFF1B40, 0x147C,0x1FD7E18,0xD30, 0x1F4F940,0x8,0x10A8,0x88,0x280,0x2A0, 0x1530},
        // b2372
        {"2372",0x252DCD8,0x1F05208,0x1F9E9F0,0x1F9F898,0x430,0x10D8,0xFE7EF8, 0x142C,0x1F9F898,0xD30, 0x1F9FFA0,0x8,0x10C8,0x88,0x280,0x2A0, 0x14E0},
        // b2189
        {"2189",0x24E6D90,0x1EE18A8,0x1F888C0,0x1F89768,0x430,0x10D8,0xFE2154, 0x142C,0x1F89768,0xD30, 0x1F6EF80,0x8,0x10C8,0x78,0x280,0x2A0, 0x14E0},
        // b2060
        {"2060",0x24C8858,0x1EC3828,0x1F6A7E0,0x1F6B940,0x430,0x10D8,0xFCE6EC, 0x142C,0x1F6B940,0xD28, 0x1F4F940,0x8,0x10A8,0x78,0x280,0x2A0, 0x14E0},
    };

    auto applyVersion = [&](const VersionEntry& v) {
        Offsets.GameWorld=v.GW; Offsets.ReplayInterface=v.RI; Offsets.ViewPort=v.VP;
        Offsets.Camera=v.CAM; Offsets.BoneList=v.BL; Offsets.WeaponManager=v.WM;
        Offsets.HandleBullet=v.HB; Offsets.VisibleFlag=v.VF; Offsets.BlipList=v.BL2;
        Offsets.Vehicle=v.VEH; Offsets.Waypoint=v.WP; Offsets.LocalPlayer=v.LP;
        Offsets.PlayerInfo=v.PI; Offsets.Id=v.ID; Offsets.Health=v.HP;
        Offsets.MaxHealth=v.MHP; Offsets.Armor=v.ARM;
        Game.Version = v.ver;
        char buf[64];
        sprintf(buf, XorString("Version detected: %s"), v.ver);
        Debug(buf, LOG_SUCCESSFUL);
    };

    while (true) {
        // 1) Try exact build number match from CitizenFX.ini first
        if (!Game.Version.empty()) {
            for (auto& v : versions) {
                if (strncmp(Game.Version.c_str(), v.ver, strlen(v.ver)) == 0) {
                    applyVersion(v);
                    return true;
                }
            }
        }

        // 2) Fall back: pick first version whose game pointers are non-null
        for (auto& v : versions) {
            auto gw = ReadMemory<uintptr_t>(Offsets.GameBase + v.GW);
            auto vp = ReadMemory<uintptr_t>(Offsets.GameBase + v.VP);
            auto ri = ReadMemory<uintptr_t>(Offsets.GameBase + v.RI);
            if (gw && vp && ri) {
                applyVersion(v);
                return true;
            }
        }
        Sleep(500);
    }
}
