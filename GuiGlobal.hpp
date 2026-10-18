// ============================================================
//  GuiGlobal  —  ImGui renk ayarlari + Cheats:: namespace
// ============================================================

namespace GuiSetting {
    inline ImVec2 Size                  = { 960, 600 };
    inline ImVec4 Color1;
    inline ImVec4 Color2;
    inline ImVec4 Color3;
    inline ImVec4 Color4;
    inline ImVec4 Background            = ImColor(22, 22, 28, 255);
    inline ImVec4 InputBg               = ImColor(30, 30, 40, 255);
    inline ImVec4 TextActive            = ImColor(255, 255, 255, 255);
    inline ImVec4 Shadow                = ImColor(40, 42, 56, 80);
    inline ImVec4 DecorativeRectBackground;
    inline ImVec4 ChildRect;
    inline ImVec4 Black;
    inline ImVec4 Black1;
    inline ImVec4 BlackIn;
    inline ImVec4 BorderChild;
    inline ImVec4 LineChild;
    inline ImVec4 ShadowTab;
    inline ImVec4 CheckboxBackground;
    inline ImVec4 CheckboxInBackground;
    inline ImVec4 CircleCheckbox;
    inline ImVec4 CircleCheckboxIn;
    inline ImVec4 Separator;
    inline ImVec4 SwitchInActive;
    inline ImVec4 SwitchText;
    inline ImVec4 SwitchTextIn;
    // Ek renkler (GuiItems / UpdateTheme tarafindan kullanilir)
    inline ImVec4 Accent;
    inline ImVec4 AccentDim;
    inline ImVec4 WindowBg;
    inline ImVec4 PanelBg;
    inline ImVec4 ChildBg;
    inline ImVec4 Border;
    inline ImVec4 CheckboxBg;
    inline ImVec4 CheckboxFill;
    inline ImVec4 SwitchOff;
    inline ImVec4 SwitchOn;
    inline ImVec4 TextMuted             = ImColor(160, 160, 170, 255);
}

// ============================================================
//  Cheats:: — tum hile ayarlari
//  NOT: 'inline' ekli — GuiGlobal.hpp birden fazla TU'ya dahil
//  edildiginden ODR ihlalini onlemek icin gerekli (C++17)
// ============================================================
namespace Cheats {
    namespace AimAssist {
        namespace Settings {
            inline bool Crosshair              = false;
            inline int  CrosshairSelectedType  = 0;
            inline const char* CrosshairType[10]{ "Type 1","Type 2","Type 3","Type 4","Type 5",
                                                   "Type 6","Type 7","Type 8","Type 9","Type 10" };
            inline ImColor CrosshairColor      = ImColor(255,255,255);
            inline int  CrosshairSize          = 15;
            inline bool DynamicCrosshairColor  = false;
            inline bool IgnorePed              = true;
            inline bool IgnoreDeath            = true;
            inline bool IgnoreInvisible        = false;
            inline bool InfiniteAmmo           = false;
            inline bool NoRecoil               = false;
            inline bool NoSpread               = false;
            inline bool NoReload               = false;
            inline bool NoRange                = false;
        }

        namespace Silent {
            inline bool Enabled     = false;
            inline int  HotKey      = 0;
            inline int  Fov         = 150;
            inline int  BoneMode    = 0;
            inline int  MaxDistance = 300;
            inline bool DrawFov     = false;
            inline ImColor FovColor = ImColor(100,220,180);
            inline int  FovWeight   = 1;
            inline bool VisCheck    = false;
            inline int  MissChance  = 0;
            inline bool SkipFriends = true;
            inline bool DrawTarget  = false;
            inline int  SelectedDrawTargetType = 0;
            inline ImColor DrawTargetColor = ImColor(255,80,80,200);
        }

        namespace MagicBullet {
            inline bool Enabled     = false;
            inline int  BoneMode    = 0;   // 0=Kafa, 1=Gogus, 2=Bacak, 3=Rastgele
            inline int  MaxDistance = 600;
            inline bool SkipFriends = true;
            inline bool SkipNPC     = true;
        }

        namespace Aimbot {
            inline bool Enabled     = false;
            inline int  HotKey      = 0;
            inline int  Fov         = 150;
            inline int  Smooth      = 5;
            inline int  BoneMode    = 0;
            inline int  MaxDistance = 300;
            inline bool StickyAim   = true;
            inline int  AimMode     = 0;
            inline int  Priority    = 0;
            inline bool DrawFov     = false;
            inline ImColor FovColor = ImColor(100,200,255);
            inline int  FovWeight   = 1;
            inline bool VisCheck    = false;
            inline bool SkipFriends = true;
            inline bool DrawTarget  = false;
            inline int  SelectedDrawTargetType = 0;
            inline ImColor DrawTargetColor = ImColor(80,200,255,200);
        }

        namespace Triggerbot {
            inline bool Enabled      = false;
            inline int  HotKey       = 0;
            inline int  Delay        = 5;
            inline int  MaxDistance  = 300;
            inline bool SkipFriends  = true;
        }
    }

    namespace Players {
        namespace Settings {
            inline bool IgnorePed      = true;
            inline bool IgnoreDeath    = true;
            inline bool IgnoreInvisible= false;
            inline int  MaxDistance    = 300;
        }

        namespace VisCheck {
            inline bool Enabled          = false;
            inline ImColor VisibleColor  = ImColor( 80,220,120,255);  
            inline ImColor HiddenColor   = ImColor(220, 70, 70,255);  
        }

        namespace VisualMarkers {
            namespace GlobalSettings {
                inline int  SelectedLineType   = 1;
                inline const char* LineTypes[2]{ "Basic","Outlined" };
                inline int  LineWeight         = 2;
            }
            namespace DrawSkeleton {
                inline bool Enabled    = false;
                inline ImColor Color   = ImColor(255,255,255);
            }
            namespace DrawBox {
                inline bool Enabled    = false;
                inline ImColor Color   = ImColor(255,155,0);
                inline int  SelectedType = 0;
                inline const char* Types[3]{ "Corner","2D","3D" };
            }
            namespace DrawLine {
                inline bool Enabled            = false;
                inline ImColor Color           = ImColor(69,158,255);
                inline int  SelectedLocation   = 2;
                inline const char* Locations[3]{ "Top","Center","Bottom" };
            }
            namespace DrawBonePoints {
                inline bool    Enabled = false;
                inline ImColor Color   = ImColor(255,200,60,220);
                inline int     Radius  = 3;
            }
        }

        namespace PlayerInfo {
            namespace GlobalSettings {
                inline int  SelectedFont       = 0;
                inline const char* Fonts[2]    = { "Roboto","Roboto Bold" };
                inline int  SelectedFontType   = 0;
                inline const char* FontTypes[2]{ "Basic","Outlined" };
                inline int  MaxDistance        = 70;
            }
            namespace DrawId {
                inline bool Enabled            = false;
                inline ImColor Color           = ImColor(255,255,255);
                inline int  SelectedLocation   = 0;
                inline const char* Locations[2]{ "Top","Bottom" };
            }
            namespace DrawName {
                inline bool Enabled            = false;
                inline ImColor Color           = ImColor(5,255,132);
                inline int  SelectedLocation   = 0;
                inline const char* Locations[2]{ "Top","Bottom" };
            }
            namespace DrawWeaponName {
                inline bool Enabled            = false;
                inline ImColor Color           = ImColor(255,255,255);
                inline int  SelectedLocation   = 1;
                inline const char* Locations[2]{ "Top","Bottom" };
            }
            namespace DrawDistance {
                inline bool Enabled            = false;
                inline ImColor Color           = ImColor(255,97,220);
                inline int  SelectedLocation   = 1;
                inline const char* Locations[2]{ "Top","Bottom" };
            }
        }

        namespace StatusBars {
            namespace GlobalSettings {
                inline int MaxHealth = 200;
                inline int MaxArmor  = 100;
            }
            namespace DrawHealthBar {
                inline bool Enabled            = false;
                inline int  SelectedLocation   = 0;
                inline const char* Locations[2]{ "Left","Right" };
            }
            namespace HealthBoost {
                inline bool Enabled = false;
                inline int  Value   = 50;
                inline int  HotKey  = 0;
            }
            namespace DrawArmorBar {
                inline bool Enabled            = false;
                inline int  SelectedLocation   = 1;
                inline const char* Locations[2]{ "Left","Right" };
            }
            namespace ArmorBoost {
                inline bool Enabled = false;
                inline int  Value   = 50;
                inline int  HotKey  = 0;
            }
        }
    }

    namespace Vehicles {
        namespace Settings {
            inline bool IgnoreLocalVehicle = true;
            inline int  MaxVehicleCount    = 500;
            inline int  MaxDistance        = 300;
        }
        namespace DrawPoint {
            inline bool Enabled            = false;
            inline ImColor Color           = ImColor(239,255,21);
            inline int  SelectedLineType   = 1;
            inline const char* LineTypes[2]{ "Basic","Outlined" };
            inline int  Size               = 7;
        }
        namespace DrawLine {
            inline bool Enabled            = false;
            inline ImColor Color           = ImColor(123,166,7);
            inline int  SelectedLocation   = 2;
            inline const char* Locations[3]{ "Top","Center","Bottom" };
            inline int  SelectedLineType   = 1;
            inline const char* LineTypes[2]{ "Basic","Outlined" };
        }
        namespace DrawDistance {
            inline bool Enabled            = false;
            inline ImColor Color           = ImColor(255,255,255);
            inline int  SelectedFont       = 0;
            inline const char* Fonts[2]    = { "Roboto","Roboto Bold" };
            inline int  SelectedFontType   = 1;  
            inline const char* FontTypes[2]{ "Basic","Outlined" };
            inline int  MaxDistance        = 70;
        }
        namespace DrawHealthBar { inline bool Enabled = false; }
        namespace VehicleFix {
            inline bool Enabled = false;
            inline int  HotKey  = 0;
        }
    }

    namespace World {
        namespace NoClip {
            inline bool Enabled        = false;
            inline int  MovementSpeed  = 5;
            inline int  ForwardKey     = 0x57; // W
            inline int  BackwardKey    = 0x53; // S
            inline int  LeftKey        = 0x41; // A
            inline int  RightKey       = 0x44; // D
            inline int  UpKey          = 0x20; // Space
            inline int  DownKey        = 0xA2; // Ctrl
        }
        namespace SemiGodMode    { inline bool Enabled = false; }
        namespace SuperSprint    { inline bool Enabled = false; inline int  Speed = 5; }
        namespace InfiniteStamina{ inline bool Enabled = false; }
        namespace ExplosiveBullets{ inline bool Enabled = false; }
        namespace FireBullets    { inline bool Enabled = false; }
        namespace RapidFire      { inline bool Enabled = false; }

        namespace Teleport {
            struct TeleportLocation { const char* Name; float x, y, z; };
            inline const TeleportLocation Locations[] = {
                { "Legion Square",              190.52f,   -873.23f,  31.5f  },
                { "Paleto Bay",                -138.52f,   6356.99f,  31.49f },
                { "Main LS Customs",           -365.43f,   -131.81f,  37.87f },
                { "IAA Roof",                   134.09f,   -637.86f, 262.85f },
                { "FIB Roof",                  -150.13f,   -754.59f, 262.87f },
                { "Maze Bank",                  -75.02f,   -818.22f, 326.18f },
                { "Mount Chiliad",              495.0f,    5589.0f,  795.0f  },
                { "Casino",                     911.96f,     38.34f,  80.72f },
                { "Prison",                    1702.08f,   2650.51f,  45.56f },
                { "Military Base",            -2751.12f,   3316.4f,   32.81f },
                { "Void",                     15000.0f,  15000.0f,    0.0f   },
                { "Hospital Central LS",        339.85f,  -1394.56f,  32.51f },
                { "Pillbox Medical",            307.87f,   -595.55f,  43.28f },
                { "Sandy Shores Medical",      1839.6f,   3672.93f,   34.28f },
                { "Lester's House",            1273.9f,  -1719.3f,   54.77f  },
                { "Pacific Standard Vault",     255.85f,    217.03f, 101.68f },
                { "Police Station",             436.49f,   -982.17f,  30.70f },
                { "Humane Labs",               3619.75f,   2742.74f,  28.69f },
                { "Floyd's Apartment",        -1150.7f,  -1520.7f,   10.63f  },
                { "Trevor's Meth Lab",         1391.77f,   3608.72f,  38.94f },
            };
            inline int SelectedIndex = 0;
        }
    }

    namespace Settings {
        inline bool StreamProof    = false;
        inline int  MenuKey        = 45;  
        inline int  PanicKey       = 123; 
        inline int  MaxPlayerCount = 500;
        inline int  SelectedTheme  = 1;  
    }
}


#include <vector>
#include <string>
struct Toast { std::string msg; float timer; ImU32 color; };
inline std::vector<Toast> g_toasts;
inline void PushToast(const char* msg,
                      ImU32 col = IM_COL32(100,220,140,255),
                      float dur  = 2.5f) {
    (void)msg; (void)col; (void)dur; 
}
