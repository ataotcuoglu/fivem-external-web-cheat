#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <ShlObj.h>
#include <string>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <mutex>
#pragma comment(lib, "Ws2_32.lib")

extern bool g_gameConnected;

static WORD        g_webPort  = 7531;
static std::string g_localIP  = "127.0.0.1";
static std::mutex  g_webMtx;


static std::string GetLocalIP() {
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return "127.0.0.1";
    sockaddr_in remote = {};
    remote.sin_family = AF_INET;
    remote.sin_port   = htons(53);
    InetPtonA(AF_INET, "8.8.8.8", &remote.sin_addr);
    if (connect(s, (sockaddr*)&remote, sizeof(remote)) != 0) {
        closesocket(s); return "127.0.0.1";
    }
    sockaddr_in local = {};
    int len = sizeof(local);
    getsockname(s, (sockaddr*)&local, &len);
    closesocket(s);
    char buf[INET_ADDRSTRLEN] = {};
    InetNtopA(AF_INET, &local.sin_addr, buf, sizeof(buf));
    return std::string(buf[0] ? buf : "127.0.0.1");
}




static std::string ColToHex(ImColor c) {
    ImVec4 v = (ImVec4)c;
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x",
        (int)(v.x*255.f+.5f),(int)(v.y*255.f+.5f),(int)(v.z*255.f+.5f));
    return buf;
}
static void HexToCol(const std::string& h, ImColor& out) {
    if (h.size() < 7 || h[0] != '#') return;
    unsigned r=0,g=0,b=0;
    sscanf_s(h.c_str()+1, "%02x%02x%02x", &r, &g, &b);
    out = ImColor((int)r,(int)g,(int)b,255);
}


static std::string BuildSettingsJson() {
    std::lock_guard<std::mutex> lk(g_webMtx);
    std::ostringstream j;
    j << "{";
    auto jb = [&](const char* k, bool v)    { j<<"\""<<k<<"\":"<<(v?"true":"false")<<","; };
    auto ji = [&](const char* k, int v)     { j<<"\""<<k<<"\":" <<v<<","; };
    auto jc = [&](const char* k, ImColor v) { j<<"\""<<k<<"\":\""<<ColToHex(v)<<"\","; };

 
    jb("silent_en",       Cheats::AimAssist::Silent::Enabled);
    ji("silent_hk",       Cheats::AimAssist::Silent::HotKey);
    ji("silent_fov",      Cheats::AimAssist::Silent::Fov);
    ji("silent_dist",     Cheats::AimAssist::Silent::MaxDistance);
    ji("silent_miss",     Cheats::AimAssist::Silent::MissChance);
    ji("silent_bone",     Cheats::AimAssist::Silent::BoneMode);
    jb("silent_fov_draw", Cheats::AimAssist::Silent::DrawFov);
    ji("silent_fov_w",    Cheats::AimAssist::Silent::FovWeight);
    jc("c_silent_fov",    Cheats::AimAssist::Silent::FovColor);
    jb("silent_vis",      Cheats::AimAssist::Silent::VisCheck);
    jb("silent_skip",     Cheats::AimAssist::Silent::SkipFriends);
    jb("mb_en",      Cheats::AimAssist::MagicBullet::Enabled);
    ji("mb_bone",    Cheats::AimAssist::MagicBullet::BoneMode);
    ji("mb_dist",    Cheats::AimAssist::MagicBullet::MaxDistance);
    jb("mb_skipfr",  Cheats::AimAssist::MagicBullet::SkipFriends);
    jb("mb_skipnpc", Cheats::AimAssist::MagicBullet::SkipNPC);
    jb("aim_en",       Cheats::AimAssist::Aimbot::Enabled);
    ji("aim_hk",       Cheats::AimAssist::Aimbot::HotKey);
    ji("aim_fov",      Cheats::AimAssist::Aimbot::Fov);
    ji("aim_smooth",   Cheats::AimAssist::Aimbot::Smooth);
    ji("aim_dist",     Cheats::AimAssist::Aimbot::MaxDistance);
    ji("aim_bone",     Cheats::AimAssist::Aimbot::BoneMode);
    ji("aim_mode",     Cheats::AimAssist::Aimbot::AimMode);
    ji("aim_prio",     Cheats::AimAssist::Aimbot::Priority);
    jb("aim_fov_draw", Cheats::AimAssist::Aimbot::DrawFov);
    ji("aim_fov_w",    Cheats::AimAssist::Aimbot::FovWeight);
    jc("c_aim_fov",    Cheats::AimAssist::Aimbot::FovColor);
    jb("aim_sticky",   Cheats::AimAssist::Aimbot::StickyAim);
    jb("aim_vis",      Cheats::AimAssist::Aimbot::VisCheck);
    jb("aim_skip",     Cheats::AimAssist::Aimbot::SkipFriends);
    jb("trig_en",    Cheats::AimAssist::Triggerbot::Enabled);
    ji("trig_hk",    Cheats::AimAssist::Triggerbot::HotKey);
    ji("trig_delay", Cheats::AimAssist::Triggerbot::Delay);
    ji("trig_dist",  Cheats::AimAssist::Triggerbot::MaxDistance);
    jb("trig_skip",  Cheats::AimAssist::Triggerbot::SkipFriends);
    jb("wpn_ammo",      Cheats::AimAssist::Settings::InfiniteAmmo);
    jb("wpn_recoil",    Cheats::AimAssist::Settings::NoRecoil);
    jb("wpn_spread",    Cheats::AimAssist::Settings::NoSpread);
    jb("wpn_reload",    Cheats::AimAssist::Settings::NoReload);
    jb("wpn_rapid",     Cheats::World::RapidFire::Enabled);
    jb("wpn_crosshair", Cheats::AimAssist::Settings::Crosshair);
    ji("wpn_xtype",     Cheats::AimAssist::Settings::CrosshairSelectedType);
    ji("wpn_xsize",     Cheats::AimAssist::Settings::CrosshairSize);
    jb("esp_vis",      Cheats::Players::VisCheck::Enabled);
    jc("c_esp_vis",    Cheats::Players::VisCheck::VisibleColor);
    jc("c_esp_hid",    Cheats::Players::VisCheck::HiddenColor);
    ji("esp_linetype", Cheats::Players::VisualMarkers::GlobalSettings::SelectedLineType);
    ji("esp_linew",    Cheats::Players::VisualMarkers::GlobalSettings::LineWeight);
    jb("esp_skel",     Cheats::Players::VisualMarkers::DrawSkeleton::Enabled);
    jc("c_esp_skel",   Cheats::Players::VisualMarkers::DrawSkeleton::Color);
    jb("esp_box",      Cheats::Players::VisualMarkers::DrawBox::Enabled);
    ji("esp_boxtype",  Cheats::Players::VisualMarkers::DrawBox::SelectedType);
    jc("c_esp_box",    Cheats::Players::VisualMarkers::DrawBox::Color);
    jb("esp_line",     Cheats::Players::VisualMarkers::DrawLine::Enabled);
    ji("esp_lineloc",  Cheats::Players::VisualMarkers::DrawLine::SelectedLocation);
    jc("c_esp_line",   Cheats::Players::VisualMarkers::DrawLine::Color);
    jb("esp_bone",     Cheats::Players::VisualMarkers::DrawBonePoints::Enabled);
    ji("esp_bone_r",   Cheats::Players::VisualMarkers::DrawBonePoints::Radius);
    jc("c_esp_bone",   Cheats::Players::VisualMarkers::DrawBonePoints::Color);
    ji("esp_dist",     Cheats::Players::Settings::MaxDistance);
    jb("esp_noped",    (bool)Cheats::Players::Settings::IgnorePed);
    jb("esp_nodead",   (bool)Cheats::Players::Settings::IgnoreDeath);
    ji("pi_maxdist",  Cheats::Players::PlayerInfo::GlobalSettings::MaxDistance);
    jb("pi_name",     Cheats::Players::PlayerInfo::DrawName::Enabled);
    ji("pi_name_loc", Cheats::Players::PlayerInfo::DrawName::SelectedLocation);
    jc("c_pi_name",   Cheats::Players::PlayerInfo::DrawName::Color);
    jb("pi_id",       Cheats::Players::PlayerInfo::DrawId::Enabled);
    ji("pi_id_loc",   Cheats::Players::PlayerInfo::DrawId::SelectedLocation);
    jc("c_pi_id",     Cheats::Players::PlayerInfo::DrawId::Color);
    jb("pi_dist",     Cheats::Players::PlayerInfo::DrawDistance::Enabled);
    ji("pi_dist_loc", Cheats::Players::PlayerInfo::DrawDistance::SelectedLocation);
    jc("c_pi_dist",   Cheats::Players::PlayerInfo::DrawDistance::Color);
    jb("pi_weapon",   Cheats::Players::PlayerInfo::DrawWeaponName::Enabled);
    ji("pi_wpn_loc",  Cheats::Players::PlayerInfo::DrawWeaponName::SelectedLocation);
    jc("c_pi_weapon", Cheats::Players::PlayerInfo::DrawWeaponName::Color);
    jb("sb_hp",        Cheats::Players::StatusBars::DrawHealthBar::Enabled);
    ji("sb_hp_loc",    Cheats::Players::StatusBars::DrawHealthBar::SelectedLocation);
    jb("sb_armor",     Cheats::Players::StatusBars::DrawArmorBar::Enabled);
    ji("sb_armor_loc", Cheats::Players::StatusBars::DrawArmorBar::SelectedLocation);
    jb("veh_pt",       Cheats::Vehicles::DrawPoint::Enabled);
    jc("c_veh_pt",     Cheats::Vehicles::DrawPoint::Color);
    ji("veh_pt_size",  Cheats::Vehicles::DrawPoint::Size);
    jb("veh_ln",       Cheats::Vehicles::DrawLine::Enabled);
    jc("c_veh_ln",     Cheats::Vehicles::DrawLine::Color);
    ji("veh_ln_loc",   Cheats::Vehicles::DrawLine::SelectedLocation);
    jb("veh_dist_en",  Cheats::Vehicles::DrawDistance::Enabled);
    jc("c_veh_dist",   Cheats::Vehicles::DrawDistance::Color);
    jb("veh_hp",       Cheats::Vehicles::DrawHealthBar::Enabled);
    jb("veh_ignlocal", Cheats::Vehicles::Settings::IgnoreLocalVehicle);
    ji("veh_maxcount", Cheats::Vehicles::Settings::MaxVehicleCount);
    ji("veh_maxdist",  Cheats::Vehicles::Settings::MaxDistance);
    jb("veh_fix",      Cheats::Vehicles::VehicleFix::Enabled);
    ji("veh_fix_hk",   Cheats::Vehicles::VehicleFix::HotKey);
    jb("w_noclip",    Cheats::World::NoClip::Enabled);
    ji("w_noclip_spd",Cheats::World::NoClip::MovementSpeed);
    jb("w_semigod",   Cheats::World::SemiGodMode::Enabled);
    jb("w_sprint",    Cheats::World::SuperSprint::Enabled);
    ji("w_sprintspd", Cheats::World::SuperSprint::Speed);
    jb("w_stamina",   Cheats::World::InfiniteStamina::Enabled);
    jb("w_explode",   Cheats::World::ExplosiveBullets::Enabled);
    jb("w_fire",      Cheats::World::FireBullets::Enabled);
    ji("s_panickey", Cheats::Settings::PanicKey);
    ji("s_menukey",  Cheats::Settings::MenuKey);
    jb("s_stream",   Cheats::Settings::StreamProof);
    ji("s_theme",    Cheats::Settings::SelectedTheme);
    ji("s_maxplrs",  Cheats::Settings::MaxPlayerCount);

    j << "\"_\":0}";
    return j.str();
}

static std::string BuildStatusJson() {
    std::ostringstream j;
    j << "{\"connected\":" << (g_gameConnected?"true":"false")
      << ",\"version\":\"" << Game.Version << "\""
      << ",\"port\":"      << g_webPort
      << ",\"ip\":\""      << g_localIP << "\"}";
    return j.str();
}

static std::unordered_map<std::string,std::string> ParseFlatJson(const std::string& js) {
    std::unordered_map<std::string,std::string> m;
    size_t i = 0, n = js.size();
    while (i < n) {
        size_t qs = js.find('"', i);    if (qs==std::string::npos) break;
        size_t qe = js.find('"', qs+1); if (qe==std::string::npos) break;
        std::string key = js.substr(qs+1, qe-qs-1);
        size_t col = js.find(':', qe);  if (col==std::string::npos) break;
        size_t vs = col+1;
        while (vs < n && js[vs]==' ') vs++;
        std::string val;
        if (vs < n && js[vs]=='"') {
            size_t ve2 = js.find('"', vs+1); if (ve2==std::string::npos) break;
            val = js.substr(vs+1, ve2-vs-1);
            i = ve2+1;
        } else {
            size_t ve = vs;
            while (ve < n && js[ve]!=',' && js[ve]!='}') ve++;
            val = js.substr(vs, ve-vs);
            while (!val.empty()&&(val.back()==' '||val.back()=='\r'||val.back()=='\n')) val.pop_back();
            i = ve+1;
        }
        if (key != "_") m[key] = val;
    }
    return m;
}

static volatile DWORD g_cfgDirtyTick = 0;  

static bool ApplySettingsJson(const std::string& body) {
    auto m = ParseFlatJson(body);
    if (m.empty()) return false;
    std::lock_guard<std::mutex> lk(g_webMtx);

    auto gb = [&](const char* k, bool& v)    { auto it=m.find(k); if(it!=m.end()) v=(it->second=="true"||it->second=="1"); };
    auto gi = [&](const char* k, int& v)     { auto it=m.find(k); if(it!=m.end()) v=atoi(it->second.c_str()); };
    auto gc = [&](const char* k, ImColor& v) { auto it=m.find(k); if(it!=m.end()) HexToCol(it->second,v); };


    gb("silent_en",       Cheats::AimAssist::Silent::Enabled);
    gi("silent_hk",       Cheats::AimAssist::Silent::HotKey);
    gi("silent_fov",      Cheats::AimAssist::Silent::Fov);
    gi("silent_dist",     Cheats::AimAssist::Silent::MaxDistance);
    gi("silent_miss",     Cheats::AimAssist::Silent::MissChance);
    gi("silent_bone",     Cheats::AimAssist::Silent::BoneMode);
    gb("silent_fov_draw", Cheats::AimAssist::Silent::DrawFov);
    gi("silent_fov_w",    Cheats::AimAssist::Silent::FovWeight);
    gc("c_silent_fov",    Cheats::AimAssist::Silent::FovColor);
    gb("silent_vis",      Cheats::AimAssist::Silent::VisCheck);
    gb("silent_skip",     Cheats::AimAssist::Silent::SkipFriends);
    gb("mb_en",      Cheats::AimAssist::MagicBullet::Enabled);
    gi("mb_bone",    Cheats::AimAssist::MagicBullet::BoneMode);
    gi("mb_dist",    Cheats::AimAssist::MagicBullet::MaxDistance);
    gb("mb_skipfr",  Cheats::AimAssist::MagicBullet::SkipFriends);
    gb("mb_skipnpc", Cheats::AimAssist::MagicBullet::SkipNPC);
    gb("aim_en",       Cheats::AimAssist::Aimbot::Enabled);
    gi("aim_hk",       Cheats::AimAssist::Aimbot::HotKey);
    gi("aim_fov",      Cheats::AimAssist::Aimbot::Fov);
    gi("aim_smooth",   Cheats::AimAssist::Aimbot::Smooth);
    gi("aim_dist",     Cheats::AimAssist::Aimbot::MaxDistance);
    gi("aim_bone",     Cheats::AimAssist::Aimbot::BoneMode);
    gi("aim_mode",     Cheats::AimAssist::Aimbot::AimMode);
    gi("aim_prio",     Cheats::AimAssist::Aimbot::Priority);
    gb("aim_fov_draw", Cheats::AimAssist::Aimbot::DrawFov);
    gi("aim_fov_w",    Cheats::AimAssist::Aimbot::FovWeight);
    gc("c_aim_fov",    Cheats::AimAssist::Aimbot::FovColor);
    gb("aim_sticky",   Cheats::AimAssist::Aimbot::StickyAim);
    gb("aim_vis",      Cheats::AimAssist::Aimbot::VisCheck);
    gb("aim_skip",     Cheats::AimAssist::Aimbot::SkipFriends);
    gb("trig_en",    Cheats::AimAssist::Triggerbot::Enabled);
    gi("trig_hk",    Cheats::AimAssist::Triggerbot::HotKey);
    gi("trig_delay", Cheats::AimAssist::Triggerbot::Delay);
    gi("trig_dist",  Cheats::AimAssist::Triggerbot::MaxDistance);
    gb("trig_skip",  Cheats::AimAssist::Triggerbot::SkipFriends);
    gb("wpn_ammo",      Cheats::AimAssist::Settings::InfiniteAmmo);
    gb("wpn_recoil",    Cheats::AimAssist::Settings::NoRecoil);
    gb("wpn_spread",    Cheats::AimAssist::Settings::NoSpread);
    gb("wpn_reload",    Cheats::AimAssist::Settings::NoReload);
    gb("wpn_rapid",     Cheats::World::RapidFire::Enabled);
    gb("wpn_crosshair", Cheats::AimAssist::Settings::Crosshair);
    gi("wpn_xtype",     Cheats::AimAssist::Settings::CrosshairSelectedType);
    gi("wpn_xsize",     Cheats::AimAssist::Settings::CrosshairSize);
    gb("esp_vis",      Cheats::Players::VisCheck::Enabled);
    gc("c_esp_vis",    Cheats::Players::VisCheck::VisibleColor);
    gc("c_esp_hid",    Cheats::Players::VisCheck::HiddenColor);
    gi("esp_linetype", Cheats::Players::VisualMarkers::GlobalSettings::SelectedLineType);
    gi("esp_linew",    Cheats::Players::VisualMarkers::GlobalSettings::LineWeight);
    gb("esp_skel",     Cheats::Players::VisualMarkers::DrawSkeleton::Enabled);
    gc("c_esp_skel",   Cheats::Players::VisualMarkers::DrawSkeleton::Color);
    gb("esp_box",      Cheats::Players::VisualMarkers::DrawBox::Enabled);
    gi("esp_boxtype",  Cheats::Players::VisualMarkers::DrawBox::SelectedType);
    gc("c_esp_box",    Cheats::Players::VisualMarkers::DrawBox::Color);
    gb("esp_line",     Cheats::Players::VisualMarkers::DrawLine::Enabled);
    gi("esp_lineloc",  Cheats::Players::VisualMarkers::DrawLine::SelectedLocation);
    gc("c_esp_line",   Cheats::Players::VisualMarkers::DrawLine::Color);
    gb("esp_bone",     Cheats::Players::VisualMarkers::DrawBonePoints::Enabled);
    gi("esp_bone_r",   Cheats::Players::VisualMarkers::DrawBonePoints::Radius);
    gc("c_esp_bone",   Cheats::Players::VisualMarkers::DrawBonePoints::Color);
    gi("esp_dist",     Cheats::Players::Settings::MaxDistance);
    { bool b; gb("esp_noped",  b); Cheats::Players::Settings::IgnorePed  = (int)b; }
    { bool b; gb("esp_nodead", b); Cheats::Players::Settings::IgnoreDeath= (int)b; }
    gi("pi_maxdist",  Cheats::Players::PlayerInfo::GlobalSettings::MaxDistance);
    gb("pi_name",     Cheats::Players::PlayerInfo::DrawName::Enabled);
    gi("pi_name_loc", Cheats::Players::PlayerInfo::DrawName::SelectedLocation);
    gc("c_pi_name",   Cheats::Players::PlayerInfo::DrawName::Color);
    gb("pi_id",       Cheats::Players::PlayerInfo::DrawId::Enabled);
    gi("pi_id_loc",   Cheats::Players::PlayerInfo::DrawId::SelectedLocation);
    gc("c_pi_id",     Cheats::Players::PlayerInfo::DrawId::Color);
    gb("pi_dist",     Cheats::Players::PlayerInfo::DrawDistance::Enabled);
    gi("pi_dist_loc", Cheats::Players::PlayerInfo::DrawDistance::SelectedLocation);
    gc("c_pi_dist",   Cheats::Players::PlayerInfo::DrawDistance::Color);
    gb("pi_weapon",   Cheats::Players::PlayerInfo::DrawWeaponName::Enabled);
    gi("pi_wpn_loc",  Cheats::Players::PlayerInfo::DrawWeaponName::SelectedLocation);
    gc("c_pi_weapon", Cheats::Players::PlayerInfo::DrawWeaponName::Color);
    gb("sb_hp",        Cheats::Players::StatusBars::DrawHealthBar::Enabled);
    gi("sb_hp_loc",    Cheats::Players::StatusBars::DrawHealthBar::SelectedLocation);
    gb("sb_armor",     Cheats::Players::StatusBars::DrawArmorBar::Enabled);
    gi("sb_armor_loc", Cheats::Players::StatusBars::DrawArmorBar::SelectedLocation);
    gb("veh_pt",       Cheats::Vehicles::DrawPoint::Enabled);
    gc("c_veh_pt",     Cheats::Vehicles::DrawPoint::Color);
    gi("veh_pt_size",  Cheats::Vehicles::DrawPoint::Size);
    gb("veh_ln",       Cheats::Vehicles::DrawLine::Enabled);
    gc("c_veh_ln",     Cheats::Vehicles::DrawLine::Color);
    gi("veh_ln_loc",   Cheats::Vehicles::DrawLine::SelectedLocation);
    gb("veh_dist_en",  Cheats::Vehicles::DrawDistance::Enabled);
    gc("c_veh_dist",   Cheats::Vehicles::DrawDistance::Color);
    gb("veh_hp",       Cheats::Vehicles::DrawHealthBar::Enabled);
    gb("veh_ignlocal", Cheats::Vehicles::Settings::IgnoreLocalVehicle);
    gi("veh_maxcount", Cheats::Vehicles::Settings::MaxVehicleCount);
    gi("veh_maxdist",  Cheats::Vehicles::Settings::MaxDistance);
    gb("veh_fix",      Cheats::Vehicles::VehicleFix::Enabled);
    gi("veh_fix_hk",   Cheats::Vehicles::VehicleFix::HotKey);
    gb("w_noclip",    Cheats::World::NoClip::Enabled);
    gi("w_noclip_spd",Cheats::World::NoClip::MovementSpeed);
    gb("w_semigod",   Cheats::World::SemiGodMode::Enabled);
    gb("w_sprint",    Cheats::World::SuperSprint::Enabled);
    gi("w_sprintspd", Cheats::World::SuperSprint::Speed);
    gb("w_stamina",   Cheats::World::InfiniteStamina::Enabled);
    gb("w_explode",   Cheats::World::ExplosiveBullets::Enabled);
    gb("w_fire",      Cheats::World::FireBullets::Enabled);
    gi("s_panickey", Cheats::Settings::PanicKey);
    gi("s_menukey",  Cheats::Settings::MenuKey);
    gb("s_stream",   Cheats::Settings::StreamProof);
    gi("s_theme",    Cheats::Settings::SelectedTheme);
    gi("s_maxplrs",  Cheats::Settings::MaxPlayerCount);

    SaveConfig(CfgPath());
    return true;
}


static std::string GetMenuHtml() {
    static const std::string s =
    std::string(R"RAW1(<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><link rel="preconnect" href="https://fonts.googleapis.com"><link rel="preconnect" href="https://fonts.gstatic.com" crossorigin><link href="https://fonts.googleapis.com/css2?family=Syne:wght@400;700;800&family=DM+Mono:ital,wght@0,300;0,400;1,300&display=swap" rel="stylesheet"><title>FUD Private</title><style>
*{box-sizing:border-box;margin:0;padding:0}
:root{
  --bg:#0a0a0a;
  --panel:rgba(15,15,15,.72);
  --card:rgba(15,15,15,.72);
  --bdr:rgba(220,220,220,.10);
  --txt:#e0e0e0;
  --muted:#909090;
  --acc:#b8c8d8;
  --off:#3a3a3a;
  --green:#c0ccd8;
  --red:#c08888;
  --anim:cubic-bezier(.25,.46,.45,.94);
  --glow:0 0 10px rgba(210,225,240,0.55), 0 0 28px rgba(180,200,220,0.22)
}
#cur{position:fixed;top:0;left:0;width:9px;height:9px;pointer-events:none;z-index:9999;transform:translate(-50%,-50%);transition:transform .07s ease, width .07s ease, height .07s ease, opacity .07s ease;background:rgba(255,255,255,0.72);border-radius:50%;box-shadow:0 0 8px rgba(255,255,255,0.35)}
#cur.ck{transform:translate(-50%,-50%) scale(0.55);background:rgba(255,255,255,0.45)}
*{cursor:none !important}
body{background:var(--bg) url('https://i.imgur.com/j9r1zFu.png') center center / auto repeat fixed;color:var(--txt);font-family:'DM Mono',monospace;font-size:14px;min-height:100vh;text-shadow:var(--glow)}
header{background:var(--panel);border-bottom:1px solid rgba(200,200,200,.09);padding:14px 24px;display:flex;align-items:center;gap:12px;position:sticky;top:0;z-index:100;text-shadow:var(--glow)}
header h1{font-size:16px;font-weight:700;letter-spacing:.5px;text-shadow:var(--glow);display:flex;align-items:center;gap:8px}
header h1 .fud{color:var(--acc)}
header h1 .private{color:var(--green)}
.sts{margin-left:auto;display:flex;align-items:center;gap:8px;font-size:12px;color:var(--muted);text-shadow:var(--glow)}
.dot{width:8px;height:8px;border-radius:50%;background:var(--red);transition:.4s;text-shadow:none}
.dot.on{background:var(--green);animation:pulse 2s infinite;text-shadow:none}
@keyframes pulse{0%{box-shadow:0 0 0 0 rgba(180,210,220,.6)}70%{box-shadow:0 0 0 6px transparent}100%{box-shadow:0 0 0 0 transparent}}
.svd{color:var(--green);opacity:0;transition:opacity .3s;font-size:11px;text-shadow:var(--glow)}
.svd.show{opacity:1}
nav{background:var(--panel);border-bottom:1px solid var(--bdr);display:flex;padding:0 20px;position:sticky;top:53px;z-index:99;text-shadow:var(--glow)}
.tab{padding:12px 16px;cursor:pointer;border-bottom:2px solid transparent;color:var(--muted);font-size:13px;font-weight:500;transition:.15s;user-select:none;text-shadow:var(--glow)}
.tab:hover,.tab.active{color:var(--txt);text-shadow:var(--glow)}
.tab.active{border-bottom-color:var(--acc)}
main{padding:18px 24px;max-width:920px;margin:0 auto;text-shadow:var(--glow)}
.page{display:none;animation:fin .18s var(--anim)}
.page.active{display:block}
@keyframes fin{from{opacity:0;transform:translateY(4px)}to{opacity:1;transform:none}}
.stbs{display:flex;gap:3px;margin-bottom:14px;background:var(--panel);border:1px solid var(--bdr);border-radius:8px;padding:4px;text-shadow:var(--glow)}
.stb{flex:1;padding:7px 4px;border-radius:6px;cursor:pointer;font-size:12px;color:var(--muted);text-align:center;transition:.15s;user-select:none;text-shadow:var(--glow)}
.stb:hover{color:var(--txt);text-shadow:var(--glow)}
.stb.active{background:var(--card);color:var(--txt);box-shadow:0 2px 8px rgba(0,0,0,.4);text-shadow:var(--glow)}
.sp{display:none}.sp.active{display:block}
.sec{margin-bottom:14px;text-shadow:var(--glow)}
.st{font-size:10px;text-transform:uppercase;letter-spacing:1.4px;color:var(--muted);margin-bottom:8px;font-weight:600;text-shadow:var(--glow)}
.card{background:rgba(18,18,20,.38);border:1px solid rgba(200,210,220,.09);border-radius:16px;overflow:hidden;text-shadow:var(--glow);box-shadow:0 22px 60px rgba(0,0,0,.30), inset 0 1px 0 rgba(255,255,255,.05);backdrop-filter:blur(18px);-webkit-backdrop-filter:blur(18px);position:relative}
.card::before{content:'';position:absolute;inset:0;background:linear-gradient(180deg,rgba(200,210,220,.05),rgba(255,255,255,0));pointer-events:none}
.card > *{position:relative;z-index:1}
.row{display:flex;align-items:center;padding:10px 16px;border-bottom:1px solid rgba(200,210,220,.04);gap:10px;transition:background .1s,text-shadow .1s;text-shadow:var(--glow)}
.row:last-child{border-bottom:none}
.row:hover{background:rgba(200,210,220,.025)}
.row > label:not(.tog){flex:1;font-size:13px;text-shadow:var(--glow)}
.row label.tog{flex:none}
.tog{position:relative;width:22px;height:22px;flex-shrink:0;cursor:pointer;border:1px solid var(--bdr);background:transparent;transition:.2s;border-radius:6px;display:inline-flex;align-items:center;justify-content:center}
.tog input{opacity:0;width:100%;height:100%;position:absolute;top:0;left:0;margin:0;cursor:pointer}
.tog .tgl{position:absolute;inset:4px;background:transparent;border-radius:4px;transition:.2s;box-shadow:inset 0 0 0 1px rgba(200,210,220,.12)}
.tog input:checked + .tgl{background:var(--acc);box-shadow:inset 0 0 0 1px rgba(180,200,220,.3)}
.row.srow{display:grid;grid-template-columns:minmax(120px,180px) minmax(200px,1fr) auto;align-items:center;gap:12px}
.row.srow label{font-size:13px;color:var(--txt);white-space:nowrap}
.row.srow .v{font-size:12px;color:var(--acc);font-weight:600;text-align:right}
input[type=range]{width:100%;accent-color:var(--acc);cursor:pointer}
select{background:rgba(200,210,220,.04);border:1px solid rgba(200,210,220,.09);color:var(--txt);padding:8px 10px;border-radius:10px;font-size:12px;cursor:pointer;outline:none;text-shadow:var(--glow)}
select:focus{border-color:var(--acc)}
input[type=color]{width:32px;height:24px;border:1px solid rgba(200,210,220,.09);border-radius:8px;padding:2px;background:rgba(200,210,220,.05);cursor:pointer;flex-shrink:0}
.hk{min-width:72px;padding:4px 10px;border-radius:6px;background:var(--off);border:1px solid var(--bdr);color:var(--txt);font-size:11px;text-align:center;cursor:pointer;transition:.15s;user-select:none;text-shadow:var(--glow)}
.hk.lst{border-color:var(--acc);color:var(--acc);animation:blk .6s steps(1) infinite}
@keyframes blk{50%{opacity:.4}}
footer{text-align:center;padding:14px;color:var(--muted);font-size:11px;text-shadow:var(--glow)}
.kb-modal{display:none;position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,0.85);z-index:10000;justify-content:center;align-items:center;padding:20px;overflow-y:auto;overflow-x:hidden}
.kb-modal.active{display:flex}
.kb-content{background:var(--panel);border:1px solid var(--bdr);border-radius:12px;padding:18px;max-width:min(1200px,98vw);width:100%;text-shadow:var(--glow);box-sizing:border-box;overflow:hidden}
.kb-title{font-size:18px;margin-bottom:14px;text-shadow:var(--glow);letter-spacing:.4px}
.kb-row{display:flex;flex-wrap:nowrap;gap:6px;margin-bottom:8px;justify-content:flex-start;min-width:0}
.kb-panels{display:flex;gap:18px;flex-wrap:nowrap;justify-content:flex-start;align-items:flex-start}
.keyboard-panel{flex:1 1 820px;display:flex;flex-direction:column;gap:8px;max-width:none;overflow-x:hidden;min-width:820px}
.mouse-panel{flex:0 0 260px;display:flex;justify-content:center;align-items:flex-start}
.kb-key{background:linear-gradient(180deg,rgba(200,210,220,.07),rgba(0,0,0,.18));border:1px solid rgba(200,210,220,.09);border-radius:10px;padding:10px 12px;cursor:pointer;font-size:12px;text-align:center;min-width:34px;transition:.15s;text-shadow:var(--glow);white-space:nowrap;box-sizing:border-box;flex:0 0 auto;box-shadow:0 6px 12px rgba(0,0,0,.35), inset 0 1px 0 rgba(200,210,220,.07)}
.kb-key:hover{background:var(--acc);color:#000;text-shadow:none;border-color:rgba(180,200,220,.8)}
.kb-key.spacer{background:transparent;border:none;cursor:default;flex:0 0 20px}
.kb-key.wide{min-width:72px}.kb-key.tab{min-width:72px}.kb-key.caps{min-width:86px}.kb-key.enter{min-width:94px}.kb-key.shift{min-width:110px}.kb-key.ctrl{min-width:72px}.kb-key.alt{min-width:54px}.kb-key.space{min-width:240px}
.mouse-shell{position:relative;width:200px;height:320px;background:linear-gradient(180deg,#111418,#0a0e12);border:2px solid var(--bdr);border-radius:90px;box-shadow:0 0 0 8px rgba(100,120,140,.08), inset 0 10px 30px rgba(0,0,0,.55);padding:18px;box-sizing:border-box;overflow:hidden}
.mouse-shell::before{content:'';position:absolute;top:12px;left:50%;transform:translateX(-50%);width:70px;height:90px;background:rgba(200,210,220,.04);border-radius:50%}
.mouse-label{position:absolute;bottom:16px;left:50%;transform:translateX(-50%);font-size:12px;color:var(--muted);letter-spacing:.4px;text-transform:uppercase}
.mouse-button{position:absolute;display:flex;align-items:center;justify-content:center;border-radius:14px;background:var(--card);border:1px solid rgba(200,210,220,.09);color:var(--txt);padding:8px 10px;cursor:pointer;font-size:12px;min-width:44px;min-height:38px;box-shadow:0 8px 18px rgba(0,0,0,.4);transition:.15s;text-shadow:var(--glow)}
.mouse-button:hover{background:var(--acc);color:#000;border-color:rgba(180,200,220,.8)}
.mouse-left{top:26px;left:16px;width:58px;height:100px;border-radius:32px}.mouse-right{top:26px;right:16px;width:58px;height:100px;border-radius:32px}
.mouse-center{top:50%;left:50%;transform:translate(-50%,-50%);width:54px;height:54px;border-radius:50%}
.mouse-back{bottom:58px;left:20px;width:52px;height:38px;border-radius:12px}.mouse-forward{bottom:58px;right:20px;width:52px;height:38px;border-radius:12px}
.kb-content .kb-row:last-child{margin-bottom:0}
)RAW1")
    + R"RAW1b(</style></head><body>
<div id="cur"></div>
<header>
  <h1><span class="fud">FUD</span>&nbsp;<span class="private">Private</span></h1>
  <div class="sts"><div class="dot" id="dot"></div><span id="stlbl">Connecting...</span><span class="svd" id="svd">&#10003; Saved</span></div>
</header>
<nav>
<div class="tab active" onclick="sw('aim')">Aim</div>
<div class="tab" onclick="sw('esp')">ESP</div>
<div class="tab" onclick="sw('veh')">Vehicles</div>
<div class="tab" onclick="sw('wld')">World</div>
<div class="tab" onclick="sw('cfg')">Settings</div>
</nav>
<main>
<div class="page active" id="page-aim">
<div class="stbs">
<div class="stb active" onclick="sst('aim','sa',this)">Silent</div>
<div class="stb" onclick="sst('aim','mb',this)">Magic Bullet</div>
<div class="stb" onclick="sst('aim','ab',this)">Aimbot</div>
<div class="stb" onclick="sst('aim','tb',this)">Triggerbot</div>
<div class="stb" onclick="sst('aim','wp',this)">Weapon</div>
</div>
<div class="sp active" id="aim-sa">
<div class="sec"><div class="st">Silent Aim</div><div class="card">
<div class="row"><label>Enable</label><label class="tog"><input type="checkbox" id="silent_en" onchange="s('silent_en',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>Hotkey</label><button class="hk" id="silent_hk_btn" onclick="openKb('silent_hk','silent_hk_btn')">None</button></div>
<div class="row srow"><label>FOV</label><input type="range" min="10" max="360" id="silent_fov" oninput="sv('silent_fov_v',this.value)" onchange="s('silent_fov',+this.value)"><span class="v" id="silent_fov_v">150</span></div>
<div class="row srow"><label>Max Distance</label><input type="range" min="10" max="600" id="silent_dist" oninput="sv('silent_dist_v',this.value)" onchange="s('silent_dist',+this.value)"><span class="v" id="silent_dist_v">300</span></div>
<div class="row srow"><label>Miss Chance %</label><input type="range" min="0" max="100" id="silent_miss" oninput="sv('silent_miss_v',this.value)" onchange="s('silent_miss',+this.value)"><span class="v" id="silent_miss_v">0</span></div>
<div class="row"><label>Bone</label><select id="silent_bone" onchange="s('silent_bone',+this.value)"><option value="0">Head</option><option value="1">Chest</option><option value="2">Legs</option><option value="3">Random</option></select></div>
<div class="row"><label>Draw FOV Circle</label><label class="tog"><input type="checkbox" id="silent_fov_draw" onchange="s('silent_fov_draw',this.checked)"><span class="tgl"></span></label><input type="color" id="c_silent_fov" onchange="s('c_silent_fov',this.value)"></div>
<div class="row srow"><label>FOV Weight</label><input type="range" min="1" max="5" id="silent_fov_w" oninput="sv('silent_fov_w_v',this.value)" onchange="s('silent_fov_w',+this.value)"><span class="v" id="silent_fov_w_v">1</span></div>
<div class="row"><label>Visibility Check</label><label class="tog"><input type="checkbox" id="silent_vis" onchange="s('silent_vis',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>Skip Friends</label><label class="tog"><input type="checkbox" id="silent_skip" onchange="s('silent_skip',this.checked)"><span class="tgl"></span></label></div>
</div></div>
</div>
<div class="sp" id="aim-mb">
<div class="sec"><div class="st">Magic Bullet</div><div class="card">
<div class="row"><label>Enable</label><label class="tog"><input type="checkbox" id="mb_en" onchange="s('mb_en',this.checked)"><span class="tgl"></span></label></div>
<div class="row srow"><label>Max Distance</label><input type="range" min="10" max="1000" id="mb_dist" oninput="sv('mb_dist_v',this.value)" onchange="s('mb_dist',+this.value)"><span class="v" id="mb_dist_v">600</span></div>
<div class="row"><label>Bone</label><select id="mb_bone" onchange="s('mb_bone',+this.value)"><option value="0">Head</option><option value="1">Chest</option><option value="2">Legs</option><option value="3">Random</option></select></div>
<div class="row"><label>Skip Friends</label><label class="tog"><input type="checkbox" id="mb_skipfr" onchange="s('mb_skipfr',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>Skip NPC</label><label class="tog"><input type="checkbox" id="mb_skipnpc" onchange="s('mb_skipnpc',this.checked)"><span class="tgl"></span></label></div>
</div></div>
</div>
<div class="sp" id="aim-ab">
<div class="sec"><div class="st">Aimbot</div><div class="card">
<div class="row"><label>Enable</label><label class="tog"><input type="checkbox" id="aim_en" onchange="s('aim_en',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>Hotkey</label><button class="hk" id="aim_hk_btn" onclick="openKb('aim_hk','aim_hk_btn')">None</button></div>
<div class="row srow"><label>FOV</label><input type="range" min="10" max="360" id="aim_fov" oninput="sv('aim_fov_v',this.value)" onchange="s('aim_fov',+this.value)"><span class="v" id="aim_fov_v">150</span></div>
<div class="row srow"><label>Smooth</label><input type="range" min="1" max="20" id="aim_smooth" oninput="sv('aim_smooth_v',this.value)" onchange="s('aim_smooth',+this.value)"><span class="v" id="aim_smooth_v">5</span></div>
<div class="row srow"><label>Max Distance</label><input type="range" min="10" max="600" id="aim_dist" oninput="sv('aim_dist_v',this.value)" onchange="s('aim_dist',+this.value)"><span class="v" id="aim_dist_v">300</span></div>
<div class="row"><label>Bone</label><select id="aim_bone" onchange="s('aim_bone',+this.value)"><option value="0">Head</option><option value="1">Chest</option><option value="2">Legs</option><option value="3">Random</option></select></div>
<div class="row"><label>Aim Mode</label><select id="aim_mode" onchange="s('aim_mode',+this.value)"><option value="0">Memory Write</option><option value="1">Mouse Move</option></select></div>
<div class="row"><label>Priority</label><select id="aim_prio" onchange="s('aim_prio',+this.value)"><option value="0">Nearest</option><option value="1">FOV Center</option><option value="2">Lowest HP</option></select></div>
<div class="row"><label>Draw FOV Circle</label><label class="tog"><input type="checkbox" id="aim_fov_draw" onchange="s('aim_fov_draw',this.checked)"><span class="tgl"></span></label><input type="color" id="c_aim_fov" onchange="s('c_aim_fov',this.value)"></div>
<div class="row srow"><label>FOV Weight</label><input type="range" min="1" max="5" id="aim_fov_w" oninput="sv('aim_fov_w_v',this.value)" onchange="s('aim_fov_w',+this.value)"><span class="v" id="aim_fov_w_v">1</span></div>
<div class="row"><label>Sticky Aim</label><label class="tog"><input type="checkbox" id="aim_sticky" onchange="s('aim_sticky',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>Visibility Check</label><label class="tog"><input type="checkbox" id="aim_vis" onchange="s('aim_vis',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>Skip Friends</label><label class="tog"><input type="checkbox" id="aim_skip" onchange="s('aim_skip',this.checked)"><span class="tgl"></span></label></div>
</div></div>
</div>
<div class="sp" id="aim-tb">
<div class="sec"><div class="st">Triggerbot</div><div class="card">
<div class="row"><label>Enable</label><label class="tog"><input type="checkbox" id="trig_en" onchange="s('trig_en',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>Hotkey</label><button class="hk" id="trig_hk_btn" onclick="openKb('trig_hk','trig_hk_btn')">None</button></div>
<div class="row srow"><label>Delay (ms)</label><input type="range" min="0" max="500" id="trig_delay" oninput="sv('trig_delay_v',this.value)" onchange="s('trig_delay',+this.value)"><span class="v" id="trig_delay_v">5</span></div>
<div class="row srow"><label>Max Distance</label><input type="range" min="10" max="600" id="trig_dist" oninput="sv('trig_dist_v',this.value)" onchange="s('trig_dist',+this.value)"><span class="v" id="trig_dist_v">300</span></div>
<div class="row"><label>Skip Friends</label><label class="tog"><input type="checkbox" id="trig_skip" onchange="s('trig_skip',this.checked)"><span class="tgl"></span></label></div>
</div></div>
</div>
<div class="sp" id="aim-wp">
<div class="sec"><div class="st">Weapon Settings</div><div class="card">
<div class="row"><label>Infinite Ammo</label><label class="tog"><input type="checkbox" id="wpn_ammo" onchange="s('wpn_ammo',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>No Recoil</label><label class="tog"><input type="checkbox" id="wpn_recoil" onchange="s('wpn_recoil',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>No Spread</label><label class="tog"><input type="checkbox" id="wpn_spread" onchange="s('wpn_spread',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>No Reload</label><label class="tog"><input type="checkbox" id="wpn_reload" onchange="s('wpn_reload',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>Rapid Fire</label><label class="tog"><input type="checkbox" id="wpn_rapid" onchange="s('wpn_rapid',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>Crosshair</label><label class="tog"><input type="checkbox" id="wpn_crosshair" onchange="s('wpn_crosshair',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>Crosshair Type</label><select id="wpn_xtype" onchange="s('wpn_xtype',+this.value)"><option value="0">Type 1</option><option value="1">Type 2</option><option value="2">Type 3</option><option value="3">Type 4</option><option value="4">Type 5</option></select></div>
<div class="row srow"><label>Crosshair Size</label><input type="range" min="5" max="50" id="wpn_xsize" oninput="sv('wpn_xsize_v',this.value)" onchange="s('wpn_xsize',+this.value)"><span class="v" id="wpn_xsize_v">15</span></div>
</div></div>
</div>
</div>
)RAW1b"
    + R"RAW2(<div class="page" id="page-esp">
<div class="stbs">
<div class="stb active" onclick="sst('esp','vm',this)">Visual</div>
<div class="stb" onclick="sst('esp','pi',this)">Player Info</div>
<div class="stb" onclick="sst('esp','ft',this)">Filters</div>
</div>
<div class="sp active" id="esp-vm">
<div class="sec"><div class="st">Visual Markers</div><div class="card">
<div class="row"><label>Vis Check (wall)</label><label class="tog"><input type="checkbox" id="esp_vis" onchange="s('esp_vis',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>Visible Color</label><input type="color" id="c_esp_vis" onchange="s('c_esp_vis',this.value)"></div>
<div class="row"><label>Hidden Color</label><input type="color" id="c_esp_hid" onchange="s('c_esp_hid',this.value)"></div>
<div class="row"><label>Line Style</label><select id="esp_linetype" onchange="s('esp_linetype',+this.value)"><option value="0">Basic</option><option value="1">Outlined</option></select></div>
<div class="row srow"><label>Line Weight</label><input type="range" min="1" max="6" id="esp_linew" oninput="sv('esp_linew_v',this.value)" onchange="s('esp_linew',+this.value)"><span class="v" id="esp_linew_v">2</span></div>
</div></div>
<div class="sec"><div class="st">Skeleton</div><div class="card">
<div class="row"><label>Draw Skeleton</label><label class="tog"><input type="checkbox" id="esp_skel" onchange="s('esp_skel',this.checked)"><span class="tgl"></span></label><input type="color" id="c_esp_skel" onchange="s('c_esp_skel',this.value)"></div>
</div></div>
<div class="sec"><div class="st">Box</div><div class="card">
<div class="row"><label>Draw Box</label><label class="tog"><input type="checkbox" id="esp_box" onchange="s('esp_box',this.checked)"><span class="tgl"></span></label><input type="color" id="c_esp_box" onchange="s('c_esp_box',this.value)"></div>
<div class="row"><label>Box Type</label><select id="esp_boxtype" onchange="s('esp_boxtype',+this.value)"><option value="0">Corner</option><option value="1">2D</option><option value="2">3D</option></select></div>
</div></div>
<div class="sec"><div class="st">Line</div><div class="card">
<div class="row"><label>Draw Line</label><label class="tog"><input type="checkbox" id="esp_line" onchange="s('esp_line',this.checked)"><span class="tgl"></span></label><input type="color" id="c_esp_line" onchange="s('c_esp_line',this.value)"></div>
<div class="row"><label>Line Origin</label><select id="esp_lineloc" onchange="s('esp_lineloc',+this.value)"><option value="0">Top</option><option value="1">Center</option><option value="2">Bottom</option></select></div>
</div></div>
<div class="sec"><div class="st">Bone Points</div><div class="card">
<div class="row"><label>Draw Bones</label><label class="tog"><input type="checkbox" id="esp_bone" onchange="s('esp_bone',this.checked)"><span class="tgl"></span></label><input type="color" id="c_esp_bone" onchange="s('c_esp_bone',this.value)"></div>
<div class="row srow"><label>Bone Radius</label><input type="range" min="1" max="10" id="esp_bone_r" oninput="sv('esp_bone_r_v',this.value)" onchange="s('esp_bone_r',+this.value)"><span class="v" id="esp_bone_r_v">3</span></div>
</div></div>
</div>
<div class="sp" id="esp-pi">
<div class="sec"><div class="st">Player Info</div><div class="card">
<div class="row srow"><label>Max Display Distance</label><input type="range" min="10" max="300" id="pi_maxdist" oninput="sv('pi_maxdist_v',this.value)" onchange="s('pi_maxdist',+this.value)"><span class="v" id="pi_maxdist_v">70</span></div>
<div class="row"><label>Name</label><label class="tog"><input type="checkbox" id="pi_name" onchange="s('pi_name',this.checked)"><span class="tgl"></span></label><select id="pi_name_loc" onchange="s('pi_name_loc',+this.value)"><option value="0">Top</option><option value="1">Bottom</option></select><input type="color" id="c_pi_name" onchange="s('c_pi_name',this.value)"></div>
<div class="row"><label>Player ID</label><label class="tog"><input type="checkbox" id="pi_id" onchange="s('pi_id',this.checked)"><span class="tgl"></span></label><select id="pi_id_loc" onchange="s('pi_id_loc',+this.value)"><option value="0">Top</option><option value="1">Bottom</option></select><input type="color" id="c_pi_id" onchange="s('c_pi_id',this.value)"></div>
<div class="row"><label>Distance</label><label class="tog"><input type="checkbox" id="pi_dist" onchange="s('pi_dist',this.checked)"><span class="tgl"></span></label><select id="pi_dist_loc" onchange="s('pi_dist_loc',+this.value)"><option value="0">Top</option><option value="1">Bottom</option></select><input type="color" id="c_pi_dist" onchange="s('c_pi_dist',this.value)"></div>
<div class="row"><label>Weapon Name</label><label class="tog"><input type="checkbox" id="pi_weapon" onchange="s('pi_weapon',this.checked)"><span class="tgl"></span></label><select id="pi_wpn_loc" onchange="s('pi_wpn_loc',+this.value)"><option value="0">Top</option><option value="1">Bottom</option></select><input type="color" id="c_pi_weapon" onchange="s('c_pi_weapon',this.value)"></div>
</div></div>
<div class="sec"><div class="st">Status Bars</div><div class="card">
<div class="row"><label>Health Bar</label><label class="tog"><input type="checkbox" id="sb_hp" onchange="s('sb_hp',this.checked)"><span class="tgl"></span></label><select id="sb_hp_loc" onchange="s('sb_hp_loc',+this.value)"><option value="0">Left</option><option value="1">Right</option></select></div>
<div class="row"><label>Armor Bar</label><label class="tog"><input type="checkbox" id="sb_armor" onchange="s('sb_armor',this.checked)"><span class="tgl"></span></label><select id="sb_armor_loc" onchange="s('sb_armor_loc',+this.value)"><option value="0">Left</option><option value="1">Right</option></select></div>
</div></div>
</div>
<div class="sp" id="esp-ft">
<div class="sec"><div class="st">Filters</div><div class="card">
<div class="row srow"><label>Max Distance</label><input type="range" min="10" max="800" id="esp_dist" oninput="sv('esp_dist_v',this.value)" onchange="s('esp_dist',+this.value)"><span class="v" id="esp_dist_v">300</span></div>
<div class="row"><label>Hide NPC</label><label class="tog"><input type="checkbox" id="esp_noped" onchange="s('esp_noped',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>Hide Dead</label><label class="tog"><input type="checkbox" id="esp_nodead" onchange="s('esp_nodead',this.checked)"><span class="tgl"></span></label></div>
</div></div>
</div>
</div>
<div class="page" id="page-veh">
<div class="sec"><div class="st">Vehicle ESP</div><div class="card">
<div class="row"><label>Draw Point</label><label class="tog"><input type="checkbox" id="veh_pt" onchange="s('veh_pt',this.checked)"><span class="tgl"></span></label><input type="color" id="c_veh_pt" onchange="s('c_veh_pt',this.value)"></div>
<div class="row srow"><label>Point Size</label><input type="range" min="2" max="20" id="veh_pt_size" oninput="sv('veh_pt_size_v',this.value)" onchange="s('veh_pt_size',+this.value)"><span class="v" id="veh_pt_size_v">7</span></div>
<div class="row"><label>Draw Line</label><label class="tog"><input type="checkbox" id="veh_ln" onchange="s('veh_ln',this.checked)"><span class="tgl"></span></label><input type="color" id="c_veh_ln" onchange="s('c_veh_ln',this.value)"></div>
<div class="row"><label>Line Origin</label><select id="veh_ln_loc" onchange="s('veh_ln_loc',+this.value)"><option value="0">Top</option><option value="1">Center</option><option value="2">Bottom</option></select></div>
<div class="row"><label>Draw Distance</label><label class="tog"><input type="checkbox" id="veh_dist_en" onchange="s('veh_dist_en',this.checked)"><span class="tgl"></span></label><input type="color" id="c_veh_dist" onchange="s('c_veh_dist',this.value)"></div>
<div class="row"><label>Health Bar</label><label class="tog"><input type="checkbox" id="veh_hp" onchange="s('veh_hp',this.checked)"><span class="tgl"></span></label></div>
</div></div>
<div class="sec"><div class="st">Filters</div><div class="card">
<div class="row"><label>Ignore Local Vehicle</label><label class="tog"><input type="checkbox" id="veh_ignlocal" onchange="s('veh_ignlocal',this.checked)"><span class="tgl"></span></label></div>
<div class="row srow"><label>Max Count</label><input type="range" min="10" max="1000" id="veh_maxcount" oninput="sv('veh_maxcount_v',this.value)" onchange="s('veh_maxcount',+this.value)"><span class="v" id="veh_maxcount_v">500</span></div>
<div class="row srow"><label>Max Distance</label><input type="range" min="10" max="800" id="veh_maxdist" oninput="sv('veh_maxdist_v',this.value)" onchange="s('veh_maxdist',+this.value)"><span class="v" id="veh_maxdist_v">300</span></div>
</div></div>
<div class="sec"><div class="st">Vehicle Fix</div><div class="card">
<div class="row"><label>Auto Repair</label><label class="tog"><input type="checkbox" id="veh_fix" onchange="s('veh_fix',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>Repair Hotkey</label><button class="hk" id="veh_fix_hk_btn" onclick="openKb('veh_fix_hk','veh_fix_hk_btn')">None</button></div>
</div></div>
</div>
)RAW2"
    + R"RAW3(<div class="page" id="page-wld">
<div class="sec"><div class="st">NoClip</div><div class="card">
<div class="row"><label>Enable NoClip</label><label class="tog"><input type="checkbox" id="w_noclip" onchange="s('w_noclip',this.checked)"><span class="tgl"></span></label></div>
<div class="row srow"><label>Speed</label><input type="range" min="1" max="30" id="w_noclip_spd" oninput="sv('w_noclip_spd_v',this.value)" onchange="s('w_noclip_spd',+this.value)"><span class="v" id="w_noclip_spd_v">5</span></div>
</div></div>
<div class="sec"><div class="st">World Cheats</div><div class="card">
<div class="row"><label>Semi God Mode</label><label class="tog"><input type="checkbox" id="w_semigod" onchange="s('w_semigod',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>Super Sprint</label><label class="tog"><input type="checkbox" id="w_sprint" onchange="s('w_sprint',this.checked)"><span class="tgl"></span></label></div>
<div class="row srow"><label>Sprint Speed</label><input type="range" min="1" max="20" id="w_sprintspd" oninput="sv('w_sprintspd_v',this.value)" onchange="s('w_sprintspd',+this.value)"><span class="v" id="w_sprintspd_v">5</span></div>
<div class="row"><label>Infinite Stamina</label><label class="tog"><input type="checkbox" id="w_stamina" onchange="s('w_stamina',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>Explosive Bullets</label><label class="tog"><input type="checkbox" id="w_explode" onchange="s('w_explode',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>Fire Bullets</label><label class="tog"><input type="checkbox" id="w_fire" onchange="s('w_fire',this.checked)"><span class="tgl"></span></label></div>
</div></div>
</div>
<div class="page" id="page-cfg">
<div class="sec"><div class="st">General</div><div class="card">
<div class="row"><label>Stream Proof</label><label class="tog"><input type="checkbox" id="s_stream" onchange="s('s_stream',this.checked)"><span class="tgl"></span></label></div>
<div class="row"><label>Theme</label><select id="s_theme" onchange="s('s_theme',+this.value)"><option value="0">Emerald</option><option value="1">Ocean</option><option value="2">Rose</option></select></div>
<div class="row srow"><label>Max Players Scan</label><input type="range" min="10" max="1000" id="s_maxplrs" oninput="sv('s_maxplrs_v',this.value)" onchange="s('s_maxplrs',+this.value)"><span class="v" id="s_maxplrs_v">500</span></div>
<div class="row"><label>Menu Hotkey</label><button class="hk" id="s_menukey_btn" onclick="openKb('s_menukey','s_menukey_btn')">Insert</button></div>
<div class="row"><label>Panic Hotkey</label><button class="hk" id="s_panickey_btn" onclick="openKb('s_panickey','s_panickey_btn')">F12</button></div>
</div></div>
<div class="sec"><div class="st">Info</div><div class="card">
<div class="row" style="flex-direction:column;align-items:flex-start;gap:8px;padding:14px 16px">
  <span style="font-size:10px;text-transform:uppercase;letter-spacing:1.2px;color:var(--muted)">Panel Adresi (Telefon)</span>
  <div style="display:flex;align-items:center;gap:10px;width:100%">
    <span id="portlbl" style="font-size:15px;font-weight:600;color:var(--acc);letter-spacing:.5px;flex:1">...</span>
    <button onclick="cpAddr()" id="cpbtn" style="padding:5px 14px;border-radius:6px;background:var(--off);border:1px solid var(--bdr);color:var(--txt);font-size:11px;cursor:pointer;transition:.15s">Kopyala</button>
  </div>
  <span style="font-size:11px;color:var(--muted)">Telefon ve PC ayn&#305; WiFi&#8217;da olmal&#305;</span>
</div>
<div class="row"><label>Game Version</label><span id="gver" style="color:var(--muted);font-size:12px">-</span></div>
</div></div>
</div>
</main>
<footer>FUD Private &mdash; All changes auto-saved</footer>
<div class="kb-modal" id="kb-modal"><div class="kb-content"><div class="kb-title">Select Key Bind</div><div class="kb-panels"><div class="keyboard-panel"><div class="kb-row"><div class="kb-key" data-key="27">Esc</div><div class="kb-key" data-key="112">F1</div><div class="kb-key" data-key="113">F2</div><div class="kb-key" data-key="114">F3</div><div class="kb-key" data-key="115">F4</div><div class="kb-key" data-key="116">F5</div><div class="kb-key" data-key="117">F6</div><div class="kb-key" data-key="118">F7</div><div class="kb-key" data-key="119">F8</div><div class="kb-key" data-key="120">F9</div><div class="kb-key" data-key="121">F10</div><div class="kb-key" data-key="122">F11</div><div class="kb-key" data-key="123">F12</div></div><div class="kb-row"><div class="kb-key wide" data-key="192">`</div><div class="kb-key" data-key="49">1</div><div class="kb-key" data-key="50">2</div><div class="kb-key" data-key="51">3</div><div class="kb-key" data-key="52">4</div><div class="kb-key" data-key="53">5</div><div class="kb-key" data-key="54">6</div><div class="kb-key" data-key="55">7</div><div class="kb-key" data-key="56">8</div><div class="kb-key" data-key="57">9</div><div class="kb-key" data-key="48">0</div><div class="kb-key" data-key="189">-</div><div class="kb-key" data-key="187">=</div><div class="kb-key backspace" data-key="8">Bksp</div></div><div class="kb-row"><div class="kb-key tab wide" data-key="9">Tab</div><div class="kb-key" data-key="81">Q</div><div class="kb-key" data-key="87">W</div><div class="kb-key" data-key="69">E</div><div class="kb-key" data-key="82">R</div><div class="kb-key" data-key="84">T</div><div class="kb-key" data-key="89">Y</div><div class="kb-key" data-key="85">U</div><div class="kb-key" data-key="73">I</div><div class="kb-key" data-key="79">O</div><div class="kb-key" data-key="80">P</div><div class="kb-key" data-key="219">[</div><div class="kb-key" data-key="221">]</div><div class="kb-key" data-key="220">\</div></div><div class="kb-row"><div class="kb-key caps wide" data-key="20">Caps</div><div class="kb-key" data-key="65">A</div><div class="kb-key" data-key="83">S</div><div class="kb-key" data-key="68">D</div><div class="kb-key" data-key="70">F</div><div class="kb-key" data-key="71">G</div><div class="kb-key" data-key="72">H</div><div class="kb-key" data-key="74">J</div><div class="kb-key" data-key="75">K</div><div class="kb-key" data-key="76">L</div><div class="kb-key" data-key="186">;</div><div class="kb-key" data-key="222">'</div><div class="kb-key enter wide" data-key="13">Enter</div></div><div class="kb-row"><div class="kb-key shift wide" data-key="16">Shift</div><div class="kb-key" data-key="90">Z</div><div class="kb-key" data-key="88">X</div><div class="kb-key" data-key="67">C</div><div class="kb-key" data-key="86">V</div><div class="kb-key" data-key="66">B</div><div class="kb-key" data-key="78">N</div><div class="kb-key" data-key="77">M</div><div class="kb-key" data-key="188">,</div><div class="kb-key" data-key="190">.</div><div class="kb-key" data-key="191">/</div><div class="kb-key shift wide" data-key="16">Shift</div></div><div class="kb-row"><div class="kb-key ctrl wide" data-key="17">Ctrl</div><div class="kb-key alt" data-key="18">Alt</div><div class="kb-key space" data-key="32">Space</div><div class="kb-key alt" data-key="18">Alt</div><div class="kb-key ctrl wide" data-key="17">Ctrl</div></div></div><div class="mouse-panel"><div class="mouse-shell"><div class="mouse-button mouse-left" data-key="1">LMB</div><div class="mouse-button mouse-right" data-key="2">RMB</div><div class="mouse-button mouse-center" data-key="4">MMB</div><div class="mouse-button mouse-back" data-key="5">X1</div><div class="mouse-button mouse-forward" data-key="6">X2</div><div class="mouse-label">Mouse</div></div></div></div></div></div>
<script>
(function(){
  const cur=document.getElementById('cur');
  document.addEventListener('mousemove',e=>{cur.style.left=e.clientX+'px';cur.style.top=e.clientY+'px';});
  document.addEventListener('mousedown',()=>cur.classList.add('ck'));
  document.addEventListener('mouseup',()=>cur.classList.remove('ck'));
})();
function sw(id){
  const ids=['aim','esp','veh','wld','cfg'];
  document.querySelectorAll('.tab').forEach((t,i)=>t.classList.toggle('active',ids[i]===id));
  document.querySelectorAll('.page').forEach(p=>p.classList.toggle('active',p.id==='page-'+id));
}
function sst(pg,id,el){
  const cont=document.getElementById('page-'+pg);
  cont.querySelectorAll('.stb').forEach(t=>t.classList.remove('active'));
  cont.querySelectorAll('.sp').forEach(p=>p.classList.remove('active'));
  if(el)el.classList.add('active');
  const sp=document.getElementById(pg+'-'+id);if(sp)sp.classList.add('active');
}
function sv(id,v){const e=document.getElementById(id);if(e)e.textContent=v;}
let _st;
async function s(k,v){
  const b={};b[k]=v;
  await fetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b)});
  const el=document.getElementById('svd');el.classList.add('show');clearTimeout(_st);_st=setTimeout(()=>el.classList.remove('show'),1500);
}
function vkName(c){
  const n={0:'None',1:'LMB',2:'RMB',4:'MMB',5:'X1MB',6:'X2MB',8:'Backspace',9:'Tab',13:'Enter',16:'Shift',17:'Ctrl',18:'Alt',19:'Pause',20:'CapsLock',27:'Esc',32:'Space',33:'PgUp',34:'PgDn',35:'End',36:'Home',37:'Left',38:'Up',39:'Right',40:'Down',44:'PrtSc',45:'Insert',46:'Delete',112:'F1',113:'F2',114:'F3',115:'F4',116:'F5',117:'F6',118:'F7',119:'F8',120:'F9',121:'F10',122:'F11',123:'F12',160:'LShift',161:'RShift',162:'LCtrl',163:'RCtrl',164:'LAlt',165:'RAlt'};
  if(n[c])return n[c];
  if(c>=65&&c<=90)return String.fromCharCode(c);
  if(c>=48&&c<=57)return String.fromCharCode(c);
  if(c>=96&&c<=105)return 'Num'+(c-96);
  return 'VK'+c;
}
let _lkH=null,_lkT=null,_lkId=null,_lkBtn=null;
function lk(id,btnId){
  const btn=document.getElementById(btnId);
  if(_lkH){document.removeEventListener('keydown',_lkH,true);clearTimeout(_lkT);if(_lkBtn)_lkBtn.classList.remove('lst');}
  _lkId=id;_lkBtn=btn;btn.textContent='Press key...';btn.classList.add('lst');
  _lkH=function(e){e.preventDefault();const vk=e.keyCode;btn.textContent=vkName(vk);btn.classList.remove('lst');s(id,vk);document.removeEventListener('keydown',_lkH,true);_lkH=null;clearTimeout(_lkT);};
  document.addEventListener('keydown',_lkH,true);
  _lkT=setTimeout(()=>{if(_lkH){document.removeEventListener('keydown',_lkH,true);_lkH=null;btn.classList.remove('lst');}},5000);
}
function cp(id,v){const e=document.getElementById(id);if(e)e.value=v;}
function applyUI(d){
  const cb=(id,v)=>{const e=document.getElementById(id);if(e)e.checked=!!v;};
  const sl=(id,v)=>{const e=document.getElementById(id);if(e){e.value=v;const l=document.getElementById(id+'_v');if(l)l.textContent=v;}};
  const se=(id,v)=>{const e=document.getElementById(id);if(e)e.value=v;};
  const hk=(btnId,v)=>{const e=document.getElementById(btnId);if(e)e.textContent=vkName(v);};
  cb('silent_en',d.silent_en);hk('silent_hk_btn',d.silent_hk);sl('silent_fov',d.silent_fov);sl('silent_dist',d.silent_dist);sl('silent_miss',d.silent_miss);se('silent_bone',d.silent_bone);cb('silent_fov_draw',d.silent_fov_draw);sl('silent_fov_w',d.silent_fov_w);cp('c_silent_fov',d.c_silent_fov);cb('silent_vis',d.silent_vis);cb('silent_skip',d.silent_skip);
  cb('mb_en',d.mb_en);sl('mb_dist',d.mb_dist);se('mb_bone',d.mb_bone);cb('mb_skipfr',d.mb_skipfr);cb('mb_skipnpc',d.mb_skipnpc);
  cb('aim_en',d.aim_en);hk('aim_hk_btn',d.aim_hk);sl('aim_fov',d.aim_fov);sl('aim_smooth',d.aim_smooth);sl('aim_dist',d.aim_dist);se('aim_bone',d.aim_bone);se('aim_mode',d.aim_mode);se('aim_prio',d.aim_prio);cb('aim_fov_draw',d.aim_fov_draw);sl('aim_fov_w',d.aim_fov_w);cp('c_aim_fov',d.c_aim_fov);cb('aim_sticky',d.aim_sticky);cb('aim_vis',d.aim_vis);cb('aim_skip',d.aim_skip);
  cb('trig_en',d.trig_en);hk('trig_hk_btn',d.trig_hk);sl('trig_delay',d.trig_delay);sl('trig_dist',d.trig_dist);cb('trig_skip',d.trig_skip);
  cb('wpn_ammo',d.wpn_ammo);cb('wpn_recoil',d.wpn_recoil);cb('wpn_spread',d.wpn_spread);cb('wpn_reload',d.wpn_reload);cb('wpn_rapid',d.wpn_rapid);cb('wpn_crosshair',d.wpn_crosshair);se('wpn_xtype',d.wpn_xtype);sl('wpn_xsize',d.wpn_xsize);
  cb('esp_vis',d.esp_vis);cp('c_esp_vis',d.c_esp_vis);cp('c_esp_hid',d.c_esp_hid);se('esp_linetype',d.esp_linetype);sl('esp_linew',d.esp_linew);
  cb('esp_skel',d.esp_skel);cp('c_esp_skel',d.c_esp_skel);cb('esp_box',d.esp_box);se('esp_boxtype',d.esp_boxtype);cp('c_esp_box',d.c_esp_box);
  cb('esp_line',d.esp_line);se('esp_lineloc',d.esp_lineloc);cp('c_esp_line',d.c_esp_line);cb('esp_bone',d.esp_bone);sl('esp_bone_r',d.esp_bone_r);cp('c_esp_bone',d.c_esp_bone);
  sl('esp_dist',d.esp_dist);cb('esp_noped',d.esp_noped);cb('esp_nodead',d.esp_nodead);
  sl('pi_maxdist',d.pi_maxdist);cb('pi_name',d.pi_name);se('pi_name_loc',d.pi_name_loc);cp('c_pi_name',d.c_pi_name);cb('pi_id',d.pi_id);se('pi_id_loc',d.pi_id_loc);cp('c_pi_id',d.c_pi_id);cb('pi_dist',d.pi_dist);se('pi_dist_loc',d.pi_dist_loc);cp('c_pi_dist',d.c_pi_dist);cb('pi_weapon',d.pi_weapon);se('pi_wpn_loc',d.pi_wpn_loc);cp('c_pi_weapon',d.c_pi_weapon);
  cb('sb_hp',d.sb_hp);se('sb_hp_loc',d.sb_hp_loc);cb('sb_armor',d.sb_armor);se('sb_armor_loc',d.sb_armor_loc);
  cb('veh_pt',d.veh_pt);cp('c_veh_pt',d.c_veh_pt);sl('veh_pt_size',d.veh_pt_size);cb('veh_ln',d.veh_ln);cp('c_veh_ln',d.c_veh_ln);se('veh_ln_loc',d.veh_ln_loc);cb('veh_dist_en',d.veh_dist_en);cp('c_veh_dist',d.c_veh_dist);cb('veh_hp',d.veh_hp);cb('veh_ignlocal',d.veh_ignlocal);sl('veh_maxcount',d.veh_maxcount);sl('veh_maxdist',d.veh_maxdist);cb('veh_fix',d.veh_fix);hk('veh_fix_hk_btn',d.veh_fix_hk);
  cb('w_noclip',d.w_noclip);sl('w_noclip_spd',d.w_noclip_spd);cb('w_semigod',d.w_semigod);cb('w_sprint',d.w_sprint);sl('w_sprintspd',d.w_sprintspd);cb('w_stamina',d.w_stamina);cb('w_explode',d.w_explode);cb('w_fire',d.w_fire);
  cb('s_stream',d.s_stream);se('s_theme',d.s_theme);sl('s_maxplrs',d.s_maxplrs);hk('s_menukey_btn',d.s_menukey);hk('s_panickey_btn',d.s_panickey);
}
async function load(){try{const r=await fetch('/api/settings');applyUI(await r.json());}catch(e){}}
async function poll(){
  try{
    const r=await fetch('/api/status');const d=await r.json();
    document.getElementById('dot').className='dot'+(d.connected?' on':'');
    document.getElementById('stlbl').textContent=d.connected?('FiveM v'+d.version+' Connected'):'Waiting for FiveM...';
    const gv=document.getElementById('gver');if(gv)gv.textContent=d.version||'-';
    const addr=(d.ip&&d.ip!=='127.0.0.1'?d.ip:'localhost')+':'+d.port;
    const pl=document.getElementById('portlbl');if(pl)pl.textContent=addr;
  }catch(e){}
}
function cpAddr(){const t=document.getElementById('portlbl');if(!t)return;navigator.clipboard.writeText('http://'+t.textContent).then(()=>{const b=document.getElementById('cpbtn');if(b){b.textContent='Kopyalandı ✓';setTimeout(()=>{b.textContent='Kopyala';},1500);}});}
load();poll();setInterval(poll,3000);
function openKb(id,btnId){_lkId=id;_lkBtn=document.getElementById(btnId);document.getElementById('kb-modal').classList.add('active');}
function closeKb(){document.getElementById('kb-modal').classList.remove('active');}
function selectKey(key){const code=parseInt(key);s(_lkId,code);if(_lkBtn)_lkBtn.textContent=vkName(code);closeKb();}
document.querySelectorAll('.kb-key,.mouse-button').forEach(k=>{if(k.dataset.key)k.onclick=()=>selectKey(k.dataset.key);});
document.getElementById('kb-modal').onclick=e=>{if(e.target.id==='kb-modal')closeKb();};
</script>
</body></html>)RAW3";
    return s;
}


static void HandleWebRequest(SOCKET client) {
    DWORD to = 5000;
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (char*)&to, sizeof(to));
    char buf[16384] = {};
    int n = recv(client, buf, sizeof(buf)-1, 0);
    if (n <= 0) { closesocket(client); return; }

    std::string req(buf, n);
    auto sp1 = req.find(' ');
    auto sp2 = req.find(' ', sp1+1);
    if (sp1==std::string::npos||sp2==std::string::npos) { closesocket(client); return; }
    std::string method = req.substr(0, sp1);
    std::string path   = req.substr(sp1+1, sp2-sp1-1);
    auto qs = path.find('?'); if (qs!=std::string::npos) path=path.substr(0,qs);

    if (method == "OPTIONS") {
        const char* pre = "HTTP/1.1 204 No Content\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET,POST,OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type\r\nConnection: close\r\n\r\n";
        send(client, pre, (int)strlen(pre), 0);
        closesocket(client); return;
    }

    std::string body, ct;
    int status = 200;

    if (path=="/"||path=="/index.html") {
        body=GetMenuHtml(); ct="text/html; charset=utf-8";
    } else if (path=="/api/settings") {
        if (method=="GET") {
            body=BuildSettingsJson(); ct="application/json";
        } else if (method=="POST") {
            auto bs=req.find("\r\n\r\n");
            std::string rb=(bs!=std::string::npos)?req.substr(bs+4):"";
            body=ApplySettingsJson(rb)?"{\"ok\":true}":"{\"ok\":false}";
            if(body[6]=='f') status=400;
            ct="application/json";
        }
    } else if (path=="/api/status") {
        body=BuildStatusJson(); ct="application/json";
    } else {
        status=404; body="Not Found"; ct="text/plain";
    }

    std::ostringstream resp;
    resp << "HTTP/1.1 " << status << " OK\r\n"
         << "Content-Type: " << ct << "\r\n"
         << "Content-Length: " << body.size() << "\r\n"
         << "Access-Control-Allow-Origin: *\r\n"
         << "Cache-Control: no-cache\r\n"
         << "Connection: close\r\n\r\n" << body;
    std::string rs = resp.str();
    send(client, rs.c_str(), (int)rs.size(), 0);
    closesocket(client);
}


static void WebServerThread() {
    g_webPort = 7531; 

    WSADATA wsa = {};
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return;

    g_localIP = GetLocalIP();

    while (true) { 
        SOCKET srv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (srv == INVALID_SOCKET) { Sleep(2000); continue; }

        int opt = 1;
        setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

        sockaddr_in addr = {};
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);  
        addr.sin_port        = htons(g_webPort);

        if (bind(srv,(sockaddr*)&addr,sizeof(addr))!=0 || listen(srv,32)!=0) {
            closesocket(srv); Sleep(2000); continue;
        }

        while (true) { 
            fd_set fds; FD_ZERO(&fds); FD_SET(srv,&fds);
            timeval tv={1,0};
            int r = select(0,&fds,nullptr,nullptr,&tv);
            if (r < 0) break;   
            if (r == 0) continue;
            SOCKET client = accept(srv,nullptr,nullptr);
            if (client == INVALID_SOCKET) break;
            std::thread([client](){ HandleWebRequest(client); }).detach();
        }

        closesocket(srv);
        Sleep(1000);
    }

    WSACleanup();
}
