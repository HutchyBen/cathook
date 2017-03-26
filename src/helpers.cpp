/*
 * helpers.cpp
 *
 *  Created on: Oct 8, 2016
 *      Author: nullifiedcat
 */

#include "common.h"
#include "hooks.h"
#include "sdk.h"
#include "profiler.h"

#include <pwd.h>
#include <sys/mman.h>

std::vector<ConVar*> g_ConVars;
FILE* hConVarsFile = 0;
void BeginConVars() {
	hConVarsFile = fopen(strfmt("%scfg/cat_defaults.cfg", g_pszTFPath), "w");
	FILE* hAutoexec = fopen(strfmt("%scfg/cat_autoexec.cfg", g_pszTFPath), "r+");
	if (!hAutoexec) {
		hAutoexec = fopen(strfmt("%scfg/cat_autoexec.cfg", g_pszTFPath), "w");
		if (hAutoexec) {
			fprintf(hAutoexec, "// Put your custom cathook settings in this file\n// This script will be executed EACH TIME YOU INJECT CATHOOK\n");
			fclose(hAutoexec);
		}
	} else fclose(hAutoexec);
	FILE* hMatchexec = fopen(strfmt("%scfg/cat_matchexec.cfg", g_pszTFPath), "r+");
	if (!hMatchexec) {
		hMatchexec = fopen(strfmt("%scfg/cat_matchexec.cfg", g_pszTFPath), "w");
		if (hMatchexec) {
			fprintf(hMatchexec, "// Put your custom cathook settings in this file\n// This script will be executed EACH TIME YOU JOIN A MATCH\n");
			fclose(hMatchexec);
		}
	} else fclose(hMatchexec);
	if (hConVarsFile) fprintf(hConVarsFile, "// THIS FILE IS AUTO-GENERATED BY CATHOOK\n// DO NOT EDIT IT, ALL CHANGES WILL BE UNDONE!\n// If you want to change default settings, add changed convars to cat_autoexec.cfg\n");
	SetCVarInterface(g_ICvar);
}

void EndConVars() {
	RegisterCatVars();
	RegisterCatCommands();
	if (hConVarsFile) {
		fprintf(hConVarsFile, "\nexec cat_autoexec\n");
		fprintf(hConVarsFile, "cat_killsay_reload\ncat_spam_reload\n");
		fclose(hConVarsFile);
	}
	ConVar_Register();
}

// StackOverflow copypasta xddd
void ReplaceString(char* target, char* what, char* with_what) {
	char buffer[1024] = { 0 };
	char *insert_point = &buffer[0];
	const char *tmp = target;
	size_t needle_len = strlen(what);
	size_t repl_len = strlen(with_what);
	while (1) {
		const char *p = strstr(tmp, what);
		if (p == NULL) {
			strcpy(insert_point, tmp);
			break;
		}
		memcpy(insert_point, tmp, p - tmp);
		insert_point += p - tmp;
		memcpy(insert_point, with_what, repl_len);
		insert_point += repl_len;
		tmp = p + needle_len;
	}
	strcpy(target, buffer);
}

bool IsPlayerInvulnerable(CachedEntity* player) {
	return (HasCondition(player, TFCond_Ubercharged) || HasCondition(player, TFCond_UberchargedCanteen)
	 || HasCondition(player, TFCond_UberchargedHidden) || HasCondition(player, TFCond_UberchargedOnTakeDamage)
	 || HasCondition(player, TFCond_Bonked) || HasCondition(player, TFCond_DefenseBuffMmmph));
}

bool IsPlayerCritBoosted(CachedEntity* player) {
	return (HasCondition(player, TFCond_Kritzkrieged) || HasCondition(player, TFCond_CritRuneTemp)
		 || HasCondition(player, TFCond_CritCanteen) || HasCondition(player, TFCond_CritMmmph)
		 || HasCondition(player, TFCond_CritOnKill) || HasCondition(player, TFCond_CritOnDamage)
		 || HasCondition(player, TFCond_CritOnFirstBlood) || HasCondition(player, TFCond_CritOnWin)
		 || HasCondition(player, TFCond_CritRuneTemp) || HasCondition(player, TFCond_HalloweenCritCandy));
}

ConVar* CreateConVar(std::string name, std::string value, std::string help) {
	char* namec = new char[256];
	strncpy(namec, name.c_str(), 255);
	char* valuec = new char[256];
	strncpy(valuec, value.c_str(), 255);
	char* helpc = new char[256];
	strncpy(helpc, help.c_str(), 255);
	//logging::Info("Creating ConVar: %s %s %s", namec, valuec, helpc);
	ConVar* ret = new ConVar((const char*)namec, (const char*)valuec, 0, (const char*)helpc);
	if (hConVarsFile)
		fprintf(hConVarsFile, "%s \"%s\"\n", name.c_str(), value.c_str());
	g_ICvar->RegisterConCommand(ret);
	g_ConVars.push_back(ret);
	return ret;
}

ConCommand* CreateConCommand(const char* name, FnCommandCallback_t callback, const char* help) {
	ConCommand* ret = new ConCommand(name, callback, help);
	g_ICvar->RegisterConCommand(ret);
	return ret;
}

const char* GetBuildingName(CachedEntity* ent) {
	if (!ent) return "[NULL]";
	if (ent->m_iClassID == g_pClassID->CObjectSentrygun) return "Sentry";
	if (ent->m_iClassID == g_pClassID->CObjectDispenser) return "Dispenser";
	if (ent->m_iClassID == g_pClassID->CObjectTeleporter) return "Teleporter";
	return "[NULL]";
}

void format_internal(std::stringstream& stream) {}

std::string WordWrap(std::string& in, int max) {
	std::stringstream result;
	std::stringstream line;
	std::stringstream wordstream;
	std::stringstream next;
	for (int i = 0; i < in.size(); i++) {
		char ch = in.at(i);
		if (ch == ' ' || ch == '\n') {
			std::string word = wordstream.str();
			//logging::Info("got word: '%s'", word.c_str());
			wordstream.str("");
			auto size = draw::GetStringLength(fonts::MENU, line.str() + word);
			if (size.first >= max) {
				//logging::Info("wrapping: '%s'", line.str().c_str());
				result << line.str() << '\n';
				line.str("");
			}
			line << word << ch;
		} else {
			wordstream << ch;
		}
	}
	std::string word = wordstream.str();
	wordstream.str("");
	auto size = draw::GetStringLength(fonts::MENU, line.str() + word);
	if (size.first >= max) {
		result << line.str() << '\n';
		line.str(word);
	} else {
		line << word;
	}
	result << line.str();
	return result.str();
}

void ReplaceString(std::string& input, const std::string& what, const std::string& with_what) {
	size_t index = input.find(what);
	while (index != std::string::npos) {
		input.replace(index, what.size(), with_what);
		index = input.find(what, index + with_what.size());
	}
}

powerup_type GetPowerupOnPlayer(CachedEntity* player) {
	if (CE_BAD(player)) return powerup_type::not_powerup;
	if (HasCondition(player, TFCond_RuneStrength)) return powerup_type::strength;
	if (HasCondition(player, TFCond_RuneHaste)) return powerup_type::haste;
	if (HasCondition(player, TFCond_RuneRegen)) return powerup_type::regeneration;
	if (HasCondition(player, TFCond_RuneResist)) return powerup_type::resistance;
	if (HasCondition(player, TFCond_RuneVampire)) return powerup_type::vampire;
	if (HasCondition(player, TFCond_RuneWarlock)) return powerup_type::reflect;
	if (HasCondition(player, TFCond_RunePrecision)) return powerup_type::precision;
	if (HasCondition(player, TFCond_RuneAgility)) return powerup_type::agility;
	if (HasCondition(player, TFCond_RuneKnockout)) return powerup_type::knockout;
	if (HasCondition(player, TFCond_KingRune)) return powerup_type::king;
	if (HasCondition(player, TFCond_PlagueRune)) return powerup_type::plague;
	if (HasCondition(player, TFCond_SupernovaRune)) return powerup_type::supernova;
	return powerup_type::not_powerup;
}

void VectorTransform (const float *in1, const matrix3x4_t& in2, float *out)
{
	out[0] = (in1[0] * in2[0][0] + in1[1] * in2[0][1] + in1[2] * in2[0][2]) + in2[0][3];
	out[1] = (in1[0] * in2[1][0] + in1[1] * in2[1][1] + in1[2] * in2[1][2]) + in2[1][3];
	out[2] = (in1[0] * in2[2][0] + in1[1] * in2[2][1] + in1[2] * in2[2][2]) + in2[2][3];
}

bool GetHitbox(CachedEntity* entity, int hb, Vector& out) {
	if (CE_BAD(entity)) return false;
	CachedHitbox* box = entity->m_pHitboxCache->GetHitbox(hb);
	if (!box) out = entity->m_vecOrigin;
	else out = box->center;
	return true;
}

void VectorAngles(Vector &forward, Vector &angles) {
	float tmp, yaw, pitch;

	if(forward[1] == 0 && forward[0] == 0)
	{
		yaw = 0;
		if(forward[2] > 0)
			pitch = 270;
		else
			pitch = 90;
	}
	else
	{
		yaw = (atan2(forward[1], forward[0]) * 180 / PI);
		if(yaw < 0)
			yaw += 360;

		tmp = sqrt((forward[0] * forward[0] + forward[1] * forward[1]));
		pitch = (atan2(-forward[2], tmp) * 180 / PI);
		if(pitch < 0)
			pitch += 360;
	}

	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}

char GetUpperChar(ButtonCode_t button) {
	switch (button) {
	case KEY_0:
		return ')';
	case KEY_1:
		return '!';
	case KEY_2:
		return '@';
	case KEY_3:
		return '#';
	case KEY_4:
		return '$';
	case KEY_5:
		return '%';
	case KEY_6:
		return '^';
	case KEY_7:
		return '&';
	case KEY_8:
		return '*';
	case KEY_9:
		return '(';
	case KEY_LBRACKET:
		return '{';
	case KEY_RBRACKET:
		return '}';
	case KEY_SEMICOLON:
		return ':';
	case KEY_BACKQUOTE:
		return '~';
	case KEY_APOSTROPHE:
		return '"';
	case KEY_COMMA:
		return '<';
	case KEY_PERIOD:
		return '>';
	case KEY_SLASH:
		return '?';
	case KEY_BACKSLASH:
		return '|';
	case KEY_MINUS:
		return '_';
	case KEY_EQUAL:
		return '+';
	default:
		if (strlen(g_IInputSystem->ButtonCodeToString(button)) != 1) return 0;
		return toupper(*g_IInputSystem->ButtonCodeToString(button));
	}
}

char GetChar(ButtonCode_t button) {
	switch (button) {
	case KEY_PAD_DIVIDE:
		return '/';
	case KEY_PAD_MULTIPLY:
		return '*';
	case KEY_PAD_MINUS:
		return '-';
	case KEY_PAD_PLUS:
		return '+';
	case KEY_SEMICOLON:
		return ';';
	default:
		if (button >= KEY_PAD_0 && button <= KEY_PAD_9) {
			return button - KEY_PAD_0 + '0';
		}
		if (strlen(g_IInputSystem->ButtonCodeToString(button)) != 1) return 0;
		return *g_IInputSystem->ButtonCodeToString(button);
	}
}

void FixMovement(CUserCmd& cmd, Vector& viewangles) {
	Vector movement(cmd.forwardmove, cmd.sidemove, cmd.upmove);
	float speed = sqrt(movement.x * movement.x + movement.y * movement.y);
	Vector ang;
	VectorAngles(movement, ang);
	float yaw = DEG2RAD(ang.y - viewangles.y + cmd.viewangles.y);
	cmd.forwardmove = cos(yaw) * speed;
	cmd.sidemove = sin(yaw) * speed;
	/*Vector vsilent(cmd->forwardmove, cmd->sidemove, cmd->upmove);
	float speed = sqrt(vsilent.x * vsilent.x + vsilent.y * vsilent.y);
	Vector ang;
	VectorAngles(vsilent, ang);
	float yaw = deg2rad(ang.y - viewangles_old.y + cmd->viewangles.y);
	cmd->forwardmove = cos(yaw) * speed;
	cmd->sidemove = sin(yaw) * speed;*/
}

bool IsPlayerInvisible(CachedEntity* player) {
	return (HasCondition(player, TFCond_Cloaked) && !(
		 HasCondition(player, TFCond_OnFire) || HasCondition(player, TFCond_Jarated)
		 || HasCondition(player, TFCond_CloakFlicker) || HasCondition(player, TFCond_Milked)
		 || HasCondition(player, TFCond_Bleeding)));
}

float RandFloatRange(float min, float max)
{
    return (min + 1) + (((float) rand()) / (float) RAND_MAX) * (max - (min + 1));
}

bool IsEntityVisible(CachedEntity* entity, int hb) {
	if (g_Settings.bInvalid) return false;
	if (entity == g_pLocalPlayer->entity) return true;
	Vector hit;
	if (hb == -1) {
		return IsEntityVectorVisible(entity, entity->m_vecOrigin);
	} else {
		return entity->m_pHitboxCache->VisibilityCheck(hb);
	}

}

bool IsEntityVectorVisible(CachedEntity* entity, Vector endpos) {
	if (g_Settings.bInvalid) return false;
	if (entity == g_pLocalPlayer->entity) return true;
	if (CE_BAD(g_pLocalPlayer->entity)) return false;
	if (CE_BAD(entity)) return false;
 	trace_t trace_object;
	Ray_t ray;
	trace::g_pFilterDefault->SetSelf(RAW_ENT(g_pLocalPlayer->entity));
	ray.Init(g_pLocalPlayer->v_Eye, endpos);
	{
		PROF_SECTION(IEVV_TraceRay);
		g_ITrace->TraceRay(ray, MASK_SHOT, trace::g_pFilterDefault, &trace_object);
	}
	return (trace_object.fraction >= 0.99f || (((IClientEntity*)trace_object.m_pEnt)) == RAW_ENT(entity));
}

Vector GetBuildingPosition(CachedEntity* ent) {
	Vector res = ent->m_vecOrigin;
	if (ent->m_iClassID == g_pClassID->CObjectDispenser) res.z += 30;
	if (ent->m_iClassID == g_pClassID->CObjectTeleporter) res.z += 8;
	if (ent->m_iClassID == g_pClassID->CObjectSentrygun) {
		switch (CE_INT(ent, netvar.iUpgradeLevel)) {
		case 1:
			res.z += 30;
			break;
		case 2:
			res.z += 50;
			break;
		case 3:
			res.z += 60;
			break;
		}
	}
	return res;
}

bool IsBuildingVisible(CachedEntity* ent) {
	return IsEntityVectorVisible(ent, GetBuildingPosition(ent));
}

void fVectorAngles(Vector &forward, Vector &angles) {
	float tmp, yaw, pitch;

	if(forward[1] == 0 && forward[0] == 0)
	{
		yaw = 0;
		if(forward[2] > 0)
			pitch = 270;
		else
			pitch = 90;
	}
	else
	{
		yaw = (atan2(forward[1], forward[0]) * 180 / PI);
		if(yaw < 0)
			yaw += 360;

		tmp = sqrt((forward[0] * forward[0] + forward[1] * forward[1]));
		pitch = (atan2(-forward[2], tmp) * 180 / PI);
		if(pitch < 0)
			pitch += 360;
	}

	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}

void fClampAngle(Vector& qaAng) {
	while(qaAng[0] > 89)
		qaAng[0] -= 180;

	while(qaAng[0] < -89)
		qaAng[0] += 180;

	while(qaAng[1] > 180)
		qaAng[1] -= 360;

	while(qaAng[1] < -180)
		qaAng[1] += 360;

	qaAng.z = 0;
}

float DistToSqr(CachedEntity* entity) {
	if (CE_BAD(entity)) return 0.0f;
	return g_pLocalPlayer->v_Origin.DistToSqr(entity->m_vecOrigin);
}

bool IsMeleeWeapon(CachedEntity* ent) {

	if (ent->m_iClassID == g_pClassID->CTFBat ||
		ent->m_iClassID == g_pClassID->CTFBat_Fish ||
		ent->m_iClassID == g_pClassID->CTFBat_Giftwrap ||
		ent->m_iClassID == g_pClassID->CTFBat_Wood ||
		ent->m_iClassID == g_pClassID->CTFShovel ||
		ent->m_iClassID == g_pClassID->CTFKatana ||
		ent->m_iClassID == g_pClassID->CTFFireAxe ||
		ent->m_iClassID == g_pClassID->CTFBottle ||
		ent->m_iClassID == g_pClassID->CTFSword ||
		ent->m_iClassID == g_pClassID->CTFFists ||
		ent->m_iClassID == g_pClassID->CTFWrench ||
		ent->m_iClassID == g_pClassID->CTFRobotArm ||
		ent->m_iClassID == g_pClassID->CTFKnife ||
		ent->m_iClassID == g_pClassID->CTFBonesaw ||
		ent->m_iClassID == g_pClassID->CTFClub) {
		return true;
	}
	return false;
}

void Patch(void* address, void* patch, size_t length) {
	void* page = (void*)((uintptr_t)address &~ 0xFFF);
	mprotect(page, 0xFFF, PROT_WRITE | PROT_EXEC);
	memcpy(address, patch, length);
	mprotect(page, 0xFFF, PROT_EXEC);
}

bool IsProjectileCrit(CachedEntity* ent) {
	if (ent->m_bGrenadeProjectile)
		return CE_BYTE(ent, netvar.Grenade_bCritical);
	return CE_BYTE(ent, netvar.Rocket_bCritical);
}

weaponmode GetWeaponMode(CachedEntity* player) {
	if (CE_BAD(player)) return weapon_invalid;
	int weapon_handle = CE_INT(player, netvar.hActiveWeapon);
	if (IDX_BAD((weapon_handle & 0xFFF))) {
		//logging::Info("IDX_BAD: %i", weapon_handle & 0xFFF);
		return weaponmode::weapon_invalid;
	}
	CachedEntity* weapon = (ENTITY(weapon_handle & 0xFFF));
	if (CE_BAD(weapon)) return weaponmode::weapon_invalid;
	if (IsMeleeWeapon(weapon)) return weaponmode::weapon_melee;
	if (weapon->m_iClassID == g_pClassID->CTFLunchBox ||
		weapon->m_iClassID == g_pClassID->CTFLunchBox_Drink ||
		weapon->m_iClassID == g_pClassID->CTFBuffItem) {
		return weaponmode::weapon_consumable;
	} else if ( weapon->m_iClassID == g_pClassID->CTFRocketLauncher_DirectHit ||
				weapon->m_iClassID == g_pClassID->CTFRocketLauncher ||
				weapon->m_iClassID == g_pClassID->CTFGrenadeLauncher ||
				weapon->m_iClassID == g_pClassID->CTFCompoundBow ||
				weapon->m_iClassID == g_pClassID->CTFBat_Wood ||
				weapon->m_iClassID == g_pClassID->CTFBat_Giftwrap ||
				weapon->m_iClassID == g_pClassID->CTFFlareGun ||
				weapon->m_iClassID == g_pClassID->CTFFlareGun_Revenge ||
				weapon->m_iClassID == g_pClassID->CTFSyringeGun) {
		return weaponmode::weapon_projectile;
	} else if (weapon->m_iClassID == g_pClassID->CTFJar ||
			   weapon->m_iClassID == g_pClassID->CTFJarMilk) {
		return weaponmode::weapon_throwable;
	} else if (weapon->m_iClassID == g_pClassID->CTFWeaponPDA_Engineer_Build ||
			   weapon->m_iClassID == g_pClassID->CTFWeaponPDA_Engineer_Destroy ||
			   weapon->m_iClassID == g_pClassID->CTFWeaponPDA_Spy) {
		return weaponmode::weapon_pda;
	} else if (weapon->m_iClassID == g_pClassID->CWeaponMedigun) {
		return weaponmode::weapon_medigun;
	}
	return weaponmode::weapon_hitscan;
}

bool LineIntersectsBox(Vector& bmin, Vector& bmax, Vector& lmin, Vector& lmax) {
	if (lmax.x < bmin.x && lmin.x < bmin.x) return false;
	if (lmax.y < bmin.y && lmin.y < bmin.y) return false;
	if (lmax.z < bmin.z && lmin.z < bmin.z) return false;
	if (lmax.x > bmax.x && lmin.x > bmax.x) return false;
	if (lmax.y > bmax.y && lmin.y > bmax.y) return false;
	if (lmax.z > bmax.z && lmin.z > bmax.z) return false;
	return true;
}

// TODO FIX this function
bool GetProjectileData(CachedEntity* weapon, float& speed, float& gravity) {
	if (CE_BAD(weapon)) return false;
	float rspeed = 0.0f;
	float rgrav = 0.0f;
	typedef float(GetProjectileData)(IClientEntity*);

	if (weapon->m_iClassID == g_pClassID->CTFRocketLauncher_DirectHit) {
		rspeed = 1980.0f;
	} else if (weapon->m_iClassID == g_pClassID->CTFRocketLauncher) {
		rspeed = 1100.0f;
	} else if (weapon->m_iClassID == g_pClassID->CTFGrenadeLauncher) {
		if (TF2) {
			rspeed = vfunc<GetProjectileData*>(RAW_ENT(weapon), 527)(RAW_ENT(weapon));
			// TODO Wrong grenade launcher gravity
			rgrav = 0.5f;
		} else if (TF2C) {
			rspeed = 1100.0f;
			rgrav = 0.5f;
		}
	} else if (weapon->m_iClassID == g_pClassID->CTFCompoundBow) {
		rspeed = vfunc<GetProjectileData*>(RAW_ENT(weapon), 527)(RAW_ENT(weapon));
		rgrav = vfunc<GetProjectileData*>(RAW_ENT(weapon), 528)(RAW_ENT(weapon));
	} else if (weapon->m_iClassID == g_pClassID->CTFBat_Wood) {
		rspeed = 3000.0f;
		rgrav = 0.5f;
	} else if (weapon->m_iClassID == g_pClassID->CTFFlareGun) {
		rspeed = 2000.0f;
		rgrav = 0.25f;
	} else if (weapon->m_iClassID == g_pClassID->CTFSyringeGun) {
		rgrav = 0.2f;
		rspeed = 990.0f;
	}
	speed = rspeed;
	gravity = rgrav;
	return (rspeed || rgrav);
}

constexpr unsigned developer_list[] = { 347272825, 401679596 };

bool Developer(CachedEntity* ent) {
	if (ent->m_pPlayerInfo) {
		for (int i = 0; i < sizeof(developer_list) / sizeof(unsigned); i++) {
			if (developer_list[i] == ent->m_pPlayerInfo->friendsID) return true;
		}
	}
	return false;
}

/*const char* MakeInfoString(IClientEntity* player) {
	char* buf = new char[256]();
	player_info_t info;
	if (!engineClient->GetPlayerInfo(player->entindex(), &info)) return (const char*)0;
	logging::Info("a");
	int hWeapon = NET_INT(player, netvar.hActiveWeapon);
	if (NET_BYTE(player, netvar.iLifeState)) {
		sprintf(buf, "%s is dead %s", info.name, tfclasses[NET_INT(player, netvar.iClass)]);
		return buf;
	}
	if (hWeapon) {
		IClientEntity* weapon = ENTITY(hWeapon & 0xFFF);
		sprintf(buf, "%s is %s with %i health using %s", info.name, tfclasses[NET_INT(player, netvar.iClass)], NET_INT(player, netvar.iHealth), weapon->GetClientClass()->GetName());
	} else {
		sprintf(buf, "%s is %s with %i health", info.name, tfclasses[NET_INT(player, netvar.iClass)], NET_INT(player, netvar.iHealth));
	}
	logging::Info("Result: %s", buf);
	return buf;
}*/

trace::FilterNoPlayer* vec_filter;
bool IsVectorVisible(Vector origin, Vector target) {
	//logging::Info("ISVV");
	if (!vec_filter) {
		vec_filter = new trace::FilterNoPlayer();
	}
	trace_t trace_visible;
	Ray_t ray;
	vec_filter->SetSelf(RAW_ENT(g_pLocalPlayer->entity));
	ray.Init(origin, target);
	g_ITrace->TraceRay(ray, MASK_SHOT_HULL, vec_filter, &trace_visible);
	float dist2 = origin.DistToSqr(trace_visible.endpos);
	float dist1 = origin.DistToSqr(target);
	//logging::Info("Target: %.1f, %.1f, %.1f; ENDPOS: %.1f, %.1f, %.1f", target.x, target.y, target.z, trace_visible.endpos.x, trace_visible.endpos.y, trace_visible.endpos.z);
	//logging::Info("Dist1: %f, Dist2: %f");
	return (dist1 <= dist2);
}

/*bool PredictProjectileAim(Vector origin, IClientEntity* target, hitbox_t hb, float speed, bool arc, float gravity, Vector& result) {
	if (!target) return false;
	//logging::Info("PRED PROJ AIM");
	//logging::Info("ProjSpeed: %f", speed);
	Vector res1 = result;
	if (GetHitboxPosition(target, hb, result)) {
		res1 = target->GetAbsOrigin();
	}
	int flags = NET_INT(target, eoffsets.iFlags);
	bool ground = (flags & (1 << 0));
	float distance = origin.DistTo(res1);
	float latency = engineClient->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING) +
		engineClient->GetNetChannelInfo()->GetLatency(FLOW_INCOMING);
	if (speed == 0) return false;
	float time = distance / speed + latency;
	if (!ground) {
		res1.z -= (400 * time * time);
	}
	res1 += NET_VECTOR(target, eoffsets.vVelocity) * time;
	if (arc)
		res1.z += (800 * 0.5 * gravity * time * time);
	result = res1;
	//if (!IsVisible();
	return IsVectorVisible(origin, res1);
}*/

relation GetRelation(CachedEntity* ent) {
	if (!ent->m_pPlayerInfo) return relation::NEUTRAL;
	for (int i = 0; i < n_friends; i++) {
		if (friends[i] == ent->m_pPlayerInfo->friendsID) return relation::FRIEND;
	}
	for (int i = 0; i < n_rage; i++) {
		if (rage[i] == ent->m_pPlayerInfo->friendsID) return relation::RAGE;
	}
	if (Developer(ent)) return relation::DEVELOPER;

	if (ipc::peer) {
		for (unsigned i = 1; i < cat_ipc::max_peers; i++) {
			if (!ipc::peer->memory->peer_data[i].free && ipc::peer->memory->peer_user_data[i].friendid == ent->m_pPlayerInfo->friendsID) {
				return relation::BOT;
			}
		}
	}

	return relation::NEUTRAL;
}

void WhatIAmLookingAt(int* result_eindex, Vector* result_pos) {
	Ray_t ray;
	trace::g_pFilterDefault->SetSelf(RAW_ENT(g_pLocalPlayer->entity));
	Vector forward;
	float sp, sy, cp, cy;
	QAngle angle;
	g_IEngine->GetViewAngles(angle);
	sy = sinf(DEG2RAD(angle[1]));
	cy = cosf(DEG2RAD(angle[1]));
	sp = sinf(DEG2RAD(angle[0]));
	cp = cosf(DEG2RAD(angle[0]));
	forward.x = cp * cy;
	forward.y = cp * sy;
	forward.z = -sp;
	forward = forward * 8192.0f + g_pLocalPlayer->v_Eye;
	ray.Init(g_pLocalPlayer->v_Eye, forward);
	trace_t trace;
	g_ITrace->TraceRay(ray, 0x4200400B, trace::g_pFilterDefault, &trace);
	if (result_pos)
		*result_pos = trace.endpos;
	if (result_eindex) {
		*result_eindex = 0;
	}
	if (trace.m_pEnt && result_eindex) {
		*result_eindex = ((IClientEntity*)(trace.m_pEnt))->entindex();
	}
}

bool IsSentryBuster(CachedEntity* entity) {
	return (entity->m_Type == EntityType::ENTITY_PLAYER &&
			CE_INT(entity, netvar.iClass) == tf_class::tf_demoman &&
			g_pPlayerResource->GetMaxHealth(entity) == 2500);
}

bool IsAmbassador(CachedEntity* entity) {
	if (!TF2) return false;
	if (entity->m_iClassID != g_pClassID->CTFRevolver) return false;
	int defidx = CE_INT(entity, netvar.iItemDefinitionIndex);
	return (defidx == 61 || defidx == 1006);
}

// F1 c&p
Vector CalcAngle(Vector src, Vector dst) {
	Vector AimAngles;
	Vector delta = src - dst;
	float hyp = sqrtf((delta.x * delta.x) + (delta.y * delta.y)); //SUPER SECRET IMPROVEMENT CODE NAME DONUT STEEL
	AimAngles.x = atanf(delta.z / hyp) * RADPI;
	AimAngles.y = atanf(delta.y / delta.x) * RADPI;
	AimAngles.z = 0.0f;
	if(delta.x >= 0.0)
		AimAngles.y += 180.0f;
	return AimAngles;
}

void MakeVector(Vector angle, Vector& vector)
{
	float pitch = float(angle[0] * PI / 180);
	float yaw = float(angle[1] * PI / 180);
	float tmp = float(cos(pitch));
	vector[0] = float(-tmp * -cos(yaw));
	vector[1] = float(sin(yaw)*tmp);
	vector[2] = float(-sin(pitch));
}

float GetFov(Vector angle, Vector src, Vector dst)
{
	Vector ang, aim;
	ang = CalcAngle(src, dst);

	MakeVector(angle, aim);
	MakeVector(ang, ang);

	float mag = sqrtf(pow(aim.x, 2) + pow(aim.y, 2) + pow(aim.z, 2));
	float u_dot_v = aim.Dot(ang);

	return RAD2DEG(acos(u_dot_v / (pow(mag, 2))));
}

bool CanHeadshot() {
	return (g_pLocalPlayer->flZoomBegin > 0.0f && (g_GlobalVars->curtime - g_pLocalPlayer->flZoomBegin > 0.2f));
}

bool CanShoot() {
	float tickbase = (float)(CE_INT(g_pLocalPlayer->entity, netvar.nTickBase)) * g_GlobalVars->interval_per_tick;
	float nextattack = CE_FLOAT(g_pLocalPlayer->weapon(), netvar.flNextPrimaryAttack);
	return nextattack <= tickbase;
}

QAngle VectorToQAngle(Vector in) {
	return *(QAngle*)&in;
}

Vector QAngleToVector(QAngle in) {
	return *(Vector*)&in;
}

void AimAt(Vector origin, Vector target, CUserCmd* cmd) {
	Vector angles;
	Vector tr = (target - origin);
	fVectorAngles(tr, angles);
	fClampAngle(angles);
	cmd->viewangles = angles;
}

void AimAtHitbox(CachedEntity* ent, int hitbox, CUserCmd* cmd) {
	Vector r = ent->m_vecOrigin;
	GetHitbox(ent, hitbox, r);
	AimAt(g_pLocalPlayer->v_Eye, r, cmd);
}

bool IsEntityVisiblePenetration(CachedEntity* entity, int hb) {
	if (!trace::g_pFilterPenetration) {
		trace::g_pFilterPenetration = new trace::FilterPenetration();
	}
	trace_t trace_visible;
	Ray_t ray;
	trace::g_pFilterPenetration->SetSelf(RAW_ENT(g_pLocalPlayer->entity));
	trace::g_pFilterPenetration->Reset();
	Vector hit;
	int ret = GetHitbox(entity, hb, hit);
	if (ret) {
		return false;
	}
	ray.Init(g_pLocalPlayer->v_Origin + g_pLocalPlayer->v_ViewOffset, hit);
	g_ITrace->TraceRay(ray, MASK_SHOT_HULL, trace::g_pFilterPenetration, &trace_visible);
	bool s = false;
	if (trace_visible.m_pEnt) {
		s = ((IClientEntity*)trace_visible.m_pEnt) == RAW_ENT(entity);
	}
	if (!s) return false;
	g_ITrace->TraceRay(ray, 0x4200400B, trace::g_pFilterDefault, &trace_visible);
	if (trace_visible.m_pEnt) {
		IClientEntity* ent = (IClientEntity*)trace_visible.m_pEnt;
		if (ent) {
			if (ent->GetClientClass()->m_ClassID == g_pClassID->C_Player) {
				if (ent == RAW_ENT(entity)) return false;
				if (trace_visible.hitbox >= 0) {
					return true;
				}
			}
		}
	}
	return false;
}


class CMoveData;

/*void RunEnginePrediction(IClientEntity* ent, CUserCmd *ucmd) {
	// we are going to require some helper functions for this to work
	// notably SetupMove, FinishMove and ProcessMovement


	// setup the types of the functions
	typedef void(*SetupMoveFn)(IClientEntity *, CUserCmd *, class IMoveHelper *, CMoveData *);
	typedef void(*FinishMoveFn)(IClientEntity *, CUserCmd*, CMoveData*);
	typedef void(*ProcessMovementFn)(IClientEntity *, CMoveData *);
	typedef void(*StartTrackPredictionErrorsFn)(IClientEntity *);
	typedef void(*FinishTrackPredictionErrorsFn)(IClientEntity *);

	// get the vtable
	void **predictionVtable = *(void ***)prediction;
	logging::Info("predictionVtable 0x%08x", predictionVtable);
	// get the functions
	SetupMoveFn oSetupMove = (SetupMoveFn) predictionVtable[19];
	FinishMoveFn oFinishMove = (FinishMoveFn) predictionVtable[20];

	// get the vtable
	void **gameMovementVtable = *(void ***)gamemovement;
	logging::Info("gameMovementVtable 0x%08x", gameMovementVtable);
	// get the functions
	ProcessMovementFn oProcessMovement = (ProcessMovementFn) gameMovementVtable[2];
	StartTrackPredictionErrorsFn oStartTrackPredictionErrors = (StartTrackPredictionErrorsFn) gameMovementVtable[3];
	FinishTrackPredictionErrorsFn oFinishTrackPredictionErrors = (FinishTrackPredictionErrorsFn) gameMovementVtable[4];

	// use this as movedata (should be big enough - otherwise the stack will die!)
	unsigned char moveData[2048];
	CMoveData *pMoveData = (CMoveData *)&(moveData[0]);
	logging::Info("pMoveData 0x%08x", pMoveData);

	// back up globals
	float frameTime = gvars->frametime;
	float curTime = gvars->curtime;

	CUserCmd defaultCmd;
	if(ucmd == NULL)
	{
		ucmd = &defaultCmd;
	}

	// set the current command
	NET_VAR(ent, 0x105C, void*) = ucmd;

	// set up the globals
	gvars->curtime =  gvars->interval_per_tick * NET_INT(ent, netvar.nTickBase);
	gvars->frametime = gvars->interval_per_tick;

	oStartTrackPredictionErrors(ent);

	logging::Info("StartTrackPredictionErrors(ent)");
	oSetupMove(ent, ucmd, NULL, pMoveData);
	logging::Info("oSetupMove");
	oProcessMovement(ent, pMoveData);
	logging::Info("oProcessMovement");
	oFinishMove(ent, ucmd, pMoveData);
	logging::Info("oFinishMove");

	oFinishTrackPredictionErrors(ent);
	logging::Info("oFinishTrackPredictionErrors");
	// reset the current command
	NET_VAR(ent, 0x105C, void *) = 0;

	// restore globals
	gvars->frametime = frameTime;
	gvars->curtime = curTime;

	return;
}

float oldCurtime;
float oldFrametime;

void StartPrediction(CUserCmd* cmd) {
	oldCurtime = gvars->curtime;
	oldFrametime = gvars->frametime;
	gvars->curtime = NET_INT(g_pLocalPlayer->entity, netvar.nTickBase) * gvars->interval_per_tick;
	gvars->frametime = gvars->interval_per_tick;
	//gamemovement->
}

void EndPrediction() {
	gvars->curtime = oldCurtime;
	gvars->frametime = oldFrametime;
}*/

char* strfmt(const char* fmt, ...) {
	char* buf = new char[1024];
	va_list list;
	va_start(list, fmt);
	vsprintf(buf, fmt, list);
	va_end(list);
	return buf;
}

const char* powerups[] = {
	"STRENGTH",
	"RESISTANCE",
	"VAMPIRE",
	"REFLECT",
	"HASTE",
	"REGENERATION",
	"PRECISION",
	"AGILITY",
	"KNOCKOUT",
	"KING",
	"PLAGUE",
	"SUPERNOVA",
	"CRITS"
};

uint32 friends[256];
uint32 rage[256];

int n_friends = 0;
int n_rage = 0;

const char* tfclasses[] = {
	"[NULL]",
	"Scout",
	"Sniper",
	"Soldier",
	"Demoman",
	"Medic",
	"Heavy",
	"Pyro",
	"Spy",
	"Engineer"
};
