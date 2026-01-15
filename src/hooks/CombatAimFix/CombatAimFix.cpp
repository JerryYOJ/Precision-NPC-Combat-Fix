#include "CombatAimFix.h"

#include <MinHook.h>

void CombatAimFix::Install() {

	REL::Relocation<std::uintptr_t> hk{ RELOCATION_ID(43154, 44376) };
	MH_CreateHook((LPVOID)hk.address(), Update, (LPVOID*)&_Update);

	REL::Relocation<std::uintptr_t> hk2{ RELOCATION_ID(36936, 37961) };
	MH_CreateHook((LPVOID)hk2.address(), GetPitch, (LPVOID*)&_GetPitch);

}

static RE::Actor* GetTarget(RE::CombatAimController* controller) {
	auto&& target = controller->target.get().get();

	if (!target) {
		auto&& cc = controller->combat_control;
		if (cc)
			target = cc->targetHandle.get().get();
	}

	return target;
}

void CombatAimFix::Update(RE::CombatMeleeAimController* thiz){
	auto&& target = GetTarget(thiz);
	
	if (!target) return _Update(thiz);

	RE::NiPoint3 backUp{};
	backUp = target->data.location;

	auto&& obj = target->Get3D();
	if (obj) {
		auto&& spine = obj->GetObjectByName("NPC Pelvis [Pelv]");
		if (spine) target->data.location = spine->world.translate;
	}

	_Update(thiz);

	target->data.location = backUp;

	return;
}

float CombatAimFix::GetPitch(RE::Actor* thiz)
{
	if (thiz->IsPlayerRef()) return _GetPitch(thiz);
	else {
		return -thiz->GetAngleX();
	}
}
