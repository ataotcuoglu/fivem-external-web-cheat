template<class T>
T ReadMemory(uintptr_t address) {
    if (!address || !Game.hProcess) return T{};
    T out{};
    ReadProcessMemory(Game.hProcess, reinterpret_cast<LPVOID>(address), &out, sizeof(T), nullptr);
    return out;
}

template<class T>
void WriteMemory(uintptr_t address, T value) {
    if (!address || !Game.hProcess) return;
    WriteProcessMemory(Game.hProcess, reinterpret_cast<LPVOID>(address), &value, sizeof(T), nullptr);
}

void WriteBytes(uintptr_t address, uint8_t* patch, size_t size) {
    if (!address || !patch || !size || !Game.hProcess) return;
    DWORD old = 0;
    VirtualProtectEx(Game.hProcess, reinterpret_cast<LPVOID>(address), size,
                     PAGE_EXECUTE_READWRITE, &old);
    WriteProcessMemory(Game.hProcess, reinterpret_cast<LPVOID>(address), patch, size, nullptr);
    if (old) VirtualProtectEx(Game.hProcess, reinterpret_cast<LPVOID>(address), size, old, &old);
}

std::string ReadString(uintptr_t addr) {
    if (!addr) return {};
    std::string out;
    out.reserve(32);
    for (int i = 0; i < 255; ++i) {
        char c = ReadMemory<char>(addr + i);
        if (!c) break;
        out += c;
    }
    return out;
}

uintptr_t GetBaseAddress() {
    MODULEENTRY32W me = {}; me.dwSize = sizeof(me);
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, Game.pID);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    uintptr_t base = 0;
    if (Module32FirstW(snap, &me))
        base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
    CloseHandle(snap);
    return base;
}

uintptr_t GetBaseAddress(const std::string& moduleName) {
    MODULEENTRY32W me = {}; me.dwSize = sizeof(me);
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, Game.pID);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    std::wstring wName(moduleName.begin(), moduleName.end());
    uintptr_t base = 0;
    if (Module32FirstW(snap, &me)) {
        do {
            if (_wcsicmp(me.szModule, wName.c_str()) == 0) {
                base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                break;
            }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return base;
}
