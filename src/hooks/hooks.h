#include <MinHook.h>

#include "CombatAimFix/CombatAimFix.h"

namespace Hooks {
    void Install() { 
        MH_Initialize();

        CombatAimFix::Install();

        MH_EnableHook(MH_ALL_HOOKS);
        return;
    }
}