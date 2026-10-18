#include <mutex>

extern bool g_gameConnected;   
static std::mutex g_pedMutex;
static std::mutex g_vehMutex;
static uintptr_t GameWorld          = 0;
static uintptr_t GameViewPort       = 0;
static uintptr_t GameReplayInterface= 0;
static Matrix    ViewMatrix         = {};
static Ped              LocalPlayer;
static Ped*             SilentPed  = nullptr;
static Ped*             AimbotPed  = nullptr;
static std::vector<Ped>     PedList;
static std::vector<Vehicle> VehicleList;

bool WorldToScreen(const Matrix& vm, const Vector3& w, Vector2& out) {
    Matrix v = vm.Transpose();
    Vector4 r2(v._21,v._22,v._23,v._24);
    Vector4 r3(v._31,v._32,v._33,v._34);
    Vector4 r4(v._41,v._42,v._43,v._44);
    Vector3 p;
    p.x = r2.x*w.x + r2.y*w.y + r2.z*w.z + r2.w;
    p.y = r3.x*w.x + r3.y*w.y + r3.z*w.z + r3.w;
    p.z = r4.x*w.x + r4.y*w.y + r4.z*w.z + r4.w;
    if (p.z <= 0.1f) return false;
    float iz = 1.f / p.z;  p.x *= iz; p.y *= iz;
    float sw = (float)Game.lpRect.right, sh = (float)Game.lpRect.bottom;
    out.x = sw*0.5f + 0.5f*p.x*sw;
    out.y = sh*0.5f - 0.5f*p.y*sh;
    return true;
}

bool IsTargetInCrosshair(const Vector2& sp) {
    float cx = (float)Game.lpRect.right  * 0.5f;
    float cy = (float)Game.lpRect.bottom * 0.5f;
    return fabsf(sp.x-cx) <= 10.f && fabsf(sp.y-cy) <= 10.f;
}


void UpdatePeds() {
    while (true) {
        if (!keepRunning || !g_gameConnected) { Sleep(50); continue; }
        std::vector<Ped> updated;
        GameWorld             = ReadMemory<uintptr_t>(Offsets.GameBase + Offsets.GameWorld);
        GameViewPort          = ReadMemory<uintptr_t>(Offsets.GameBase + Offsets.ViewPort);
        GameReplayInterface   = ReadMemory<uintptr_t>(Offsets.GameBase + Offsets.ReplayInterface);
        LocalPlayer.Pointer   = ReadMemory<uintptr_t>(GameWorld + Offsets.LocalPlayer);

        
        if (Game.hWnd) {
            RECT r = {};
            if (GetClientRect(Game.hWnd, &r) && r.right > 0)
                Game.lpRect = r;
            else if (!Game.lpRect.right) {
                Game.lpRect.right  = GetSystemMetrics(SM_CXSCREEN);
                Game.lpRect.bottom = GetSystemMetrics(SM_CYSCREEN);
            }
        }

        uintptr_t elp  = ReadMemory<uintptr_t>(GameReplayInterface + 0x18);
        
        uintptr_t el   = ReadMemory<uintptr_t>(elp + 0x100);
        if (!el) el    = ReadMemory<uintptr_t>(elp + 0x108);

        int maxPlayerCount = Cheats::Settings::MaxPlayerCount;
        for (int i = 0; i < maxPlayerCount; ++i) {
            Ped ped;
            uintptr_t player = ReadMemory<uintptr_t>(el + i*0x10);
            if (!player || player == LocalPlayer.Pointer) continue;
            if (!ped.GetPlayer(player))  continue;
            if (!ped.Update())           continue;
            updated.push_back(ped);
        }
        { std::lock_guard<std::mutex> lk(g_pedMutex); PedList = std::move(updated); }

        
        if (!g_cit.ready && !g_cit.scanning && !updated.empty()) {
            int refId = -1;
            for (auto& p : updated) {
                int id = p.GetId();
                if (id > 0 && id < 2048) { refId = id; break; }
            }
            if (refId > 0)
                std::thread(ScanCitizenNames, refId).detach();
        }

       
        Sleep(isMenuVisible ? 8 : 16);
    }
}


void UpdateVehicles() {
    while (true) {
        if (!keepRunning || !g_gameConnected) { Sleep(50); continue; }
        if (Cheats::Vehicles::DrawPoint::Enabled ||
            Cheats::Vehicles::DrawLine::Enabled  ||
            Cheats::Vehicles::DrawDistance::Enabled ||
            Cheats::Vehicles::DrawHealthBar::Enabled)
        {
            std::vector<Vehicle> updated;
            uintptr_t vi  = ReadMemory<uintptr_t>(GameReplayInterface + 0x10);
            uintptr_t vl  = ReadMemory<uintptr_t>(vi + 0x180);
            int       cnt = ReadMemory<int>(vi + 0x188);
            if (cnt <= 0 || cnt > Cheats::Vehicles::Settings::MaxVehicleCount) { Sleep(10); continue; }

            Matrix vm = ReadMemory<Matrix>(GameViewPort + 0x24C);
            Vector3 localPos = ReadMemory<Vector3>(LocalPlayer.Pointer + 0x90);

            for (int i = 0; i < cnt; ++i) {
                uintptr_t vp = ReadMemory<uintptr_t>(vl + i*0x10);
                if (!vp) continue;
                Vector3 vpos = ReadMemory<Vector3>(vp + 0x90);
                Vector2 scr;
                if (!WorldToScreen(vm, vpos, scr)) continue;
                Vector3 diff = localPos - vpos;
                float dist = sqrtf(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
                if (dist > Cheats::Vehicles::Settings::MaxDistance) continue;
                if ((int)dist == 0 && Cheats::Vehicles::Settings::IgnoreLocalVehicle) continue;
                Vehicle veh;
                veh.Pointer = vp;
                veh.Location = scr;
                veh.Health = ReadMemory<float>(vp + Offsets.Health);
                veh.Distance = dist;
                updated.push_back(veh);
            }
            { std::lock_guard<std::mutex> lk(g_vehMutex); VehicleList = updated; }
        }
        Sleep(10);
    }
}


void DrawTextElement(ImDrawList* dl, const ImVec2& pos, const ImVec2& sz,
                     const std::string& text, bool enabled, int locType,
                     float locTop, float locBot, int font, int fontType, ImU32 col)
{
    if (!enabled) return;
    ImVec2 ts   = ImGui::CalcTextSize(text.c_str());
    ImVec2 tp;
    if (locType == 0) { tp.x = pos.x+(sz.x-ts.x)*0.5f; tp.y = pos.y-ts.y-locTop; }
    else              { tp.x = pos.x+(sz.x-ts.x)*0.5f; tp.y = pos.y+sz.y+locBot; }

    ImFont* f = (font==0) ? RobotoEsp : RobotoBold;
    if (f) ImGui::PushFont(f);
   
    ImVec2 ts2 = ImGui::CalcTextSize(text.c_str());
    dl->AddRectFilled({tp.x-3.f, tp.y-1.f}, {tp.x+ts2.x+3.f, tp.y+ts2.y+1.f},
                      IM_COL32(0,0,0,110), 2.f);
    
    {
        ImU32 rgb = col & 0x00FFFFFFu;
        static const ImU32 ga[] = {0x28000000u,0x18000000u,0x0C000000u};
        static const float ge[] = {2.5f, 4.5f, 7.f};
        for (int gi=0;gi<3;++gi) {
            ImU32 gc = rgb | ga[gi];
            dl->AddText({tp.x-ge[gi], tp.y}, gc, text.c_str());
            dl->AddText({tp.x+ge[gi], tp.y}, gc, text.c_str());
            dl->AddText({tp.x, tp.y-ge[gi]}, gc, text.c_str());
            dl->AddText({tp.x, tp.y+ge[gi]}, gc, text.c_str());
        }
    }
   
    if (fontType == 0) {
        dl->AddText(tp, col, text.c_str());
    } else {
        ImVec2 o{1.5f,1.5f};
        dl->AddText({tp.x-o.x,tp.y-o.y}, IM_COL32(0,0,0,200), text.c_str());
        dl->AddText({tp.x+o.x,tp.y-o.y}, IM_COL32(0,0,0,200), text.c_str());
        dl->AddText({tp.x-o.x,tp.y+o.y}, IM_COL32(0,0,0,200), text.c_str());
        dl->AddText({tp.x+o.x,tp.y+o.y}, IM_COL32(0,0,0,200), text.c_str());
        dl->AddText(tp, col, text.c_str());
    }
    if (f) ImGui::PopFont();
}


void DrawCheat() {
    ImDrawList* dl = ImGui::GetBackgroundDrawList();


    if (Game.hWnd) {
        RECT r = {};
        if (GetClientRect(Game.hWnd, &r) && r.right > 0)
            Game.lpRect = r;
    }
    
    if (!Game.lpRect.right) {
        Game.lpRect.right  = GetSystemMetrics(SM_CXSCREEN);
        Game.lpRect.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    float sw = (float)Game.lpRect.right, sh = (float)Game.lpRect.bottom;
    ImVec2 center(sw*0.5f, sh*0.5f);

    
    auto drawFovCircle = [&](bool draw, float radius, ImColor baseCol, float baseW,
                              bool hasTarget, float& lockProg) {
        if (!draw) return;
        double t = ImGui::GetTime();
        bool locked = hasTarget;
        
        float dt = ImGui::GetIO().DeltaTime;
        lockProg = locked ? ImMin(lockProg + dt * 4.f, 1.f)
                          : ImMax(lockProg - dt * 6.f, 0.f);

       
        ImU32 circCol = (ImU32)baseCol;
        float lw = baseW;
        if (lockProg > 0.f) {
           
            float r2=0.39f, g2=0.87f, b2=0.51f;
            float p = lockProg;
            ImColor ac(baseCol.Value.x+(r2-baseCol.Value.x)*p,
                       baseCol.Value.y+(g2-baseCol.Value.y)*p,
                       baseCol.Value.z+(b2-baseCol.Value.z)*p,
                       baseCol.Value.w);
            circCol = (ImU32)ac;
            lw = baseW + lockProg * 1.5f;
        }
        dl->AddCircle(center, radius, circCol, 100, lw);

        
        if (lockProg > 0.f && lockProg < 1.f) {
            float sweep = lockProg * IM_PI * 2.f;
            dl->PathArcTo(center, radius, -IM_PI * 0.5f, -IM_PI * 0.5f + sweep, 80);
            ImU32 arcCol = IM_COL32(99,220,130,(int)(180*lockProg));
            dl->PathStroke(arcCol, false, lw + 1.5f);
        }
       
        if (lockProg >= 1.f) {
            float pulse = (float)(sin(t * 8.0) * 0.5 + 0.5);
            float tickLen = 8.f + pulse * 4.f;
            ImU32 tickCol = IM_COL32(99,220,130,(int)(200+55*pulse));
            for (int qi = 0; qi < 4; ++qi) {
                float ang = -IM_PI*0.5f + qi * IM_PI*0.5f;
                ImVec2 tip{ center.x + cosf(ang)*radius, center.y + sinf(ang)*radius };
                ImVec2 ext{ center.x + cosf(ang)*(radius+tickLen),
                             center.y + sinf(ang)*(radius+tickLen) };
                dl->AddLine(tip, ext, tickCol, 2.f);
            }
        }
    };

    static float s_silentLock = 0.f, s_aimLock = 0.f;
    bool silentKeyHeld2 = (!Cheats::AimAssist::Silent::HotKey ||
                           (GetAsyncKeyState(Cheats::AimAssist::Silent::HotKey)&0x8000));
    bool aimbotKeyHeld2 = (!Cheats::AimAssist::Aimbot::HotKey ||
                           (GetAsyncKeyState(Cheats::AimAssist::Aimbot::HotKey)&0x8000));

    drawFovCircle(Cheats::AimAssist::Silent::DrawFov,
                  (float)Cheats::AimAssist::Silent::Fov,
                  Cheats::AimAssist::Silent::FovColor,
                  (float)Cheats::AimAssist::Silent::FovWeight,
                  SilentPed != nullptr && silentKeyHeld2,
                  s_silentLock);
    drawFovCircle(Cheats::AimAssist::Aimbot::DrawFov,
                  (float)Cheats::AimAssist::Aimbot::Fov,
                  Cheats::AimAssist::Aimbot::FovColor,
                  (float)Cheats::AimAssist::Aimbot::FovWeight,
                  AimbotPed != nullptr && aimbotKeyHeld2,
                  s_aimLock);

    Matrix vm = ReadMemory<Matrix>(GameViewPort + 0x24C);
    if (!LocalPlayer.Update()) return;

    DemoUser.Id         = LocalPlayer.GetId();
    DemoUser.Name       = LocalPlayer.GetName();
    DemoUser.WeaponName = LocalPlayer.GetWeaponName();
    DemoUser.Health     = LocalPlayer.GetHealth();
    DemoUser.Armor      = LocalPlayer.GetArmor();

    
    std::vector<Ped> localPeds;
    { std::lock_guard<std::mutex> lk(g_pedMutex); localPeds = PedList; }

    for (auto& ped : localPeds) {
    
        if (!ped.Pointer || !ped.PlayerInfo) continue;

        Vector2 pBase{}, pHead{}, pNeck{}, pLFoot{}, pRFoot{};
        if (!WorldToScreen(vm, ped.Position,           pBase)  ||
            !WorldToScreen(vm, ped.BoneList[Head],     pHead)  ||
            !WorldToScreen(vm, ped.BoneList[Neck],     pNeck)  ||
            !WorldToScreen(vm, ped.BoneList[LeftFoot], pLFoot) ||
            !WorldToScreen(vm, ped.BoneList[RightFoot],pRFoot)) continue;

        float h2n   = pNeck.y - pHead.y;
        float pTop  = pHead.y - h2n*2.5f;
        float pBot  = (pLFoot.y > pRFoot.y ? pLFoot.y : pRFoot.y)*1.001f;
        float pH    = pBot - pTop;
        float pW    = pH / 3.5f;
        float pDist = GetDistance(ped.Position, LocalPlayer.Position);

        
        if (Cheats::AimAssist::Settings::Crosshair) {
            ImU32 cc = Cheats::AimAssist::Settings::CrosshairColor;
            int   cz = Cheats::AimAssist::Settings::CrosshairSize;
            int   ct = Cheats::AimAssist::Settings::CrosshairSelectedType;
            float th = 1.5f, h = (float)cz*0.5f;
            if (Cheats::AimAssist::Settings::DynamicCrosshairColor &&
                (IsTargetInCrosshair(pBase)||IsTargetInCrosshair(pHead)||
                 IsTargetInCrosshair(pNeck)||IsTargetInCrosshair(pLFoot)||IsTargetInCrosshair(pRFoot)))
                cc = IM_COL32(255,0,0,255);
            if      (ct==0) { dl->AddLine({center.x-h,center.y},{center.x+h,center.y},cc,th); dl->AddLine({center.x,center.y-h},{center.x,center.y+h},cc,th); }
            else if (ct==1) { float g=(float)cz*0.6f; dl->AddLine({center.x-h-g,center.y},{center.x-h,center.y},cc,th); dl->AddLine({center.x+h,center.y},{center.x+h+g,center.y},cc,th); dl->AddLine({center.x,center.y-h-g},{center.x,center.y-h},cc,th); dl->AddLine({center.x,center.y+h},{center.x,center.y+h+g},cc,th); }
            else if (ct==2) { dl->AddCircle(center,(float)cz*0.75f,cc,32,th); }
            else if (ct==3) { dl->AddRect({center.x-h,center.y-h},{center.x+h,center.y+h},cc,0,0,th); }
            else if (ct==4) { dl->AddTriangle({center.x,center.y-(float)cz},{center.x-(float)cz*0.8f,center.y+(float)cz*0.8f},{center.x+(float)cz*0.8f,center.y+(float)cz*0.8f},cc,th); }
            else if (ct==5) { dl->AddLine({center.x-(float)cz,center.y},{center.x+(float)cz,center.y},cc,th); dl->AddLine({center.x,center.y-(float)cz},{center.x,center.y+(float)cz},cc,th); }
            else if (ct==6) { dl->AddCircleFilled(center,(float)cz*0.25f,cc,16); }
            else if (ct==7) { dl->AddCircle(center,(float)cz,cc,32,th); dl->AddCircleFilled(center,(float)cz*0.25f,IM_COL32(255,0,0,255),16); }
        }

        // Target markers
        bool silentActive = SilentPed &&
            (!Cheats::AimAssist::Silent::HotKey || (GetAsyncKeyState(Cheats::AimAssist::Silent::HotKey)&0x8000));
        if (silentActive && Cheats::AimAssist::Silent::DrawTarget && ped.GetId()==SilentPed->GetId())
        {
            int t=Cheats::AimAssist::Silent::SelectedDrawTargetType;
            ImU32 tc=Cheats::AimAssist::Silent::DrawTargetColor;
            if      (t==0) dl->AddCircleFilled({pBase.x,(pTop+pBot)*0.5f},5.f,tc);
            else if (t==1) dl->AddRect({pBase.x-pW*0.75f,pTop},{pBase.x+pW*0.75f,pBot},tc,0,0,2.f);
            else           dl->AddText({pBase.x,pTop},tc,"Silent");
        }
        bool aimbotActive = AimbotPed &&
            (!Cheats::AimAssist::Aimbot::HotKey || (GetAsyncKeyState(Cheats::AimAssist::Aimbot::HotKey)&0x8000));
        if (aimbotActive && Cheats::AimAssist::Aimbot::DrawTarget && ped.GetId()==AimbotPed->GetId())
        {
            int t=Cheats::AimAssist::Aimbot::SelectedDrawTargetType;
            ImU32 tc=Cheats::AimAssist::Aimbot::DrawTargetColor;
            if      (t==0) dl->AddCircleFilled({pBase.x,(pTop+pBot)*0.5f},5.f,tc);
            else if (t==1) dl->AddRect({pBase.x-pW*0.75f,pTop},{pBase.x+pW*0.75f,pBot},tc,0,0,2.f);
            else           dl->AddText({pBase.x,pTop},tc,"Aimbot");
        }

        if (pDist > Cheats::Players::Settings::MaxDistance) continue;
        if (Cheats::Players::Settings::IgnorePed       && !ped.IsPlayer())  continue;
        if (Cheats::Players::Settings::IgnoreDeath     && ped.IsDead())     continue;
        if (Cheats::Players::Settings::IgnoreInvisible && !ped.IsVisible()) continue;
        if (IsFriend(ped.GetId())) continue;

        bool pedVis = ped.IsVisible();
        auto resolveColor = [&](ImColor normalCol) -> ImColor {
            if (Cheats::Players::VisCheck::Enabled)
                return pedVis ? ImColor(Cheats::Players::VisCheck::VisibleColor)
                              : ImColor(Cheats::Players::VisCheck::HiddenColor);
            if (!pedVis) return ImColor(normalCol.Value.x*0.3f,normalCol.Value.y*0.3f,normalCol.Value.z*0.3f,normalCol.Value.w);
            return normalCol;
        };

        // Skeleton
        if (Cheats::Players::VisualMarkers::DrawSkeleton::Enabled) {
            ImColor sc = resolveColor(Cheats::Players::VisualMarkers::DrawSkeleton::Color);
            ImU32  sc32 = (ImU32)sc;
            float lw    = ImMax(1.5f, (float)Cheats::Players::VisualMarkers::GlobalSettings::LineWeight);
            bool outline = Cheats::Players::VisualMarkers::GlobalSettings::SelectedLineType == 1;
            ImU32 shadow = IM_COL32(0,0,0,200);

            // Project all 9 bones once
            Vector2 bS[9] = {};
            bool    bOk[9] = {};
            for (int bi = 0; bi < 9; ++bi)
                if (!Vec3Empty(ped.BoneList[bi]))
                    bOk[bi] = WorldToScreen(vm, ped.BoneList[bi], bS[bi]);

            // Head circle (radius proportional to head-neck screen distance)
            if (bOk[Head]) {
                float hr = 5.f;
                if (bOk[Neck]) hr = ImMax(3.5f, fabsf(bS[Head].y - bS[Neck].y) * 0.6f);
                ImVec2 hPos{bS[Head].x, bS[Head].y};
                if (outline) dl->AddCircle(hPos, hr+1.5f, shadow, 14, lw+1.f);
                dl->AddCircle(hPos, hr, sc32, 14, lw);
            }

            // Helpers: Vector2 → ImVec2 conversions inside lambdas
            auto toI = [](Vector2 v) -> ImVec2 { return {v.x, v.y}; };
            auto drawLine = [&](Vector2 a, Vector2 b) {
                ImVec2 ia = toI(a), ib = toI(b);
                if (outline) dl->AddLine(ia, ib, shadow, lw+2.f);
                dl->AddLine(ia, ib, sc32, lw);
            };
            auto drawDot = [&](Vector2 p) {
                ImVec2 ip = toI(p);
                if (outline) dl->AddCircleFilled(ip, 2.5f, shadow);
                dl->AddCircleFilled(ip, 2.f, sc32, 8);
            };

            // Spine: neck → hip
            if (bOk[Neck] && bOk[Hip]) drawLine(bS[Neck], bS[Hip]);

            // Arms: synthesize shoulder midpoint (30% from neck toward hand)
            if (bOk[Neck] && bOk[LeftHand]) {
                Vector2 lShoulder = {
                    bS[Neck].x + (bS[LeftHand].x  - bS[Neck].x) * 0.30f,
                    bS[Neck].y + (bS[LeftHand].y  - bS[Neck].y) * 0.20f
                };
                drawLine(bS[Neck], lShoulder);
                drawLine(lShoulder, bS[LeftHand]);
                drawDot(lShoulder);
            }
            if (bOk[Neck] && bOk[RightHand]) {
                Vector2 rShoulder = {
                    bS[Neck].x + (bS[RightHand].x - bS[Neck].x) * 0.30f,
                    bS[Neck].y + (bS[RightHand].y - bS[Neck].y) * 0.20f
                };
                drawLine(bS[Neck], rShoulder);
                drawLine(rShoulder, bS[RightHand]);
                drawDot(rShoulder);
            }

            // Legs: hip → ankle → foot (fallback to direct hip→foot if ankle missing)
            if (bOk[Hip]       && bOk[LeftAnkle])  drawLine(bS[Hip],       bS[LeftAnkle]);
            if (bOk[LeftAnkle] && bOk[LeftFoot])   drawLine(bS[LeftAnkle], bS[LeftFoot]);
            else if (bOk[Hip]  && bOk[LeftFoot])   drawLine(bS[Hip],       bS[LeftFoot]);

            if (bOk[Hip]        && bOk[RightAnkle]) drawLine(bS[Hip],        bS[RightAnkle]);
            if (bOk[RightAnkle] && bOk[RightFoot])  drawLine(bS[RightAnkle], bS[RightFoot]);
            else if (bOk[Hip]   && bOk[RightFoot])  drawLine(bS[Hip],        bS[RightFoot]);

            // Joint dots at main joints
            for (int bi : {Neck, Hip, LeftHand, RightHand, LeftAnkle, RightAnkle})
                if (bOk[bi]) drawDot(bS[bi]);
        }

        // Bone Points
        if (Cheats::Players::VisualMarkers::DrawBonePoints::Enabled) {
            ImColor bpc = resolveColor(Cheats::Players::VisualMarkers::DrawBonePoints::Color);
            float   bpr = (float)Cheats::Players::VisualMarkers::DrawBonePoints::Radius;
            for (int bi = 0; bi < 9; ++bi) {
                if (Vec3Empty(ped.BoneList[bi])) continue;
                Vector2 bs;
                if (WorldToScreen(vm, ped.BoneList[bi], bs))
                    dl->AddCircleFilled({bs.x, bs.y}, bpr, (ImU32)bpc, 8);
            }
        }

        // Box
        if (Cheats::Players::VisualMarkers::DrawBox::Enabled) {
            ImColor bc = resolveColor(Cheats::Players::VisualMarkers::DrawBox::Color);
            float nw=pW*1.5f, lw=(float)Cheats::Players::VisualMarkers::GlobalSettings::LineWeight;
            ImVec2 tl(pBase.x-nw*0.5f,pTop), br(pBase.x+nw*0.5f,pBot);
            int bt=Cheats::Players::VisualMarkers::DrawBox::SelectedType;
            if (bt==0) {
                dl->AddRectFilled(tl,br,IM_COL32(0,0,0,50));
                dl->AddRect(tl,br,IM_COL32(0,0,0,255),0,0,1.f);
                float cs=nw*0.3f;
                dl->AddLine(tl,{tl.x+cs,tl.y},bc,2.f); dl->AddLine(tl,{tl.x,tl.y+cs},bc,2.f);
                dl->AddLine({br.x,tl.y},{br.x-cs,tl.y},bc,2.f); dl->AddLine({br.x,tl.y},{br.x,tl.y+cs},bc,2.f);
                dl->AddLine({tl.x,br.y},{tl.x+cs,br.y},bc,2.f); dl->AddLine({tl.x,br.y},{tl.x,br.y-cs},bc,2.f);
                dl->AddLine(br,{br.x-cs,br.y},bc,2.f); dl->AddLine(br,{br.x,br.y-cs},bc,2.f);
            } else {
                if (Cheats::Players::VisualMarkers::GlobalSettings::SelectedLineType==1)
                    dl->AddRect(tl,br,IM_COL32(0,0,0,255),0,0,lw+5.f);
                dl->AddRect(tl,br,bc,0,0,lw);
            }
        }

        // Snapline
        if (Cheats::Players::VisualMarkers::DrawLine::Enabled) {
            ImColor lc = resolveColor(Cheats::Players::VisualMarkers::DrawLine::Color);
            int loc=Cheats::Players::VisualMarkers::DrawLine::SelectedLocation;
            ImVec2 start = (loc==0)?ImVec2(sw*0.5f,0.f):(loc==1)?ImVec2(sw*0.5f,sh*0.5f):ImVec2(sw*0.5f,sh);
            ImVec2 target(pBase.x,(pTop+pBot)*0.5f);
            float lw=(float)Cheats::Players::VisualMarkers::GlobalSettings::LineWeight;
            if (Cheats::Players::VisualMarkers::GlobalSettings::SelectedLineType==1)
                dl->AddLine(start,target,IM_COL32(0,0,0,255),lw+5.f);
            dl->AddLine(start,target,lc,lw);
        }

        // Player info text
        if (pDist <= Cheats::Players::PlayerInfo::GlobalSettings::MaxDistance) {
            float bh=pBot-pTop, bw2=bh/3.5f;
            ImVec2 pos2(pBase.x-bw2*0.5f,pTop), sz2(bw2,bh);
            int fn=Cheats::Players::PlayerInfo::GlobalSettings::SelectedFont;
            int ft=Cheats::Players::PlayerInfo::GlobalSettings::SelectedFontType;
            float yT=7.f,yB=7.f,step=15.f;
            auto addInfo=[&](bool en,int loc,ImColor col,const std::string& txt){
                if(!en)return;
                if(loc==0){DrawTextElement(dl,pos2,sz2,txt,true,0,yT,0.f,fn,ft,col);yT+=step;}
                else      {DrawTextElement(dl,pos2,sz2,txt,true,1,0.f,yB,fn,ft,col);yB+=step;}
            };
            addInfo(Cheats::Players::PlayerInfo::DrawName::Enabled,
                    Cheats::Players::PlayerInfo::DrawName::SelectedLocation,
                    Cheats::Players::PlayerInfo::DrawName::Color, ped.GetName());
            addInfo(Cheats::Players::PlayerInfo::DrawId::Enabled,
                    Cheats::Players::PlayerInfo::DrawId::SelectedLocation,
                    Cheats::Players::PlayerInfo::DrawId::Color, std::to_string(ped.GetId()));
            addInfo(Cheats::Players::PlayerInfo::DrawWeaponName::Enabled,
                    Cheats::Players::PlayerInfo::DrawWeaponName::SelectedLocation,
                    Cheats::Players::PlayerInfo::DrawWeaponName::Color, ped.GetWeaponName());
            addInfo(Cheats::Players::PlayerInfo::DrawDistance::Enabled,
                    Cheats::Players::PlayerInfo::DrawDistance::SelectedLocation,
                    Cheats::Players::PlayerInfo::DrawDistance::Color,
                    "["+std::to_string((int)pDist)+"m]");
        }

        // Health bar
        if (Cheats::Players::StatusBars::DrawHealthBar::Enabled) {
            float nw=pW*1.5f, bH=pBot-pTop+2.f;
            float bW=4.f-std::min(pDist/100.f,3.f);
            float hR=ped.GetHealth()/(float)Cheats::Players::StatusBars::GlobalSettings::MaxHealth;
            int   loc=Cheats::Players::StatusBars::DrawHealthBar::SelectedLocation;
            ImVec2 bTL=(loc==0)?ImVec2(pBase.x-nw*0.5f-9.f,pTop):ImVec2(pBase.x+nw*0.5f+3.f,pTop);
            ImVec2 bBR={bTL.x+bW,bTL.y+bH};
            dl->AddRectFilled(bTL,bBR,IM_COL32(50,50,50,150));
            int r=255,g=0;
            if(hR>=0.8f){r=(int)(255*(1.f-(hR-0.8f)*5));g=255;}
            else if(hR>=0.6f){r=128;g=255;}else if(hR>=0.4f){g=192;}else if(hR>=0.2f){g=100;}
            dl->AddRectFilled({bTL.x,bBR.y-bH*hR},bBR,IM_COL32(r,g,0,255));
        }

        // Armor bar
        if (Cheats::Players::StatusBars::DrawArmorBar::Enabled) {
            float nw=pW*1.5f, bH=pBot-pTop+2.f;
            float bW=4.f-std::min(pDist/100.f,3.f);
            float aR=ped.GetArmor()/(float)Cheats::Players::StatusBars::GlobalSettings::MaxArmor;
            int   loc=Cheats::Players::StatusBars::DrawArmorBar::SelectedLocation;
            ImVec2 bTL=(loc==0)?ImVec2(pBase.x-nw*0.5f-9.f,pTop):ImVec2(pBase.x+nw*0.5f+3.f,pTop);
            ImVec2 bBR={bTL.x+bW,bTL.y+bH};
            dl->AddRectFilled(bTL,bBR,IM_COL32(50,50,50,150));
            dl->AddRectFilled({bTL.x,bBR.y-bH*aR},bBR,IM_COL32(0,(int)(aR*100),(int)(aR*255),255));
        }
    }

    // Vehicle ESP
    if (Cheats::Vehicles::DrawPoint::Enabled || Cheats::Vehicles::DrawLine::Enabled ||
        Cheats::Vehicles::DrawDistance::Enabled || Cheats::Vehicles::DrawHealthBar::Enabled)
    {
        std::vector<Vehicle> localVehs;
        { std::lock_guard<std::mutex> lk(g_vehMutex); localVehs = VehicleList; }
        for (auto& v : localVehs) {
            ImVec2 vp2(v.Location.x, v.Location.y);
            if (Cheats::Vehicles::DrawLine::Enabled) {
                int loc=Cheats::Vehicles::DrawLine::SelectedLocation;
                ImVec2 start=(loc==0)?ImVec2(sw*0.5f,0.f):(loc==1)?ImVec2(sw*0.5f,sh*0.5f):ImVec2(sw*0.5f,sh);
                if (Cheats::Vehicles::DrawLine::SelectedLineType==1)
                    dl->AddLine(start,vp2,IM_COL32(0,0,0,255),3.f);
                dl->AddLine(start,vp2,Cheats::Vehicles::DrawLine::Color,1.5f);
            }
            if (Cheats::Vehicles::DrawPoint::Enabled) {
                dl->AddCircleFilled(vp2,(float)Cheats::Vehicles::DrawPoint::Size,Cheats::Vehicles::DrawPoint::Color);
                if (Cheats::Vehicles::DrawPoint::SelectedLineType==1)
                    dl->AddCircle(vp2,(float)(Cheats::Vehicles::DrawPoint::Size+1),IM_COL32(0,0,0,255),32,1.5f);
            }
            if (Cheats::Vehicles::DrawDistance::Enabled) {
                char dtxt[32]; snprintf(dtxt,sizeof(dtxt),"%.0fm",v.Distance);
                ImFont* f=(Cheats::Vehicles::DrawDistance::SelectedFont==0)?RobotoEsp:RobotoBold;
                if(f) ImGui::PushFont(f);
                dl->AddText({vp2.x-10,vp2.y-30},Cheats::Vehicles::DrawDistance::Color,dtxt);
                if(f) ImGui::PopFont();
            }
            if (Cheats::Vehicles::DrawHealthBar::Enabled) {
                float hR=v.Health/1000.f; int r=255,g=0;
                if(hR>=0.8f){r=(int)(255*(1.f-(hR-0.8f)*5));g=255;}
                else if(hR>=0.6f){r=128;g=255;}else if(hR>=0.4f){g=192;}
                ImVec2 bp(vp2.x-25,vp2.y-20);
                dl->AddRectFilled(bp,{bp.x+50,bp.y+5},IM_COL32(0,0,0,255));
                dl->AddRectFilled(bp,{bp.x+50*hR,bp.y+5},IM_COL32(r,g,0,255));
            }
        }
    }
}

// ============================================================
//  GetTargetBone — maps BoneMode to actual bone index
// ============================================================
int GetTargetBone(int boneMode) {
    switch (boneMode) {
        case 0: return Head;
        case 1: return Hip;                            // Torso ≈ hip/center mass
        case 2: return (rand() & 1) ? LeftFoot : RightFoot;  // Legs
        case 3: { static const int k[]={Head,Neck,Hip,LeftHand,RightHand,LeftFoot,RightFoot};
                  return k[rand() % 7]; }              // Random
        default: return Head;
    }
}

// ============================================================
//  FindTarget
// ============================================================
Ped FindTarget(int fov, int dist, int boneMode, int priority = 0,
               bool visCheck = false, bool skipFriends = true) {
    int area = GetTargetBone(boneMode);
    Ped best; float bestVal = 9999.f;
    Matrix vm = ReadMemory<Matrix>(GameViewPort + 0x24C);
    Vector2 center((float)Game.lpRect.right*0.5f,(float)Game.lpRect.bottom*0.5f);
    std::vector<Ped> snapPeds;
    { std::lock_guard<std::mutex> lk(g_pedMutex); snapPeds = PedList; }
    for (auto& ped : snapPeds) {
        if (!LocalPlayer.Update()) break;
        if (!ped.Update()) continue;
        float pd = GetDistance(ped.Position, LocalPlayer.Position);
        if (pd > dist) continue;
        if (Cheats::AimAssist::Settings::IgnorePed       && !ped.IsPlayer())  continue;
        if (Cheats::AimAssist::Settings::IgnoreDeath     && ped.IsDead())     continue;
        if (Cheats::AimAssist::Settings::IgnoreInvisible && !ped.IsVisible()) continue;
        if (visCheck && !ped.IsVisible()) continue;
        if (skipFriends && IsFriend(ped.GetId())) continue;
        Vector2 sp;
        if (!WorldToScreen(vm, ped.BoneList[area], sp)) continue;
        float cf = fabsf((center - sp).Length());
        if (cf > fov) continue;
        float val = (priority == 1) ? pd
                  : (priority == 2) ? ped.GetHealth()
                  : cf;
        if (val < bestVal) { best = ped; bestVal = val; }
    }
    return best;
}

Vector3 CalcAngle(Vector3 cam, Vector3 target) {
    float d = GetDistance(cam, target);
    if (d < 0.001f) return {};
    return { (target.x-cam.x)/d, (target.y-cam.y)/d, (target.z-cam.z)/d };
}

void NormalizeAngles(Vector3& a) {
    while (a.x >  89.f) a.x -= 180.f;
    while (a.x < -89.f) a.x += 180.f;
    while (a.y >  180.f) a.y -= 360.f;
    while (a.y < -180.f) a.y += 360.f;
}



Vector3 EndBulletPos;


static uintptr_t g_cave = 0;


static uintptr_t GetOrAllocCave() {
    if (g_cave) return g_cave;
    if (!Offsets.HandleBullet || !Game.hProcess) return 0;

    uintptr_t origin = Offsets.GameBase + Offsets.HandleBullet;
    uintptr_t lo = (origin > 0x6BFFFFFF) ? origin - 0x6BFFFFFF : 0x10000;
    uintptr_t hi = origin + 0x6BFFFFFF;

    
    uintptr_t scan = origin;
    MEMORY_BASIC_INFORMATION mbi{};
    while (scan < hi) {
        if (!VirtualQueryEx(Game.hProcess, reinterpret_cast<LPVOID>(scan), &mbi, sizeof(mbi))) break;
        if (mbi.State == MEM_FREE && mbi.RegionSize >= 64) {
            LPVOID mem = VirtualAllocEx(Game.hProcess, reinterpret_cast<LPVOID>(scan), 64,
                                        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (mem) { g_cave = reinterpret_cast<uintptr_t>(mem); return g_cave; }
        }
        uintptr_t next = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        if (next <= scan) break;
        scan = next;
    }
    // Geri tarama
    scan = (origin > 0x10000) ? origin - 0x10000 : 0;
    while (scan > lo) {
        if (!VirtualQueryEx(Game.hProcess, reinterpret_cast<LPVOID>(scan), &mbi, sizeof(mbi))) break;
        if (mbi.State == MEM_FREE && mbi.RegionSize >= 64) {
            LPVOID mem = VirtualAllocEx(Game.hProcess, reinterpret_cast<LPVOID>(scan), 64,
                                        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (mem) { g_cave = reinterpret_cast<uintptr_t>(mem); return g_cave; }
        }
        if (reinterpret_cast<uintptr_t>(mbi.BaseAddress) < 0x10000) break;
        scan = reinterpret_cast<uintptr_t>(mbi.BaseAddress) - 0x10000;
    }
    return 0;
}

void ApplySilent() {
    uint64_t HB    = Offsets.GameBase + Offsets.HandleBullet;
    uint64_t Alloc = GetOrAllocCave();
    if (!Alloc) return;
    union { float f; uint32_t i; } X,Y,Z;
    X.f=EndBulletPos.x; Y.f=EndBulletPos.y; Z.f=EndBulletPos.z;
    {
        uint32_t rel=(uint32_t)(intptr_t)(Alloc-(HB+5));
        uint8_t jmp[]={0xE9,(uint8_t)rel,(uint8_t)(rel>>8),(uint8_t)(rel>>16),(uint8_t)(rel>>24)};
        WriteBytes(HB,jmp,5);
    }
    {
        intptr_t rel=(intptr_t)(HB-(Alloc+28)); uint32_t jof=(uint32_t)rel;
        uint8_t code[]={
            0x41,0xC7,0x01,(uint8_t)X.i,(uint8_t)(X.i>>8),(uint8_t)(X.i>>16),(uint8_t)(X.i>>24),
            0x41,0xC7,0x41,0x04,(uint8_t)Y.i,(uint8_t)(Y.i>>8),(uint8_t)(Y.i>>16),(uint8_t)(Y.i>>24),
            0x41,0xC7,0x41,0x08,(uint8_t)Z.i,(uint8_t)(Z.i>>8),(uint8_t)(Z.i>>16),(uint8_t)(Z.i>>24),
            0xF3,0x41,0x0F,0x10,0x19,
            0xE9,(uint8_t)jof,(uint8_t)(jof>>8),(uint8_t)(jof>>16),(uint8_t)(jof>>24)
        };
        WriteBytes(Alloc,code,sizeof(code));
    }
}

void RestoreSilent() {
    if (!Offsets.HandleBullet) return;
    uint8_t orig[]={0xF3,0x41,0x0F,0x10,0x19,0xF3,0x41,0x0F,0x10,0x41,0x04,0xF3,0x41,0x0F,0x10,0x51,0x08};
    WriteBytes(Offsets.GameBase+Offsets.HandleBullet, orig, sizeof(orig));
    // Cave'i temizle (alloc edilmişse)
    if (g_cave) {
        uint8_t zeros[64]={};
        WriteBytes(g_cave, zeros, sizeof(zeros));
    }
}

// ============================================================
//  AimAssist thread
// ============================================================
void AimAssist() {
    // Kalici hedef depolama — dangling pointer olmasın
    static Ped s_silentTarget;
    static Ped s_aimbotTarget;
    static Ped s_mbTarget;
    static bool mbApplied = false;

    while (true) {
        if (!keepRunning || !g_gameConnected) { Sleep(50); continue; }
        // ── Magic Bullet ──────────────────────────────────────────
        // Silent ile aynı code cave'i kullanır; silent açıksa öncelik silent'ta.
        if (Cheats::AimAssist::MagicBullet::Enabled && !Cheats::AimAssist::Silent::Enabled) {
            int bm   = Cheats::AimAssist::MagicBullet::BoneMode;
            int area = GetTargetBone(bm);
            Ped t = FindTarget(500,
                               Cheats::AimAssist::MagicBullet::MaxDistance,
                               bm, 0,
                               false,   // vis check yok — duvar arkasini da hedef al
                               Cheats::AimAssist::MagicBullet::SkipFriends);
            // NPC filtresi
            if (!Vec3Empty(t.BoneList[area]) &&
                Cheats::AimAssist::MagicBullet::SkipNPC && !t.IsPlayer()) {
                // NPC'yi atla
                if (mbApplied) { RestoreSilent(); mbApplied = false; }
            } else if (!Vec3Empty(t.BoneList[area])) {
                if (!mbApplied)
                    PushToast("Magic Bullet: Aktif", IM_COL32(255,180,0,255), 1.5f);
                s_mbTarget   = t;
                EndBulletPos = t.BoneList[area];
                ApplySilent();   // aynı code cave'i kullan
                mbApplied = true;
            } else {
                if (mbApplied) { RestoreSilent(); mbApplied = false; }
            }
        } else if (mbApplied && !Cheats::AimAssist::Silent::Enabled) {
            // Magic bullet kapandı, cave'i temizle
            RestoreSilent();
            mbApplied = false;
        }

       
        if (Cheats::AimAssist::Silent::Enabled) {
            int bm = Cheats::AimAssist::Silent::BoneMode;
            int area = GetTargetBone(bm);
            Ped t = FindTarget(Cheats::AimAssist::Silent::Fov,
                               Cheats::AimAssist::Silent::MaxDistance, bm, 0,
                               Cheats::AimAssist::Silent::VisCheck,
                               Cheats::AimAssist::Silent::SkipFriends);
            bool silentKey = (!Cheats::AimAssist::Silent::HotKey ||
                               (GetAsyncKeyState(Cheats::AimAssist::Silent::HotKey)&0x8000));
            if (Vec3Empty(t.BoneList[area])) { RestoreSilent(); SilentPed=nullptr; }
            else if (silentKey) {
                if (!SilentPed) PushToast("Silent: Hedef Kilitlendi", IM_COL32(100,220,140,255), 1.5f);
                s_silentTarget = t;          
                SilentPed = &s_silentTarget;
                EndBulletPos = t.BoneList[area];
                // waveShield bypass
                EndBulletPos.x += ((rand()%101)-50)*0.001f;
                EndBulletPos.y += ((rand()%101)-50)*0.001f;
                EndBulletPos.z += ((rand()%51 )-25)*0.001f;
                int mc = Cheats::AimAssist::Silent::MissChance;
                if (mc <= 0 || (rand()%100) >= mc) ApplySilent();
                else RestoreSilent();
            } else { SilentPed=nullptr; RestoreSilent(); }
        } else {
            // Silent kapaliyken temizle
            if (SilentPed) { RestoreSilent(); SilentPed=nullptr; }
        }

        // Aimbot
        bool aimbotKey = (!Cheats::AimAssist::Aimbot::HotKey ||
                          (GetAsyncKeyState(Cheats::AimAssist::Aimbot::HotKey)&0x8000));
        if (Cheats::AimAssist::Aimbot::Enabled && aimbotKey) {
            int bm   = Cheats::AimAssist::Aimbot::BoneMode;
            int area = GetTargetBone(bm);
            int prio = Cheats::AimAssist::Aimbot::Priority;
            Ped t = FindTarget(Cheats::AimAssist::Aimbot::Fov,
                               Cheats::AimAssist::Aimbot::MaxDistance, bm, prio,
                               Cheats::AimAssist::Aimbot::VisCheck,
                               Cheats::AimAssist::Aimbot::SkipFriends);
            if (!Vec3Empty(t.BoneList[area])) {
                // Kamera pointer null kontrolü — 0'a yazma → FiveM crash
                uintptr_t cam = (Offsets.Camera && Offsets.GameBase)
                    ? ReadMemory<uintptr_t>(Offsets.GameBase + Offsets.Camera) : 0;
                if (cam) {
                    Vector3 vAngle=ReadMemory<Vector3>(cam+0x3D0);
                    Vector3 camPos=ReadMemory<Vector3>(cam+0x60);
                    Vector3 angle=CalcAngle(camPos,t.BoneList[area]);
                    NormalizeAngles(angle);
                    Vector3 delta=angle-vAngle; NormalizeAngles(delta);
                    int sm=Cheats::AimAssist::Aimbot::Smooth;
                    Vector3 write=vAngle+(sm>0?delta/(float)sm:delta);
                    NormalizeAngles(write);
                    // NaN/Inf koruması: geçersiz açı değeri oyunu çökertiyor
                    bool valid = std::isfinite(write.x) && std::isfinite(write.y)
                              && !Vec3Empty(write);
                    if (valid) {
                        if (Cheats::AimAssist::Aimbot::AimMode == 1 && GameViewPort) {
                            // Mouse mode: pixel tabanlı hareket
                            Matrix vm = ReadMemory<Matrix>(GameViewPort + 0x24C);
                            Vector2 sp;
                            if (WorldToScreen(vm, t.BoneList[area], sp)) {
                                float cx=(float)Game.lpRect.right*0.5f, cy=(float)Game.lpRect.bottom*0.5f;
                                float dx=(sp.x-cx)/(float)(sm>0?sm:1), dy=(sp.y-cy)/(float)(sm>0?sm:1);
                                if (std::isfinite(dx) && std::isfinite(dy))
                                    mouse_event(MOUSEEVENTF_MOVE,(DWORD)(int)dx,(DWORD)(int)dy,0,0);
                            }
                        } else if (Cheats::AimAssist::Aimbot::AimMode != 1) {
                            WriteMemory<Vector3>(cam+0x3D0, write);
                        }
                        if (!AimbotPed) PushToast("Aimbot: Hedef Kilitlendi", IM_COL32(80,180,255,255), 1.5f);
                        s_aimbotTarget = t;
                        AimbotPed = &s_aimbotTarget;
                    }
                }
            } else if (!Cheats::AimAssist::Aimbot::StickyAim) {
                AimbotPed=nullptr;
            }
        }

        // Triggerbot
        if (Cheats::AimAssist::Triggerbot::Enabled) {
            Matrix vm=ReadMemory<Matrix>(GameViewPort+0x24C);
            std::vector<Ped> tbPeds;
            { std::lock_guard<std::mutex> lk(g_pedMutex); tbPeds = PedList; }
            for (auto& ped : tbPeds) {
                if (!ped.Update()) continue;
                float pd=GetDistance(ped.Position,LocalPlayer.Position);
                if (pd>Cheats::AimAssist::Triggerbot::MaxDistance) continue;
                if (Cheats::AimAssist::Settings::IgnorePed&&!ped.IsPlayer()) continue;
                if (Cheats::AimAssist::Settings::IgnoreDeath&&ped.IsDead()) continue;
                if (!ped.IsVisible()) continue;
                if (Cheats::AimAssist::Triggerbot::SkipFriends && IsFriend(ped.GetId())) continue;
                Vector2 pB{},pH{},pN{},pLF{},pRF{};
                if(!WorldToScreen(vm,ped.Position,pB)||!WorldToScreen(vm,ped.BoneList[Head],pH)||
                   !WorldToScreen(vm,ped.BoneList[Neck],pN)||!WorldToScreen(vm,ped.BoneList[LeftFoot],pLF)||
                   !WorldToScreen(vm,ped.BoneList[RightFoot],pRF)) continue;
                if(IsTargetInCrosshair(pB)||IsTargetInCrosshair(pH)||IsTargetInCrosshair(pN)||
                   IsTargetInCrosshair(pLF)||IsTargetInCrosshair(pRF)){
                    mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
                    Sleep(Cheats::AimAssist::Triggerbot::Delay);
                    mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
                }
            }
        }
        Sleep(10);
    }
}

// ============================================================
//  Teleport helpers
// ============================================================
void TeleportObject(uintptr_t obj, uintptr_t nav, uintptr_t mi, Vector3 pos, bool stop) {
    float magic=0.f;
    if (stop&&mi){ magic=ReadMemory<float>(mi+0x2C); WriteMemory<float>(mi+0x2C,0.f); }
    WriteMemory<Vector3>(obj+0x90, pos);
    WriteMemory<Vector3>(nav+0x50, pos);
    if (stop&&mi){ std::this_thread::sleep_for(std::chrono::milliseconds(40)); WriteMemory<float>(mi+0x2C,magic); }
}

void PositionTeleport(Vector3 pos) {
    if (!LocalPlayer.Pointer) return;
    uintptr_t nav=ReadMemory<uintptr_t>(LocalPlayer.Pointer+0x30);
    uintptr_t mi =ReadMemory<uintptr_t>(LocalPlayer.Pointer+0x20);
    if (nav&&mi) TeleportObject(LocalPlayer.Pointer,nav,mi,pos,true);
}

void TeleportWaypoint() {
    for (int i=0;i<2000;++i){
        uint64_t blip=ReadMemory<uint64_t>(Offsets.GameBase+Offsets.Waypoint+i*8);
        if(!blip) continue;
        if(ReadMemory<int>(blip+0x40)!=8||ReadMemory<int>(blip+0x48)!=84) continue;
        Vector2 wp=ReadMemory<Vector2>(blip+0x10);
        if(wp.x!=0||wp.y!=0){ PositionTeleport(Vector3(wp.x,wp.y,-210.f)); break; }
    }
}

// ============================================================
//  Exploits thread
// ============================================================
void Exploits() {
    static bool magicInit=false; static float magicVal=0.f; static bool noClipOn=false;
    static bool iaA=false; static float oA=0.f;
    static bool nrA=false; static float oR=0.f;
    static bool nsA=false; static float oS=0.f;
    static bool nreA=false; static float oRe=0.f;
    static bool nraA=false; static float oRa=0.f;
    static bool ebA=false; static int   oEb=0;
    static bool rfA=false; static float oRf=0.f;

    while (true) {
        if (!keepRunning || !g_gameConnected) { Sleep(50); continue; }
        if (!LocalPlayer.Pointer) { Sleep(10); continue; }

        if (!magicInit) {
            uintptr_t mi=ReadMemory<uintptr_t>(LocalPlayer.Pointer+0x20);
            if(mi){ magicVal=ReadMemory<float>(mi+0x2C); magicInit=true; }
        }

        // NoClip
        if (Cheats::World::NoClip::Enabled) {
            noClipOn=true;
            uintptr_t cam=ReadMemory<uintptr_t>(ReadMemory<uintptr_t>(Offsets.GameBase+Offsets.BlipList)+0x3C0);
            Vector3 camPos=ReadMemory<Vector3>(cam+0x40);
            Vector3 plPos=ReadMemory<Vector3>(LocalPlayer.Pointer+0x90);
            uintptr_t nav=ReadMemory<uintptr_t>(LocalPlayer.Pointer+0x30);
            Vector3 fwd=Normalize({camPos.x-plPos.x,camPos.y-plPos.y,camPos.z-plPos.z});
            Vector3 right=Normalize(fwd.Cross({0,0,1}));
            float spd=Cheats::World::NoClip::MovementSpeed*0.1f;
            Vector3 np=plPos;
            if(GetAsyncKeyState(Cheats::World::NoClip::ForwardKey)) np=np-right*spd;
            if(GetAsyncKeyState(Cheats::World::NoClip::BackwardKey))np=np+right*spd;
            if(GetAsyncKeyState(Cheats::World::NoClip::LeftKey))    np=np-fwd*spd;
            if(GetAsyncKeyState(Cheats::World::NoClip::RightKey))   np=np+fwd*spd;
            if(GetAsyncKeyState(Cheats::World::NoClip::UpKey))      np=np+Vector3(0,0,1)*spd;
            if(GetAsyncKeyState(Cheats::World::NoClip::DownKey))    np=np-Vector3(0,0,1)*spd;
            TeleportObject(LocalPlayer.Pointer,nav,0,np,false);
        } else if (noClipOn) {
            uintptr_t mi=ReadMemory<uintptr_t>(LocalPlayer.Pointer+0x20);
            if(mi) WriteMemory<float>(mi+0x2C,magicVal);
            noClipOn=false;
        }

        // Weapon mods
        uintptr_t wm=ReadMemory<uintptr_t>(LocalPlayer.Pointer+Offsets.WeaponManager);
        uintptr_t wi=ReadMemory<uintptr_t>(wm+0x20);
        if(wi){
            auto mod=[&](bool en,bool&ap,float&old,uintptr_t off,float val){
                if(en){if(!ap){old=ReadMemory<float>(wi+off);ap=true;}WriteMemory<float>(wi+off,val);}
                else if(ap){WriteMemory<float>(wi+off,old);ap=false;}
            };
            mod(Cheats::AimAssist::Settings::NoRecoil, nrA, oR, 0x2F4, 0.f);
            mod(Cheats::AimAssist::Settings::NoSpread, nsA, oS, 0x84,  0.f);
            mod(Cheats::AimAssist::Settings::NoReload, nreA,oRe,0x134,1000.f);
            mod(Cheats::AimAssist::Settings::NoRange,  nraA,oRa,0x28C,1000.f);

            if(Cheats::AimAssist::Settings::InfiniteAmmo){
                uintptr_t ai=ReadMemory<uintptr_t>(wi+0x60);
                uintptr_t ac=ReadMemory<uintptr_t>(ai+0x8);
                uintptr_t ac2=ReadMemory<uintptr_t>(ac+0x0);
                if(ac2){if(!iaA){oA=ReadMemory<float>(ac2+0x18);iaA=true;}WriteMemory<float>(ac2+0x18,30.f);}
            } else if(iaA){
                uintptr_t ai=ReadMemory<uintptr_t>(wi+0x60);
                uintptr_t ac=ReadMemory<uintptr_t>(ai+0x8);
                uintptr_t ac2=ReadMemory<uintptr_t>(ac+0x0);
                if(ac2) WriteMemory<float>(ac2+0x18,oA);
                iaA=false;
            }
        }

        if(Cheats::Vehicles::VehicleFix::Enabled&&(GetAsyncKeyState(Cheats::Vehicles::VehicleFix::HotKey)&1)){
            uintptr_t veh=ReadMemory<uintptr_t>(LocalPlayer.Pointer+Offsets.Vehicle);
            if(veh) WriteMemory<uint8_t>(veh+0x972,0x17);
        }

        if(Cheats::Players::StatusBars::HealthBoost::Enabled&&
           (GetAsyncKeyState(Cheats::Players::StatusBars::HealthBoost::HotKey)&1)){
            float hp=ReadMemory<float>(LocalPlayer.Pointer+Offsets.Health);
            float nh=hp+(float)Cheats::Players::StatusBars::HealthBoost::Value;
            if(nh>199.f)nh=198.f;
            WriteMemory<float>(LocalPlayer.Pointer+Offsets.Health,nh);
        }
        if(Cheats::Players::StatusBars::ArmorBoost::Enabled&&
           (GetAsyncKeyState(Cheats::Players::StatusBars::ArmorBoost::HotKey)&1)){
            float ar=ReadMemory<float>(LocalPlayer.Pointer+Offsets.Armor);
            float na=ar+(float)Cheats::Players::StatusBars::ArmorBoost::Value;
            if(na>99.f)na=98.f;
            WriteMemory<float>(LocalPlayer.Pointer+Offsets.Armor,na);
        }

        if(Cheats::World::SemiGodMode::Enabled){
            float hp=ReadMemory<float>(LocalPlayer.Pointer+Offsets.Health);
            if(hp<170.f) WriteMemory<float>(LocalPlayer.Pointer+Offsets.Health,198.f);
        }

        // Player info features (SuperSprint / Stamina / NoWanted)
        uintptr_t pi2 = ReadMemory<uintptr_t>(LocalPlayer.Pointer + Offsets.PlayerInfo);
        if (pi2) {
            if (Cheats::World::SuperSprint::Enabled)
                WriteMemory<float>(pi2 + 0x1A4, Cheats::World::SuperSprint::Speed * 0.3f);
            if (Cheats::World::InfiniteStamina::Enabled)
                WriteMemory<float>(pi2 + 0x294, 100.f);
        }

        // Weapon-based features using already-fetched wi
        if (wi) {
            auto modI=[&](bool en,bool&ap,int&old2,uintptr_t off,int val){
                if(en){if(!ap){old2=ReadMemory<int>(wi+off);ap=true;}WriteMemory<int>(wi+off,val);}
                else if(ap){WriteMemory<int>(wi+off,old2);ap=false;}
            };
            auto modF2=[&](bool en,bool&ap,float&old2,uintptr_t off,float val){
                if(en){if(!ap){old2=ReadMemory<float>(wi+off);ap=true;}WriteMemory<float>(wi+off,val);}
                else if(ap){WriteMemory<float>(wi+off,old2);ap=false;}
            };
            modI (Cheats::World::ExplosiveBullets::Enabled, ebA, oEb, 0x120, 0x4000000);
            modF2(Cheats::World::RapidFire::Enabled,        rfA, oRf, 0x128, 0.001f);
            if (Cheats::World::FireBullets::Enabled) WriteMemory<BYTE>(wi+0x220, 1);
            else                                     WriteMemory<BYTE>(wi+0x220, 0);
        }

        Sleep(10);
    }
}
