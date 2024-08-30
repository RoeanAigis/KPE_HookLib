# PE Hook Library

This is a powerful and flexible Process Event Hooking Library designed for use with Dumper-7 SDK projects. The library allows you to hook into specific `UFunction` calls and control existing parameters aswell as returning the original or completely overriding logic within said function.

## Features

- **Add Hooks:** Hook into specific `UFunction` calls with custom logic using lambda functions.
- **Remove Hooks:** Easily remove hooks by function name.
- **Logging:** Enable or disable logging of hooked events, with customizable filtering through a blacklist.
- **Enable/Disable Hooks:** Easily enable or disable the hooking mechanism.

## Installation

To use this library in your project, include the `KEI_PE_HOOK.hpp` file in your project:

```cpp
#include "KEI_PE_HOOK.hpp"
```

## Usage

### Adding Hooks

You can add a hook to any `UFunction` by using the `KPE_AddHook` macro. The hook body is a lambda function that receives the `UObject`, `UFunction`, and parameters `void* Parms`. Return `true` if you want the original function to be called, or `false` if you want to skip it.

```cpp
KPE_AddHook("FunctionName", {
    // Your custom logic here.
    // Return true to run the original function.
    // Return false to skip the original function.
    return true;
});
```

### Removing Hooks

To remove a hook, use the `KPE_RemoveHook` macro and provide the function name:

```cpp
KPE_RemoveHook("FunctionName");
```

### Enabling/Disabling Logging

To enable or disable logging of `ProcessEvent` hooks, use the `EnableLogs` function. Logging is only effective if it is enabled and the function name is not blacklisted.

```cpp
KPE::EnableLogs(true);  // Enable logging
KPE::EnableLogs(false); // Disable logging
```

### Customizing the Log Blacklist

The log blacklist determines which functions should not be logged. You can customize this list using the `SetLogBlacklist` function. This function requires logging to be enabled.

```cpp
std::vector<std::string> customBlacklist = {
    "Tick",
    "BeginPlay"
};

KPE::SetLogBlacklist(customBlacklist);
```

### Enabling/Disabling the Hook

To enable the hook and start intercepting `ProcessEvent` calls, use the `Enable` function:

```cpp
KPE::Enable();
```

To disable the hook, use the `Disable` function:

```cpp
KPE::Disable();
```

## Internal Functions

**Note:** The following functions are intended for internal use only and should not be called directly:

- `INTERNAL_FUNCTION_AddHook`
- `INTERNAL_FUNCTION_RemoveHook`
- `INTERNAL_FUNCTION_PE_Hook`

## Examples

### Template Main.cpp
```c++
#include <Windows.h>
#include <iostream>

DWORD MainThread(HMODULE Module)
{
        /* Code to open a console window */
        AllocConsole();
        FILE* Dummy;
        freopen_s(&Dummy, "CONOUT$", "w", stdout);
        freopen_s(&Dummy, "CONIN$", "r", stdin);

        // Your code here

        return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
        switch (reason)
        {
                case DLL_PROCESS_ATTACH:
                CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
                break;
        }

        return TRUE;
}
```

### Template using Library functions.
```cpp
#include <Windows.h>
#include <iostream>

#include "KEI_PE_HOOK.hpp"
#include "SDK.hpp"
#include "SDK/Engine_Parameters.hpp"

using namespace SDK;
using namespace SDK::Params;

static HMODULE mainThread;

DWORD MainThread(HMODULE Module)
{
    /* Code to open a console window */
    AllocConsole();
    FILE* Dummy;
    freopen_s(&Dummy, "CONOUT$", "w", stdout);
    freopen_s(&Dummy, "CONIN$", "r", stdin);  

    KPE::EnableLogs(false);
    KPE::SetLogBlacklist({ "Tick", "BeginPlay", "Paint", "Draw"});
    KPE::Enable();

    KPE_AddHook("Function Engine.GameModeBase.RestartPlayerAtPlayerStart", {
        auto params = (GameModeBase_RestartPlayerAtPlayerStart*)Parms;

        params->NewPlayer = UGameplayStatics::GetPlayerController(UWorld::GetWorld(), 0);

        std::cout << "Hooked RestartPlayerAtPlayerStart!\n";

        return true;
    });

    for (;;)
    {
        if (GetAsyncKeyState(VK_INSERT))
        {
            FreeLibraryAndExitThread(mainThread, 0);
        }
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
        mainThread = hModule;
        break;
    case DLL_PROCESS_DETACH:
        KPE::Disable();
    }

    return TRUE;
}
```
