#pragma once

#include "SDK.hpp"
using namespace SDK;

using UFHook = std::function<bool(UObject*, UFunction*, void*)>;
static std::unordered_map<std::string, UFHook> hooks;

typedef void(__thiscall* ProcessEvent)(UObject* Object, UFunction* Function, void* Parms);
static ProcessEvent oProcessEvent = nullptr;
static bool ShouldDoLogging = false;

static std::vector<std::string> pe_blacklist = {
    "Animation",
    "Tick",
    "Paint",
    "Draw",
    "ReadyToEndMatch",
    "BeginPlay",
    "BlueprintModify",
    "BlueprintUpdate",
    "ReplicatedWorldTime",
    "Destruct",
    "Anim",
    "OnMouse",
    "IsInteractable",
    "HandleKey",
    "Dissolve",
    "OnBind",
    "Dissolve__",
    "EndPlay",
    "WBP_",
    "Widget",
    "BP_Menu"
};

// THIS IS AN INTERNAL FUNCTION SHOULD NOT BE USED OUTSIDE OF THIS CLASS!
static void INTERNAL_FUNCTION_AddHook(const std::string& functionName, UFHook hook)
{
    hooks[functionName] = hook;
}

// THIS IS AN INTERNAL FUNCTION SHOULD NOT BE USED OUTSIDE OF THIS CLASS!
static void INTERNAL_FUNCTION_RemoveHook(const std::string& functionName)
{
    auto it = hooks.find(functionName);
    if (it != hooks.end())
    {
        hooks.erase(it);
    }
}

// Allows you to define a function to hook and code to run using a lambda function. Return Values -> (True = RunOriginalFunction / False = DoNotRunOriginalFunction)
#define KPE_AddHook(FunctionName, HookBody) \
        INTERNAL_FUNCTION_AddHook(FunctionName, [](UObject* Object, UFunction* Function, void* Parms) -> bool HookBody)

// Allows you to remove a hook by function name.
#define KPE_RemoveHook(FunctionName) \
        INTERNAL_FUNCTION_RemoveHook(FunctionName)

namespace KPE
{
    // Set's if ProcessEvent Hook should output events, can be filtered with SetLogBlacklist().
    static void EnableLogs(bool Enabled)
    {
        ShouldDoLogging = Enabled;
    }

    // Overrides the Blacklist that ProcessEvent Logs will use for output. (REQUIRES EnableLogs TO BE TRUE)
    static void SetLogBlacklist(std::vector<std::string> newBlacklist)
    {
        pe_blacklist = newBlacklist;
    }

    // THIS IS AN INTERNAL FUNCTION SHOULD NOT BE USED OUTSIDE OF THIS CLASS!
    static void INTERNAL_FUNCTION_PE_Hook(UObject* Object, UFunction* Function, void* Parms)
    {
        auto name = Function->GetFullName();
        auto objName = Object->GetName();

        
        auto blacklisted = false;
        for (int i = 0; i < pe_blacklist.size(); i++)
        {
            if (name.contains(pe_blacklist[i]) || objName.contains(pe_blacklist[i]))
            {
                blacklisted = true;
                break;
            }
        }

        if (!blacklisted && ShouldDoLogging)
        {
            std::cout << "[PE] [" << objName << "] " << name << "\n";
        }
        
        if (hooks.find(name) != hooks.end())
        {
            auto& hook = hooks[name];
            if (hook(Object, Function, Parms))
            {
                return oProcessEvent(Object, Function, Parms);
            }
        }
        else
        {
            return oProcessEvent(Object, Function, Parms);
        }
    }

    // Enables ProcessEvent Hook.
	static void Enable()
	{
        void** Engine_vTable = (void**)UEngine::GetEngine()->VTable;
        oProcessEvent = reinterpret_cast<decltype(oProcessEvent)>(Engine_vTable[Offsets::ProcessEventIdx]);
        for (int i = 0; i < UObject::GObjects->Num(); i++)
        {
            UObject* Obj = UObject::GObjects->GetByIndex(i);

            if (!Obj)
                continue;

            if (!Obj->IsDefaultObject())
                continue;

            void** vTable = reinterpret_cast<void**>(Obj->VTable);
            DWORD oldProtect;
            if (VirtualProtect(&vTable[Offsets::ProcessEventIdx], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect))
            {
                if (vTable && vTable[Offsets::ProcessEventIdx] != nullptr)
                {
                    vTable[Offsets::ProcessEventIdx] = reinterpret_cast<void*>(&INTERNAL_FUNCTION_PE_Hook);
                    VirtualProtect(&vTable[Offsets::ProcessEventIdx], sizeof(void*), oldProtect, &oldProtect);
                }
            }
        }
	}

    // Disables ProcessEvent Hook.
    static void Disable()
    {
        for (int i = 0; i < UObject::GObjects->Num(); i++)
        {
            UObject* Obj = UObject::GObjects->GetByIndex(i);

            if (!Obj)
                continue;

            if (!Obj->IsDefaultObject())
                continue;

            void** vTable = reinterpret_cast<void**>(Obj->VTable);
            DWORD oldProtect;
            if (VirtualProtect(&vTable[Offsets::ProcessEventIdx], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect))
            {
                if (vTable && vTable[Offsets::ProcessEventIdx] != nullptr)
                {
                    vTable[Offsets::ProcessEventIdx] = reinterpret_cast<void*>(oProcessEvent);
                    VirtualProtect(&vTable[Offsets::ProcessEventIdx], sizeof(void*), oldProtect, &oldProtect);
                }
            }
        }
    }
}
