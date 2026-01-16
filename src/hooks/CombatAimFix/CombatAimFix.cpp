#include "CombatAimFix.h"

#include <MinHook.h>

void CombatAimFix::Install() {

	//REL::Relocation<std::uintptr_t> hk{ RELOCATION_ID(43154, 44376) };
	//MH_CreateHook((LPVOID)hk.address(), Update, (LPVOID*)&_Update);

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

static float CalcPitch(RE::NiPoint3& source, RE::NiPoint3& target) {
	float dx = target.x - source.x;
	float dy = target.y - source.y;
	float dz = target.z - source.z;
	float distXY = sqrt(dx * dx + dy * dy);
	return atan2(dz, distXY) * 57.29578;
}

float CombatAimFix::GetPitch(RE::Actor* thiz)
{
	if(!thiz->IsPlayerRef()) {
		auto&& selfmodel = thiz->Get3D();
		if (!selfmodel) return _GetPitch(thiz);

		auto&& selfhead = selfmodel->GetObjectByName("NPC R Clavicle [RClv]");
		if (!selfhead) return _GetPitch(thiz);
		
		auto&& cc = thiz->GetActorRuntimeData().combatController;
		if(!cc) return _GetPitch(thiz);

		auto&& target = thiz->GetActorRuntimeData().combatController->targetHandle;
		if(!target) return _GetPitch(thiz);

		auto&& targetmodel = target.get()->Get3D();
		if (!targetmodel) return _GetPitch(thiz);

		auto&& targetpelvis = targetmodel->GetObjectByName("NPC Spine1 [Spn1]");
		if (!targetpelvis) return _GetPitch(thiz);

		auto&& headpos = selfhead->world.translate;
		auto&& pelvispos = targetpelvis->world.translate;

		thiz->SetGraphVariableFloat("PitchHalf_AF", CalcPitch(headpos, pelvispos) * 0.5);
	}
	//else thiz->SetGraphVariableFloat("PitchHalf_AF", _GetPitch(thiz) * 57.29578 * 0.3);
	return _GetPitch(thiz);
}
