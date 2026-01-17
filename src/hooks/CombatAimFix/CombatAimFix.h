#pragma once

#include "../../RE/CombatAimController.h"
#include "../../RE/CombatMeleeAimController.h"
#include "../../RE/CombatBehaviorContextMelee.h"

class CombatAimFix : HookTemplate<CombatAimFix> {
public:
	static void Install();
protected:
	//static void Update(RE::CombatMeleeAimController* thiz);
	static float GetPitch(RE::Actor*);
private:

	//inline static REL::Relocation<decltype(Update)> _Update;
	inline static REL::Relocation<decltype(GetPitch)> _GetPitch;
};