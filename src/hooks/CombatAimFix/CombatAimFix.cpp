#include "CombatAimFix.h"

#include <MinHook.h>

#undef GetObject

void CombatAimFix::Install() {

	REL::Relocation<std::uintptr_t> hk2{ RELOCATION_ID(36936, 37961) };
	MH_CreateHook((LPVOID)hk2.address(), GetPitch, (LPVOID*)&_GetPitch);

}

static float CalcPitch(const RE::NiPoint3& source, const RE::NiPoint3& target) {
	float dx = target.x - source.x;
	float dy = target.y - source.y;
	float dz = target.z - source.z;
	float distXY = sqrt(dx * dx + dy * dy);
	return atan2(dz, distXY);
}

static float SoftCapAsymmetric(float value, float limitMin, float limitMax, float softness) {
	if (value > (limitMax - softness)) {
		float overflow = value - (limitMax - softness);
		return (limitMax - softness) + (softness * (overflow / (overflow + softness)));
	}
	else if (value < (limitMin + softness)) {
		float overflow = (limitMin + softness) - value;
		return (limitMin + softness) - (softness * (overflow / (overflow + softness)));
	}

	return value;
}

static RE::NiPoint3 GetClosestPointOnSegment(RE::NiPoint3 point, RE::NiPoint3 p1, RE::NiPoint3 p2) { // foot, head
	RE::NiPoint3 segment = p2 - p1;
	float lenSq = segment.SqrLength();
	if (lenSq < 1.0f) return p1; // Check for 0 length
	// Project vector (point - p1) onto segment
	RE::NiPoint3 pointVector = point - p1;
	float t = (pointVector.x * segment.x + pointVector.y * segment.y + pointVector.z * segment.z) / lenSq;
	// Clamp t to [0, 1] to stay on the body
	// t=0 is Pelvis, t=1 is Head
	t = std::clamp(t, 0.0f, 1.0f);
	return p1 + (segment * t);
}

static RE::NiPoint3 GetLazyAimTarget(RE::NiPoint3 root, RE::NiPoint3 torso, RE::NiPoint3 head, RE::NiPoint3 aimOrigin) {
	auto closest1 = GetClosestPointOnSegment(aimOrigin, root, torso);
	auto closest2 = GetClosestPointOnSegment(aimOrigin, torso, head);

	// Return whichever is closer (requires less twist)
	float dist1 = closest1.GetSquaredDistance(aimOrigin);
	float dist2 = closest2.GetSquaredDistance(aimOrigin);

	return (dist1 < dist2) ? closest1 : closest2;
}

static bool IsHumanoid(RE::TESObjectREFR* refr) {
	static auto&& dobj = RE::BGSDefaultObjectManager::GetSingleton();
	auto&& keywd = dobj->GetObject<RE::BGSKeyword>(RE::BGSDefaultObjectManager::DefaultObject::kKeywordNPC);
	if (keywd) {
		return refr->HasKeyword(keywd);
	}
	return false;
}

float CombatAimFix::GetPitch(RE::Actor* thiz)
{
	const static float LIMIT_MIN = -0.7f;
	const static float LIMIT_MAX = 0.7f;
	const static float SOFTNESS = 0.2f;
	
	if (thiz->IsPlayerRef()) {
		thiz->SetGraphVariableFloat("PitchHalf_AF", SoftCapAsymmetric(_GetPitch(thiz), LIMIT_MIN, LIMIT_MAX, SOFTNESS) * 57.29578 * 0.5);
	}
	else if (IsHumanoid(thiz)) {
		static REL::Relocation<bool(*)(RE::Actor*, bool)> IsDead{ RELOCATION_ID(36484, 37483) };
		
		auto&& cc = thiz->GetActorRuntimeData().combatController; if (!cc) goto ret;
		auto&& target = cc->targetHandle; if (!target || IsDead(target.get().get(), true)) goto ret;
		
		auto&& selfmodel = thiz->Get3D(); if (!selfmodel) goto ret;
		auto&& selfbone = selfmodel->GetObjectByName("NPC R Clavicle [RClv]"); if (!selfbone) goto ret;
		auto selfpos = selfbone->world.translate;

		if (IsHumanoid(target.get().get())) {
			auto&& targetmodel = target.get()->Get3D(); if (!targetmodel) goto ret;
			auto head = targetmodel->GetObjectByName("NPC Head [Head]");
			auto root = target.get()->data.location;

			thiz->SetGraphVariableFloat("PitchHalf_AF", SoftCapAsymmetric(CalcPitch(selfpos, GetClosestPointOnSegment(selfpos, root, head->world.translate)), LIMIT_MIN, LIMIT_MAX, SOFTNESS) * 57.29578 * 0.5);
		}
		else {
			auto&& targetmodel = target.get()->Get3D(); if (!targetmodel) goto ret;
			auto&& race = target.get()->GetRace(); if (!race) goto ret;
			auto* bodyPartData = race->bodyPartData; if (!bodyPartData) goto ret;
			auto* headpart = bodyPartData->parts[RE::BGSBodyPartDefs::LIMB_ENUM::kHead];
			auto* torsopart = bodyPartData->parts[RE::BGSBodyPartDefs::LIMB_ENUM::kTorso];
			if (!headpart || !torsopart) {
				thiz->SetGraphVariableFloat("PitchHalf_AF", SoftCapAsymmetric(CalcPitch(selfpos, target.get()->GetLookingAtLocation()), LIMIT_MIN, LIMIT_MAX, SOFTNESS) * 57.29578 * 0.5); //fallback
				goto ret;
			}
			auto* headbone = targetmodel->GetObjectByName(headpart->targetName);
			auto* torsobone = targetmodel->GetObjectByName(torsopart->targetName);
			if (!headbone || !torsobone) {
				thiz->SetGraphVariableFloat("PitchHalf_AF", SoftCapAsymmetric(CalcPitch(selfpos, target.get()->GetLookingAtLocation()), LIMIT_MIN, LIMIT_MAX, SOFTNESS) * 57.29578 * 0.5); //fallback
				goto ret;
			}

			auto&& root = target.get()->data.location;
			auto&& head = headbone->world.translate;
			auto&& torso = torsobone->world.translate;

			auto&& pitch = SoftCapAsymmetric(CalcPitch(selfpos, GetLazyAimTarget(root, torso, head, selfpos)), LIMIT_MIN, LIMIT_MAX, SOFTNESS);
			thiz->SetGraphVariableFloat("PitchHalf_AF", pitch * 57.29578 * 0.5);
		}
	}
ret:
	return _GetPitch(thiz);
}
