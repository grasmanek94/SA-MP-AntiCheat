/*	Changelog
	rc 1:
		*Complete rewrite of the AntiCheat plugin.
	rc 2:
		*First public release after testing
	rc 3:
		*minor bugfixes
	rc 4: 
		*minor bugfixes
		*improved weapon detection systems
		*added hook (SetPlayerSpecialAction) to avoid innocent jetpack cheat detections
		*fixed missing ';' in the AntiCheat include (silly me, just being human..)
	rc 5:
		*fixed fake kill (spoof kill) detection
		*small bugfix in OnPlayerStateChange
*/

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <list>
#include <cstring>
#include <limits>
#include <sstream>
#include <ctime>
#include <unordered_set>

#include <sampgdk/core.h>
#include <sampgdk/a_players.h>
#include <sampgdk/plugin.h>
#include <sampgdk/a_vehicles.h>
#include <sampgdk/a_samp.h>

#include "vehiclespeedlist.var"
/////////////////////////////////////////////////////////////////////////////////////

#pragma warning (disable: 4996)

#undef MAX_PLAYERS
#define MAX_PLAYERS (1000)
#define PLUGIN_MAIN_VERSION (25)

#define PI (3.1415926535897932384626433832795f)

using namespace std;  

list<AMX *> amx_list;

int PlayerColors[100] =
{
	0x1874CDFF,0xFFB90FFF,0xDC143CFF,0x98F5FFFF,0x7FFFD4FF,0x912CEEFF,0xC00040FF,0x66cc99FF,0x00ccccFF,0x33ccffFF,
	0x33FFFFFF,0x009933FF,0xFF3333FF,0x3333CCFF,0x003366FF,0x008080FF,0xB22222FF,0xFF1493FF,0x8A2BE2FF,0x32CD32FF,
	0xE0FFFFFF,0x00BFFFFF,0xFF6030FF,0xFFB100FF,0x406090FF,0x0000FFFF,0x8000FFFF,0x40E010FF,0xFF8040FF,0xFF00F0FF,
	0xC0FF50FF,0x80FFC0FF,0x8020FFFF,0x40A0E0FF,0xFFFF40FF,0x008040FF,0xC02050FF,0xFFA070FF,0xFF8010FF,0xFF2030FF,
	0x80A0C0FF,0x80C040FF,0x24C005FF,0x00FFFFFF,0x00A0D0FF,0x004070FF,0x80E0FFFF,0x8020B0FF,0xA79C20FF,0x0080B0FF,
	0x80C0FFFF,0x004080FF,0xE0D560FF,0xFF00D5FF,0xF4A460FF,0x4169E1FF,0x006400FF,0x00FFFFFF,0xDAA520FF,0x9370DBFF,
	0x000080FF,0xDC143CFF,0xFF4500FF,0x708090FF,0xFF6347FF,0x800080FF,0x00FF7FFF,0x20B2AAFF,0xFF1493FF,0x6633ffFF,
	0x00ff99FF,0xcccc00FF,0xffcc00FF,0xff9900FF,0xcc66ffFF,0x5F9F9FFF,0x7093DBFF,0x238E23FF,0x4D4DFFFF,0x3333ffFF,
	0x9F945CFF,0xDCDE3DFF,0x10C9C5FF,0x70524DFF,0x0BE472FF,0x8A2CD7FF,0x6152C2FF,0xCF72A9FF,0xE59338FF,0xEEDC2DFF,
	0x3FE65CFF,0xFFD720FF,0xBD34DAFF,0x54137DFF,0xAF2FF3FF,0x4B8987FF,0xE3AC12FF,0xC1F7ECFF,0xA04E0AFF,0xD8C762FF
};

int g_Ticked = 0;
int g_TickMax = 200;
signed long g_PlayerMoney[MAX_PLAYERS];
int g_max_ip = 257;//257 = on ip can connect as many times as it wants to, 0 means NOBODY can connect, 1 means each player needs an unique IP address
//
/* Helper macros */ 
#define HEX__(n) 0x##n##LU 
#define B8__(x) (((x&0x0000000FLU)?1:0)+((x&0x000000F0LU)?2:0)+((x&0x00000F00LU)?4:0)+((x&0x0000F000LU)?8:0)+((x&0x000F0000LU)?16:0)+((x&0x00F00000LU)?32:0)+((x&0x0F000000LU)?64:0)+((x&0xF0000000LU)?128:0))
 
/* User macros */ 
#define B8(d) ((unsigned char)B8__(HEX__(d))) 
#define B16(dmsb,dlsb) (((unsigned short)B8(dmsb)<<8)+B8(dlsb)) 
#define B32(dmsb,db2,db3,dlsb) (((unsigned long)B8(dmsb)<<24)+ ((unsigned long)B8(db2)<<16)+ ((unsigned long)B8(db3)<<8)+ B8(dlsb)) 

#define g_RESET_BITS()     (g_CheckForStuff = 0)
#define g_SET_TRUE(check)		(g_CheckForStuff |= check)
#define g_SET_FALSE(check)		(g_CheckForStuff &= (~check))
#define g_TOGGLE(check)			(g_CheckForStuff ^= check)
#define g_IS_TRUE(check)      (g_CheckForStuff & check)

#define FIX_RESET_BITS()     (g_Fixes = 0)
#define FIX_SET_TRUE(check)		(g_Fixes |= check)
#define FIX_SET_FALSE(check)		(g_Fixes &= (~check))
#define FIX_TOGGLE(check)			(g_Fixes ^= check)
#define FIX_IS_TRUE(check)      (g_Fixes & check)

#define p_RESET_BITS(playerid)     (p_CheckForStuff[playerid] = 0)
#define p_SET_TRUE(playerid,check)		(p_CheckForStuff[playerid] |= check)
#define p_SET_FALSE(playerid,check)		(p_CheckForStuff[playerid] &= (~check))
#define p_TOGGLE(playerid,check)			(p_CheckForStuff[playerid] ^= check)
#define p_IS_TRUE(playerid,check)      (p_CheckForStuff[playerid] & check)


// From "amx.c", part of the PAWN language runtime:
// http://code.google.com/p/pawnscript/source/browse/trunk/amx/amx.c

#define USENAMETABLE(hdr) \
	((hdr)->defsize==sizeof(AMX_FUNCSTUBNT))

#define NUMENTRIES(hdr,field,nextfield) \
	(unsigned)(((hdr)->nextfield - (hdr)->field) / (hdr)->defsize)

#define GETENTRY(hdr,table,index) \
	(AMX_FUNCSTUB *)((unsigned char*)(hdr) + (unsigned)(hdr)->table + (unsigned)index*(hdr)->defsize)

#define GETENTRYNAME(hdr,entry) \
	(USENAMETABLE(hdr) ? \
		(char *)((unsigned char*)(hdr) + (unsigned)((AMX_FUNCSTUBNT*)(entry))->nameofs) : \
		((AMX_FUNCSTUB*)(entry))->name)


void Report(int playerid,int type, int extraint,float extrafloat,int extraint2)
{
	int amx_idx = 0;
	int amxerr = 0;
	for(list <AMX *>::iterator i = amx_list.begin(); i != amx_list.end(); ++i)
	{
		amxerr = amx_FindPublic(*i, "AC_OnCheatDetected", &amx_idx);
		if (amxerr == AMX_ERR_NONE)
		{
			amx_Push(* i, extraint2);
			amx_Push(* i, amx_ftoc(extrafloat));
			amx_Push(* i, extraint);
			amx_Push(* i, type);
			amx_Push(* i, playerid);
			amx_Exec(* i, NULL, amx_idx);
		}
	}
}

void Report(int playerid,int type)
{
	Report(playerid,type,0,0.0f,0);
}

void Report(int playerid,int type, int extraint)
{
	Report(playerid,type,extraint,0.0f,0);
}

void Report(int playerid,int type, int extraint,int extraint2)
{
	Report(playerid,type,extraint,0.0f,extraint2);
}

void Report(int playerid,int type, int extraint,float extraint2)
{
	Report(playerid,type,extraint,extraint2,0);
}

void Report(int playerid,int type, float extrafloat)
{
	Report(playerid,type,0,extrafloat,0);
}

//
unsigned long g_CheckForStuff = -1;
unsigned long g_Fixes = -1;
unsigned long p_CheckForStuff[MAX_PLAYERS];

unordered_map<int, unsigned short> ConnectedIPs;
unordered_map<int, clock_t> IPLastConnectTime;
clock_t g_IPConnectDelay = 100;

char OnPlayerConnectszIP[17];
unsigned short OnPlayerConnectexplodeIP[4];
int PlayerIPIndex[MAX_PLAYERS];

int g_max_ping = 65537;//65536+1;
unordered_set<int> PlayerLoopList;
unordered_set<int> ToKick;
unordered_set<int> ToBan;
bool GivesParachute[612];
//
bool p_ABneecnextdetect[MAX_PLAYERS];
bool p_IsSpectateAllowed[MAX_PLAYERS];
bool p_SkipNextAB[MAX_PLAYERS];
bool g_WeaponEnabled[48];
bool p_WeaponEnabled[MAX_PLAYERS][48];
bool p_HasWeapon[MAX_PLAYERS][48];
bool p_Frozen[MAX_PLAYERS];
bool p_HasJetPack[MAX_PLAYERS];
int p_IsInVehicle[MAX_PLAYERS];
bool UseChatForInactivityMeasurement = true;
int v_PlayerInVehicle[MAX_VEHICLES];
int g_SkinHasWeapon[300][3];
int p_SpawnInfoWeapons[MAX_PLAYERS][3];
int p_LastAnimation[MAX_PLAYERS];
bool InLoop = false;

struct st_INACTIVITY
{
	clock_t LastActive;
	clock_t LastTick;
	bool Reported;
};

st_INACTIVITY p_AcivityInfo[MAX_PLAYERS];
clock_t AllowedInactivityTime = 180000;
clock_t p_SpawnTime[MAX_PLAYERS];
clock_t TimeDiffForSpawnKill = 2000;
clock_t p_VehicleEnterTime[MAX_PLAYERS];
clock_t g_MassTpDelay;

struct st_AIRBREAKMETER
{
	clock_t LastMeasured;
	float lastX;
	float lastY;
	float lastZ;
	float newX;
	float newY;
	float newZ;
};

st_AIRBREAKMETER p_AirBrkCheck[MAX_PLAYERS];

#define CHECK_JETPACK			(1)		// 0000 0000 0000 0000 0000 0000 0000 0001
#define CHECK_WEAPON			(2)		// 0000 0000 0000 0000 0000 0000 0000 0010
#define CHECK_SPEED				(4)		// 0000 0000 0000 0000 0000 0000 0000 0100
#define CHECK_HEALTHARMOUR      (8)		// 0000 0000 0000 0000 0000 0000 0000 1000
#define CHECK_IPFLOOD			(16)	// 0000 0000 0000 0000 0000 0000 0001 0000
#define CHECK_PING				(32)	// 0000 0000 0000 0000 0000 0000 0010 0000
#define CHECK_SPOOFKILL			(64)	// 0000 0000 0000 0000 0000 0000 0100 0000
#define CHECK_SPAWNKILL			(128)	// 0000 0000 0000 0000 0000 0000 1000 0000
#define CHECK_INACTIVITY		(256)	// 0000 0000 0000 0000 0000 0001 0000 0000
#define CHECK_TELEPORT			(512)	// 0000 0000 0000 0000 0000 0010 0000 0000
#define CHECK_AIRBREAK			(1024)	// 0000 0000 0000 0000 0000 0100 0000 0000
#define CHECK_BACK_FROM_INACTIVITY (2048)//0000 0000 0000 0000 0000 1000 0000 0000
#define CHECK_SPECTATE			(4096)	// 0000 0000 0000 0000 0001 0000 0000 0000
#define CHECK_FASTCONNECT		(8192)	// 0000 0000 0000 0000 0010 0000 0000 0000
#define CHECK_REMOTECONTROL		(16384)	// 0000 0000 0000 0000 0100 0000 0000 0000
#define CHECK_MASSCARTELEPORT	(32768) // 0000 0000 0000 0000 1000 0000 0000 0000
#define CHECK_CARJACKHACK		(65536)	// 0000 0000 0000 0001 0000 0000 0000 0000

/////////////////////////////////////////////////////////////////////////////////////

static cell AMX_NATIVE_CALL n_safeGetPlayerMoney( AMX* amx, cell* params )
{
	if(params[1] < 0 || params[1] >= MAX_PLAYERS)return 0;
	return g_PlayerMoney[params[1]];
}

static cell AMX_NATIVE_CALL n_safeGivePlayerMoney( AMX* amx, cell* params )
{
	if(params[1] < 0 || params[1] >= MAX_PLAYERS)return 0;
	g_PlayerMoney[params[1]] += params[2];
	return GivePlayerMoney(params[1],g_PlayerMoney[params[1]]-GetPlayerMoney(params[1]));
}

static cell AMX_NATIVE_CALL n_safeResetPlayerMoney( AMX* amx, cell* params )
{
	if(params[1] < 0 || params[1] >= MAX_PLAYERS)return 0;
	g_PlayerMoney[params[1]] = 0;
	return ResetPlayerMoney(params[1]);
}

static cell AMX_NATIVE_CALL n_safeSetSpawnInfo( AMX* amx, cell* params )
{
	if(params[1] < 0 || params[1] >= MAX_PLAYERS)return 0;
	for(int i = 1; i < 48; ++i)p_SpawnInfoWeapons[params[1]][i] = false;
	p_SpawnInfoWeapons[params[1]][0] = params[12];
	p_SpawnInfoWeapons[params[1]][1] = params[10];
	p_SpawnInfoWeapons[params[1]][2] = params[8];
	return SetSpawnInfo(params[1],params[2],params[3],amx_ctof(params[4]),amx_ctof(params[5]),amx_ctof(params[6]),amx_ctof(params[7]),params[8],params[9],params[10],params[11],params[12],params[13]);
}

static cell AMX_NATIVE_CALL n_safeAddPlayerClass( AMX* amx, cell* params )
{
	if(params[1] < 0 || params[1] >= 300)return 0;
	for(int x = 1; x < 48; ++x)
	{
		g_SkinHasWeapon[params[1]][x] = false;
	}
	g_SkinHasWeapon[params[1]][0] = params[10];
	g_SkinHasWeapon[params[1]][1] = params[8];
	g_SkinHasWeapon[params[1]][2] = params[6];
	return AddPlayerClass(params[1],amx_ctof(params[2]),amx_ctof(params[3]),amx_ctof(params[4]),amx_ctof(params[5]),params[6],params[7],params[8],params[9],params[10],params[11]);
}

static cell AMX_NATIVE_CALL n_safeAddPlayerClassEx( AMX* amx, cell* params )
{
	if(params[2] < 0 || params[2] >= 300)return 0;
	for(int x = 1; x < 48; ++x)
	{
		g_SkinHasWeapon[params[1]][x] = false;
	}
	g_SkinHasWeapon[params[2]][0] = params[11];
	g_SkinHasWeapon[params[2]][1] = params[9];
	g_SkinHasWeapon[params[2]][2] = params[7];
	return AddPlayerClassEx(params[1],params[2],amx_ctof(params[3]),amx_ctof(params[4]),amx_ctof(params[5]),amx_ctof(params[6]),params[7],params[8],params[9],params[10],params[11],params[12]);
}

static cell AMX_NATIVE_CALL n_safeGivePlayerWeapon( AMX* amx, cell* params )
{
	if(params[1] < 0 || params[1] >= MAX_PLAYERS)return 0;
	p_HasWeapon[params[1]][params[2]] = true;
	return GivePlayerWeapon(params[1],params[2],params[3]);
}

static cell AMX_NATIVE_CALL n_safeResetPlayerWeapons( AMX* amx, cell* params )
{
	if(params[1] < 0 || params[1] >= MAX_PLAYERS)return 0;
	for(int i = 1; i < 48; ++i)
	{
		p_HasWeapon[params[1]][i] = false;
	}
	return ResetPlayerWeapons(params[1]);
}

static cell AMX_NATIVE_CALL n_fixSpawnPlayer( AMX* amx, cell* params )
{
	if(params[1] < 0 || params[1] >= MAX_PLAYERS)return 0;
	if(IsPlayerInAnyVehicle(params[1]) == true)RemovePlayerFromVehicle(params[1]);
	return SpawnPlayer(params[1]);
}

static cell AMX_NATIVE_CALL n_fixSetPlayerRaceCheckpoint( AMX* amx, cell* params )
{
	if(params[1] < 0 || params[1] >= MAX_PLAYERS)return 0;
	DisablePlayerRaceCheckpoint(params[1]);
	return SetPlayerRaceCheckpoint(params[1],params[2],amx_ctof(params[3]),amx_ctof(params[4]),amx_ctof(params[5]),amx_ctof(params[6]),amx_ctof(params[7]),amx_ctof(params[8]),amx_ctof(params[9]));
}

static cell AMX_NATIVE_CALL n_fixSetPlayerCheckpoint( AMX* amx, cell* params )
{
	if(params[1] < 0 || params[1] >= MAX_PLAYERS)return 0;
	DisablePlayerCheckpoint(params[1]);
	return SetPlayerCheckpoint(params[1],amx_ctof(params[2]),amx_ctof(params[3]),amx_ctof(params[4]),amx_ctof(params[5]));
}

#define amx_ctob(c)   ( * ((bool*)&c) )  /* cell to bool */
static cell AMX_NATIVE_CALL n_fixTogglePlayerControllable( AMX* amx, cell* params )
{
	if(params[1] < 0 || params[1] >= MAX_PLAYERS)return 0;
	p_Frozen[params[1]] = amx_ctob(params[2]);
	return TogglePlayerControllable(params[1],p_Frozen[params[1]]);
}

static cell AMX_NATIVE_CALL n_fixPutPlayerInVehicle( AMX* amx, cell* params )
{
	if(params[1] < 0 || params[1] >= MAX_PLAYERS)return 0;
	if(params[2] < 0 || params[2] >= MAX_VEHICLES)return 0;
	float POS[3];
	GetVehiclePos(params[2], &POS[0], &POS[1], &POS[2]);
	SetPlayerPos(params[1], POS[0], POS[1], POS[2]);
	p_VehicleEnterTime[params[1]]-=(g_MassTpDelay+1);
	p_AirBrkCheck[params[1]].newX = POS[0];
	p_AirBrkCheck[params[1]].newY = POS[1];
	p_AirBrkCheck[params[1]].newZ = POS[2];
	p_AirBrkCheck[params[1]].lastX = p_AirBrkCheck[params[1]].newX+0.2f;
	p_AirBrkCheck[params[1]].lastY = p_AirBrkCheck[params[1]].newY+0.2f;
	p_AirBrkCheck[params[1]].lastZ = p_AirBrkCheck[params[1]].newZ+0.2f;	
	p_IsInVehicle[params[1]] = params[2];
	v_PlayerInVehicle[p_IsInVehicle[params[1]]] = params[1];
	return PutPlayerInVehicle(params[1],params[2],params[3]);
}

static cell AMX_NATIVE_CALL n_fixSetPlayerPos( AMX* amx, cell* params )
{
	p_AirBrkCheck[params[1]].newX = amx_ctof(params[2]);
	p_AirBrkCheck[params[1]].newY = amx_ctof(params[3]);
	p_AirBrkCheck[params[1]].newZ = amx_ctof(params[4]);
	p_AirBrkCheck[params[1]].lastX = p_AirBrkCheck[params[1]].newX+0.2f;
	p_AirBrkCheck[params[1]].lastY = p_AirBrkCheck[params[1]].newY+0.2f;
	p_AirBrkCheck[params[1]].lastZ = p_AirBrkCheck[params[1]].newZ+0.2f;	

	return SetPlayerPos(params[1],amx_ctof(params[2]),amx_ctof(params[3]),amx_ctof(params[4]));
}

static cell AMX_NATIVE_CALL n_fixSetPlayerPosFindZ( AMX* amx, cell* params )
{
	p_AirBrkCheck[params[1]].newX = amx_ctof(params[2]);
	p_AirBrkCheck[params[1]].newY = amx_ctof(params[3]);
	p_AirBrkCheck[params[1]].newZ = amx_ctof(params[4]);
	p_AirBrkCheck[params[1]].lastX = p_AirBrkCheck[params[1]].newX+0.2f;
	p_AirBrkCheck[params[1]].lastY = p_AirBrkCheck[params[1]].newY+0.2f;
	p_AirBrkCheck[params[1]].lastZ = p_AirBrkCheck[params[1]].newZ+0.2f;	

	return SetPlayerPosFindZ(params[1],amx_ctof(params[2]),amx_ctof(params[3]),amx_ctof(params[4]));
}

static cell AMX_NATIVE_CALL n_fixSetVehiclePos( AMX* amx, cell* params )
{
	if(v_PlayerInVehicle[params[1]] != (-1))
	{
		p_AirBrkCheck[v_PlayerInVehicle[params[1]]].newX = amx_ctof(params[2]);
		p_AirBrkCheck[v_PlayerInVehicle[params[1]]].newY = amx_ctof(params[3]);
		p_AirBrkCheck[v_PlayerInVehicle[params[1]]].newZ = amx_ctof(params[4]);
		p_AirBrkCheck[v_PlayerInVehicle[params[1]]].lastX = p_AirBrkCheck[v_PlayerInVehicle[params[1]]].newX+0.2f;
		p_AirBrkCheck[v_PlayerInVehicle[params[1]]].lastY = p_AirBrkCheck[v_PlayerInVehicle[params[1]]].newY+0.2f;
		p_AirBrkCheck[v_PlayerInVehicle[params[1]]].lastZ = p_AirBrkCheck[v_PlayerInVehicle[params[1]]].newZ+0.2f;
	}
	return SetVehiclePos(params[1],amx_ctof(params[2]),amx_ctof(params[3]),amx_ctof(params[4]));
}

static cell AMX_NATIVE_CALL n_fixTogglePlayerSpectating( AMX* amx, cell* params )
{
	p_IsSpectateAllowed[params[1]] = amx_ctob(params[2]);
	return TogglePlayerSpectating(params[1],amx_ctob(params[2]));
}

static cell AMX_NATIVE_CALL n_SetIPConnectDelay( AMX* amx, cell* params )
{
	g_IPConnectDelay = params[1];
	return 1;
}

//native CheckSet(ToCheck,playerid = (-1), bool:check=true);
static cell AMX_NATIVE_CALL n_CheckSet( AMX* amx, cell* params )
{
	if(params[2] == (-1))
	{
		if(params[3] == 0)
		{
			g_SET_FALSE(params[1]);
		}
		else
		{
			g_SET_TRUE(params[1]);
		}
		return 1;
	}
	if(params[2] < 0 || params[2] >= MAX_PLAYERS)return 0;
	if(params[3] == 0)
	{
		p_SET_FALSE(params[2],params[1]);
	}
	else
	{
		p_SET_TRUE(params[2],params[1]);
	}
	return 1;
}

//native FixSet(Fix,bool:enabled=true);
static cell AMX_NATIVE_CALL n_FixSet( AMX* amx, cell* params )
{
	if(params[2] == 0)
	{
		FIX_SET_FALSE(params[1]);
	}
	else
	{
		FIX_SET_TRUE(params[1]);
	}
	return 1;
}

//native SetSpawnKillDelay(delay=2000);
static cell AMX_NATIVE_CALL n_SetSpawnKillTime( AMX* amx, cell* params )
{
	TimeDiffForSpawnKill = params[1];
	return 1;
}

//native SetMaxMassTPDelay(delay=150);
static cell AMX_NATIVE_CALL n_SetMaxMassTPDelay( AMX* amx, cell* params )
{
	g_MassTpDelay = params[1];
	return 1;
}

//native SetInactivityDelay(delay=180000);
static cell AMX_NATIVE_CALL n_SetInactivityDelay( AMX* amx, cell* params )
{
	AllowedInactivityTime = params[1];
	return 1;
}

//native AntiCheatSetUpdateDelay(ticks=100);
static cell AMX_NATIVE_CALL n_AntiCheatSetUpdateDelay( AMX* amx, cell* params )
{
	g_TickMax = params[1];
	return 1;
}

//native SetWeaponAllowed(playerid=(-1),weaponid=0,bool:allowed=true);
static cell AMX_NATIVE_CALL n_SetWeaponAllowed( AMX* amx, cell* params )
{
	if(params[1] == (-1))
	{
		if(params[2] < 1 || params[2] >= 48)
		{
			return 0;
		}
		if(params[3] == 1)
		{
			g_WeaponEnabled[params[2]] = true;
		}
		else
		{
			g_WeaponEnabled[params[2]] = false;
		}
		return 1;
	}
	if(params[1] < 0 || params[1] >= MAX_PLAYERS)return 0;
	if(params[2] < 1 || params[2] >= 48)
	{
		return 0;
	}
	if(params[3] == 1)
	{
		p_WeaponEnabled[params[1]][params[2]] = true;
	}
	else
	{
		p_WeaponEnabled[params[1]][params[2]] = false;
	}	
	return 1;
}

//native SetMaxPing(ping=65537);
static cell AMX_NATIVE_CALL n_SetMaxPing( AMX* amx, cell* params )
{
	if(params[1] < 0 || params[1] >= 65567)
	{
		g_max_ping = 65567;
	}
	else
	{
		g_max_ping = params[1];
	}
	return 1;
}

//native UseChatForInactivityMeasurement(bool:use=true);
static cell AMX_NATIVE_CALL n_UseChatForInactivityMeasurement( AMX* amx, cell* params )
{
	UseChatForInactivityMeasurement = amx_ctob(params[1]);
	return 1;
}

static cell AMX_NATIVE_CALL n_safeSetPlayerSpecialAction( AMX* amx, cell* params )
{
	if(params[2] == SPECIAL_ACTION_USEJETPACK)
	{
		p_HasJetPack[params[1]] = true;
	}
	return SetPlayerSpecialAction(params[1],params[2]);
}

static cell AMX_NATIVE_CALL n_safeKick( AMX* amx, cell* params )
{
	if(InLoop)
	{
		ToKick.insert(params[1]);
		return 1;
	}
	return Kick(params[1]);
}

static cell AMX_NATIVE_CALL n_safeBan( AMX* amx, cell* params )
{
	if(InLoop)
	{
		ToBan.insert(params[1]);
		return 1;
	}
	return Ban(params[1]);
}

PLUGIN_EXPORT bool PLUGIN_CALL Load( void **ppData ) 
{	
	for(int i = 0; i < 48; ++i)
	{
		g_WeaponEnabled[i] = true;
	}
	for(int i = 0; i < MAX_PLAYERS; ++i)
	{
		p_WeaponEnabled[i][0] = true;
		p_HasWeapon[i][0] = true;
		for(int x = 1; x < 48; ++x)
		{
			p_WeaponEnabled[i][x] = true;
			p_HasWeapon[i][x] = false;
		}
		for(int x = 0; x < 3; ++x)
		{
			p_SpawnInfoWeapons[i][x] = 0;
		}
	}
	for(int i = 0; i < 300; ++i)
	{
		for(int x = 0; x < 3; ++x)
		{
			g_SkinHasWeapon[i][x] = 0;
		}
	}
	for(int i = 0; i < 612; ++i)
	{
		GivesParachute[i] = false;
	}
	GivesParachute[417] = true;
	GivesParachute[425] = true;
	GivesParachute[447] = true; 
	GivesParachute[460] = true; 
	GivesParachute[469] = true; 
	GivesParachute[476] = true; 
	GivesParachute[487] = true; 
	GivesParachute[488] = true; 
	GivesParachute[497] = true; 
	GivesParachute[511] = true; 
	GivesParachute[512] = true; 
	GivesParachute[513] = true; 
	GivesParachute[519] = true; 
	GivesParachute[520] = true; 
	GivesParachute[553] = true; 
	GivesParachute[548] = true; 
	GivesParachute[563] = true; 
	GivesParachute[577] = true; 
	GivesParachute[592] = true; 
	GivesParachute[593] = true; 
	sampgdk_initialize_plugin(ppData);
	cout << "---Loading---\r\n\tGamer_Z's Project Bundle: \r\n\t\tAnti Cheat R" << PLUGIN_MAIN_VERSION << "\r\n---LOADED---";
	return true;
}

AMX_NATIVE_INFO driftAMXNatives[ ] =
{
	{"CheckSet",n_CheckSet},
	{"FixSet",n_FixSet},
	{"SetMaxPing",n_SetMaxPing},
	{"SetSpawnKillDelay",n_SetSpawnKillTime},
	{"SetInactivityDelay",n_SetInactivityDelay},
	{"SetWeaponAllowed",n_SetWeaponAllowed},
	{"UseChatForInactivityMeasurement",n_UseChatForInactivityMeasurement},
	{"AntiCheatSetUpdateDelay",n_AntiCheatSetUpdateDelay},
	{"SetIPConnectDelay",n_SetIPConnectDelay},
	{"SetMaxMassTPDelay",n_SetMaxMassTPDelay},
	{0,                0}
};

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad( AMX *amx ) 
{
	//Code from sscanf2.5, [PAWN]SSCANF Author: Y_Less
	int
		num,
		idx;
	// Operate on the raw AMX file, don't use the amx_ functions to avoid issues
	// with the fact that we've not actually finished initialisation yet.  Based
	// VERY heavilly on code from "amx.c" in the PAWN runtime library.
	AMX_HEADER *
		hdr = (AMX_HEADER *)amx->base;
	AMX_FUNCSTUB *
		func;
	num = NUMENTRIES(hdr, natives, libraries);
	for (idx = 0; idx != num; ++idx)
	{
		func = GETENTRY(hdr, natives, idx);
		if (!strcmp("GetPlayerMoney", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_safeGetPlayerMoney;
		}
		if (!strcmp("GivePlayerMoney", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_safeGivePlayerMoney;
		}
		if (!strcmp("ResetPlayerMoney", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_safeResetPlayerMoney;
		}
		if (!strcmp("SetSpawnInfo", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_safeSetSpawnInfo;
		}
		if (!strcmp("AddPlayerClass", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_safeAddPlayerClass;
		}
		if (!strcmp("AddPlayerClassEx", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_safeAddPlayerClassEx;
		}
		if (!strcmp("GivePlayerWeapon", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_safeGivePlayerWeapon;
		}
		if (!strcmp("ResetPlayerWeapons", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_safeResetPlayerWeapons;
		}
		if (!strcmp("SpawnPlayer", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_fixSpawnPlayer;
		}
		if (!strcmp("SetPlayerRaceCheckpoint", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_fixSetPlayerRaceCheckpoint;
		}
		if (!strcmp("SetPlayerCheckpoint", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_fixSetPlayerCheckpoint;
		}
		if (!strcmp("TogglePlayerControllable", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_fixTogglePlayerControllable;
		}
		if (!strcmp("PutPlayerInVehicle", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_fixPutPlayerInVehicle;
		}
		if (!strcmp("SetPlayerPos", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_fixSetPlayerPos;
		}
		if (!strcmp("SetPlayerPosFindZ", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_fixSetPlayerPosFindZ;
		}
		if (!strcmp("SetVehiclePos", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_fixSetVehiclePos;
		}
		if (!strcmp("TogglePlayerSpectating", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_fixTogglePlayerSpectating;
		}
		if (!strcmp("SetPlayerSpecialAction", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_safeSetPlayerSpecialAction;
		}
		if (!strcmp("Kick", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_safeKick;
		}
		if (!strcmp("Ban", GETENTRYNAME(hdr, func)))
		{
			func->address = (ucell)n_safeBan;
		}
	}
	//End of sscanf cut
	amx_list.push_back(amx);
	return amx_Register( amx, driftAMXNatives, -1 );
}

PLUGIN_EXPORT bool PLUGIN_CALL OnPlayerText(int playerid, const char *text)
{
	if(UseChatForInactivityMeasurement)
	{
		p_AcivityInfo[playerid].LastActive = clock();
		p_AcivityInfo[playerid].LastTick = p_AcivityInfo[playerid].LastActive;
		if(p_AcivityInfo[playerid].Reported)
		{
			Report(playerid,CHECK_BACK_FROM_INACTIVITY);
		}
		p_AcivityInfo[playerid].Reported = false;
	}
	return true;
}

PLUGIN_EXPORT bool PLUGIN_CALL OnPlayerConnect(int playerid)
{
	GetPlayerIp(playerid,OnPlayerConnectszIP);
	sscanf(OnPlayerConnectszIP, "%d.%d.%d.%d", &OnPlayerConnectexplodeIP[0], &OnPlayerConnectexplodeIP[1], &OnPlayerConnectexplodeIP[2], &OnPlayerConnectexplodeIP[3]); 
	PlayerIPIndex[playerid] = (OnPlayerConnectexplodeIP[3] + (OnPlayerConnectexplodeIP[2] << 8) + (OnPlayerConnectexplodeIP[1] << 16) + (OnPlayerConnectexplodeIP[0] << 24));
	if(IPLastConnectTime.find(PlayerIPIndex[playerid]) == IPLastConnectTime.end())
	{
		IPLastConnectTime[PlayerIPIndex[playerid]] = clock();
	}
	else
	{
		if((clock()-IPLastConnectTime[PlayerIPIndex[playerid]]) < g_IPConnectDelay)
		{
			Report(playerid,CHECK_FASTCONNECT,PlayerIPIndex[playerid]);
		}
		IPLastConnectTime[PlayerIPIndex[playerid]] = clock();
	}
	p_Frozen[playerid] = true;
	if(!IsPlayerNPC(playerid))
	{
		for(int i = 0; i < 48; ++i)
		{
			p_WeaponEnabled[playerid][i] = g_WeaponEnabled[i];
		}
		p_AcivityInfo[playerid].LastActive = clock();
		p_AcivityInfo[playerid].LastTick = p_AcivityInfo[playerid].LastActive;
		p_AcivityInfo[playerid].Reported = false;
		p_HasJetPack[playerid] = false;
		p_VehicleEnterTime[playerid] = p_AcivityInfo[playerid].LastTick;
		p_IsSpectateAllowed[playerid] = false;
		SetSpawnInfo(playerid, NO_TEAM, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0);
		SetPlayerColor(playerid,PlayerColors[playerid % 100]);
		DisablePlayerCheckpoint(playerid);
		DisablePlayerRaceCheckpoint(playerid);
		PlayerLoopList.insert(playerid);
	
		g_PlayerMoney[playerid] = 0;
		p_CheckForStuff[playerid] =-1;
		if(++ConnectedIPs[PlayerIPIndex[playerid]] >= g_max_ip)
		{
			Report(playerid,CHECK_IPFLOOD,PlayerIPIndex[playerid]);
		}
		p_SkipNextAB[playerid] = false;
	}
	return true;
}


PLUGIN_EXPORT bool PLUGIN_CALL OnPlayerDisconnect(int playerid,int reason)
{
	if(!IsPlayerNPC(playerid))
	{
		if(--ConnectedIPs[PlayerIPIndex[playerid]] == 0)
		{
			ConnectedIPs.erase(ConnectedIPs.find(PlayerIPIndex[playerid]));
			IPLastConnectTime.erase(IPLastConnectTime.find(PlayerIPIndex[playerid]));
		}
		for(int i = 1; i < 48; ++i)
		{
			p_HasWeapon[playerid][i] = false;
		}
		ResetPlayerWeapons(playerid);
		PlayerLoopList.erase(PlayerLoopList.find(playerid));
	}
	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL
	ProcessTick()
{
	if(g_Ticked == g_TickMax)
	{
		int playerid = 0;
		int ProcessTicksize = 0;
		int ProcessTickindex = 0;
		InLoop = true;
		for (unordered_set <int>::iterator i = PlayerLoopList.begin(); i != PlayerLoopList.end(); ++i)
		{
			playerid = *i;
			int playerstate = GetPlayerState(playerid);
			int vid = GetPlayerVehicleID(playerid);
			if(g_IS_TRUE(CHECK_INACTIVITY))
			{
				if(p_IS_TRUE(playerid,CHECK_INACTIVITY))
				{
					if(p_AcivityInfo[playerid].Reported == false)
					{
						if((clock() - p_AcivityInfo[playerid].LastActive) > AllowedInactivityTime)
						{
							p_AcivityInfo[playerid].Reported = true;
							Report(playerid,CHECK_INACTIVITY);
							continue;
						}
					}
				}
			}
			if(g_IS_TRUE(CHECK_PING))
			{
				if(p_IS_TRUE(playerid,CHECK_PING))
				{
					if(GetPlayerPing(playerid) > g_max_ping)
					{
						Report(playerid,CHECK_PING);
						continue;
					}
				}
			}
			if(g_IS_TRUE(CHECK_JETPACK))
			{
				if(p_IS_TRUE(playerid,CHECK_JETPACK))
				{
					if(p_HasJetPack[playerid] == false)
					{
						if(GetPlayerSpecialAction(playerid) == SPECIAL_ACTION_USEJETPACK)
						{
							Report(playerid,CHECK_JETPACK);
							continue;
						}
					}
				}
			}
			if(g_IS_TRUE(CHECK_SPEED))
			{
				if(p_IS_TRUE(playerid,CHECK_SPEED))
				{
					if(playerstate == PLAYER_STATE_DRIVER)
					{
						float s[3];
						GetVehicleVelocity(vid,&s[0],&s[1],&s[2]);
						s[0] *= s[0];
						s[1] *= s[1];
						if(s[2] < -0.023f)
						{
							if(sqrt(s[0]+s[1])*174.0f > (MaxVehicleSpeed[GetVehicleModel(vid)]+10.00f))
							{
								Report(playerid,CHECK_SPEED,vid);
								continue;
							}
						}
						else
						{
							s[2] *= s[2];
							if(sqrt(s[0]+s[1]+s[2])*174.0f > (MaxVehicleSpeed[GetVehicleModel(vid)]+10.00f))
							{
								Report(playerid,CHECK_SPEED,vid);
								continue;
							}
						}
					}
				}
			}
			if(g_IS_TRUE(CHECK_AIRBREAK) || g_IS_TRUE(CHECK_TELEPORT))
			{
				if(p_IS_TRUE(playerid,CHECK_AIRBREAK) || g_IS_TRUE(CHECK_TELEPORT))
				{   
					float SPEEDx = 0.0f;
					float SPEEDu = 0.0f;
					float VEC[3];
					bool Checks[3];
					clock_t diff = clock()-p_AirBrkCheck[playerid].LastMeasured; 
					switch(playerstate)
					{
						case PLAYER_STATE_ONFOOT:
						GetPlayerVelocity(playerid,&VEC[0],&VEC[1],&VEC[2]);
						GetPlayerPos(playerid,&p_AirBrkCheck[playerid].newX,&p_AirBrkCheck[playerid].newY,&p_AirBrkCheck[playerid].newZ);
						Checks[2] = GetPlayerSurfingObjectID(playerid) == INVALID_OBJECT_ID;
						if(VEC[2] >= -0.00535f)
						{	
							if(p_SkipNextAB[playerid])
							{
								SPEEDx = (((sqrt(((p_AirBrkCheck[playerid].newX-p_AirBrkCheck[playerid].lastX)*(p_AirBrkCheck[playerid].newX-p_AirBrkCheck[playerid].lastX))+((p_AirBrkCheck[playerid].newY-p_AirBrkCheck[playerid].lastY)*(p_AirBrkCheck[playerid].newY-p_AirBrkCheck[playerid].lastY)))/((diff/1000)+0.0001f)))+0.000000000000001f);
								SPEEDu = sqrt(VEC[0]*VEC[0]+VEC[1]*VEC[1]);
								Checks[0] = ((SPEEDu/SPEEDx)*100.0f < 0.00001f) && !IsPlayerInRangeOfPoint(playerid,15.0f,p_AirBrkCheck[playerid].lastX,p_AirBrkCheck[playerid].lastY,p_AirBrkCheck[playerid].newZ); 
								p_SkipNextAB[playerid] = false;
							}
							else
							{
								SPEEDx = (((sqrt(((p_AirBrkCheck[playerid].newX-p_AirBrkCheck[playerid].lastX)*(p_AirBrkCheck[playerid].newX-p_AirBrkCheck[playerid].lastX))+((p_AirBrkCheck[playerid].newY-p_AirBrkCheck[playerid].lastY)*(p_AirBrkCheck[playerid].newY-p_AirBrkCheck[playerid].lastY))+((p_AirBrkCheck[playerid].newZ-p_AirBrkCheck[playerid].lastZ)*(p_AirBrkCheck[playerid].newZ-p_AirBrkCheck[playerid].lastZ)))/((diff/1000)+0.0001f)))+0.000000000000001f);
								SPEEDu = sqrt(VEC[0]*VEC[0]+VEC[1]*VEC[1]+VEC[2]*VEC[2]);
								Checks[0] = ((SPEEDu/SPEEDx)*100.0f < 0.00001f) && !IsPlayerInRangeOfPoint(playerid,15.0f,p_AirBrkCheck[playerid].lastX,p_AirBrkCheck[playerid].lastY,p_AirBrkCheck[playerid].lastZ); 	
							}
						}
						else
						{
							SPEEDx = (((sqrt(((p_AirBrkCheck[playerid].newX-p_AirBrkCheck[playerid].lastX)*(p_AirBrkCheck[playerid].newX-p_AirBrkCheck[playerid].lastX))+((p_AirBrkCheck[playerid].newY-p_AirBrkCheck[playerid].lastY)*(p_AirBrkCheck[playerid].newY-p_AirBrkCheck[playerid].lastY)))/((diff/1000)+0.0001f)))+0.000000000000001f);
							SPEEDu = sqrt(VEC[0]*VEC[0]+VEC[1]*VEC[1]);
							Checks[0] = ((SPEEDu/SPEEDx)*100.0f < 0.00001f) && !IsPlayerInRangeOfPoint(playerid,15.0f,p_AirBrkCheck[playerid].lastX,p_AirBrkCheck[playerid].lastY,p_AirBrkCheck[playerid].newZ); 
							p_SkipNextAB[playerid] = true;
						}
						Checks[1] = SPEEDx >= 15.0f && SPEEDu < 0.33000f;
						p_AirBrkCheck[playerid].LastMeasured = clock();
						p_AirBrkCheck[playerid].lastX = p_AirBrkCheck[playerid].newX;
						p_AirBrkCheck[playerid].lastY = p_AirBrkCheck[playerid].newY;
						p_AirBrkCheck[playerid].lastZ = p_AirBrkCheck[playerid].newZ;
						if((Checks[0] && Checks[2]) ||  (Checks[1] && Checks[2]))
						{
							if(Checks[0] && Checks[1])
							{
								if(SPEEDx < 9000.0f)
								{
									if(SPEEDx < 250.0f)
									{
										Report(playerid,CHECK_AIRBREAK,100,SPEEDx);
									}
									else
									{
										Report(playerid,CHECK_TELEPORT,100,SPEEDx);
									}
								}
							}
							else
							{
								if(SPEEDx < 9000.0f)
								{
									if(SPEEDx < 250.0f)
									{
										Report(playerid,CHECK_AIRBREAK,50,SPEEDx);
									}
									else
									{
										Report(playerid,CHECK_TELEPORT,50,SPEEDx);
									}
								}
							}
							break;
						}
						break;
						case PLAYER_STATE_DRIVER:
						GetVehicleVelocity(vid,&VEC[0],&VEC[1],&VEC[2]);
						GetVehiclePos(vid,&p_AirBrkCheck[playerid].newX,&p_AirBrkCheck[playerid].newY,&p_AirBrkCheck[playerid].newZ);
						Checks[2] = GetPlayerSurfingObjectID(playerid) == INVALID_OBJECT_ID;
						if(VEC[2] >= -0.00535f)
						{	
							if(p_SkipNextAB[playerid])
							{
								SPEEDx = (((sqrt(((p_AirBrkCheck[playerid].newX-p_AirBrkCheck[playerid].lastX)*(p_AirBrkCheck[playerid].newX-p_AirBrkCheck[playerid].lastX))+((p_AirBrkCheck[playerid].newY-p_AirBrkCheck[playerid].lastY)*(p_AirBrkCheck[playerid].newY-p_AirBrkCheck[playerid].lastY)))/((diff/1000)+0.0001f)))+0.000000000000001f);
								SPEEDu = sqrt(VEC[0]*VEC[0]+VEC[1]*VEC[1]);
								Checks[0] = ((SPEEDu/SPEEDx)*100.0f < 0.00001f) && !IsPlayerInRangeOfPoint(playerid,20.0f,p_AirBrkCheck[playerid].lastX,p_AirBrkCheck[playerid].lastY,p_AirBrkCheck[playerid].newZ); 
								p_SkipNextAB[playerid] = false;
							}
							else
							{
								SPEEDx = (((sqrt(((p_AirBrkCheck[playerid].newX-p_AirBrkCheck[playerid].lastX)*(p_AirBrkCheck[playerid].newX-p_AirBrkCheck[playerid].lastX))+((p_AirBrkCheck[playerid].newY-p_AirBrkCheck[playerid].lastY)*(p_AirBrkCheck[playerid].newY-p_AirBrkCheck[playerid].lastY))+((p_AirBrkCheck[playerid].newZ-p_AirBrkCheck[playerid].lastZ)*(p_AirBrkCheck[playerid].newZ-p_AirBrkCheck[playerid].lastZ)))/((diff/1000)+0.0001f)))+0.000000000000001f);
								SPEEDu = sqrt(VEC[0]*VEC[0]+VEC[1]*VEC[1]+VEC[2]*VEC[2]);
								Checks[0] = ((SPEEDu/SPEEDx)*100.0f < 0.00001f) && !IsPlayerInRangeOfPoint(playerid,20.0f,p_AirBrkCheck[playerid].lastX,p_AirBrkCheck[playerid].lastY,p_AirBrkCheck[playerid].lastZ); 	
							}
						}
						else
						{
							SPEEDx = (((sqrt(((p_AirBrkCheck[playerid].newX-p_AirBrkCheck[playerid].lastX)*(p_AirBrkCheck[playerid].newX-p_AirBrkCheck[playerid].lastX))+((p_AirBrkCheck[playerid].newY-p_AirBrkCheck[playerid].lastY)*(p_AirBrkCheck[playerid].newY-p_AirBrkCheck[playerid].lastY)))/((diff/1000)+0.0001f)))+0.000000000000001f);
							SPEEDu = sqrt(VEC[0]*VEC[0]+VEC[1]*VEC[1]);
							Checks[0] = ((SPEEDu/SPEEDx)*100.0f < 0.001f) && !IsPlayerInRangeOfPoint(playerid,20.0f,p_AirBrkCheck[playerid].lastX,p_AirBrkCheck[playerid].lastY,p_AirBrkCheck[playerid].newZ); 
							p_SkipNextAB[playerid] = true;
						}
						Checks[1] = SPEEDx >= 75.0f && SPEEDu < 0.40000f;
						p_AirBrkCheck[playerid].LastMeasured = clock();
						p_AirBrkCheck[playerid].lastX = p_AirBrkCheck[playerid].newX;
						p_AirBrkCheck[playerid].lastY = p_AirBrkCheck[playerid].newY;
						p_AirBrkCheck[playerid].lastZ = p_AirBrkCheck[playerid].newZ;
						if((Checks[0] && Checks[2]) ||  (Checks[1] && Checks[2]))
						{
							if(Checks[0] && Checks[1])
							{
								if(SPEEDx < 9000.0f)
								{
									if(SPEEDx < 250.0f)
									{
										//if(SPEEDx < 40.0 && p_ABneecnextdetect[playerid] == false)
										//{
										//	p_ABneecnextdetect[playerid] = true;
										//}
										//else
										//{
										//	p_ABneecnextdetect[playerid] = false;
										Report(playerid,CHECK_AIRBREAK,100,SPEEDx);
										//}
									}
									else
									{
										Report(playerid,CHECK_TELEPORT,100,SPEEDx);
									}
								}
							}
							else
							{
								if(SPEEDx < 9000.0f)
								{
									if(SPEEDx < 250.0f)
									{
										//if(SPEEDx < 40.0 && p_ABneecnextdetect[playerid] == false)
										//{
										//	p_ABneecnextdetect[playerid] = true;
										//}
										//else
										//{
										//	p_ABneecnextdetect[playerid] = false;
										Report(playerid,CHECK_AIRBREAK,100,SPEEDx);
										//}
									}
									else
									{
										Report(playerid,CHECK_TELEPORT,50,SPEEDx);
									}
								}
							}
							break;
						}
						break;
					}
				}
			}
			if(g_IS_TRUE(CHECK_SPECTATE))
			{
				if(p_IS_TRUE(playerid,CHECK_SPECTATE))
				{
					if(playerstate == PLAYER_STATE_SPECTATING)
					{
						if(p_IsSpectateAllowed[playerid] == false)
						{
							Report(playerid,CHECK_SPECTATE);
						}
					}
				}
			}
			if(g_IS_TRUE(CHECK_WEAPON))
			{
				if(p_IS_TRUE(playerid,CHECK_WEAPON))
				{
					if(playerstate == PLAYER_STATE_ONFOOT)
					{
						for(int i = 0; i < 13; ++i)
						{
							int weapon;
							int ammo;
							GetPlayerWeaponData(playerid,i,&weapon,&ammo);
							if(weapon > 0 && weapon != 40 && ammo > 0)
							{
								if(g_WeaponEnabled[weapon] == true)
								{
									if(p_WeaponEnabled[playerid][weapon] == true)
									{
										if(p_HasWeapon[playerid][weapon] == false)
										{
											Report(playerid,CHECK_WEAPON,weapon,0);
											break;
										}
									}
									else
									{
										Report(playerid,CHECK_WEAPON,weapon,1);
										break;
									}
								}
								else
								{
									Report(playerid,CHECK_WEAPON,weapon,1);
									break;
								}
							}
						}
					}
					else
					{
						int weapon = GetPlayerWeapon(playerid);
						if(weapon > 0 && weapon != 40)
						{
							if(g_WeaponEnabled[weapon] == true)
							{
								if(p_WeaponEnabled[playerid][weapon] == true)
								{
									if(p_HasWeapon[playerid][weapon] == false)
									{
										Report(playerid,CHECK_WEAPON,weapon,0);
										break;
									}
								}
								else
								{
									Report(playerid,CHECK_WEAPON,weapon,1);
									break;
								}
							}
							else
							{
								Report(playerid,CHECK_WEAPON,weapon,1);
								break;
							}
						}
					}
				}
			}
		}
		InLoop = false;
		for (unordered_set <int>::iterator i = ToKick.begin(); i != ToKick.end(); ++i)
		{
			Kick(*i);
		}
		ToKick.clear();
		for (unordered_set <int>::iterator i = ToBan.begin(); i != ToBan.end(); ++i)
		{
			Ban(*i);
		}
		ToBan.clear();
		g_Ticked = -1;
	}
	g_Ticked += 1;
}

int OPUvid;
PLUGIN_EXPORT bool PLUGIN_CALL OnPlayerUpdate(int playerid)
{
	p_LastAnimation[playerid] = GetPlayerAnimationIndex(playerid);
	if(g_IS_TRUE(CHECK_INACTIVITY))
	{
		if(p_IS_TRUE(playerid,CHECK_INACTIVITY))
		{
			if(clock()-p_AcivityInfo[playerid].LastTick < 300)
			{
				p_AcivityInfo[playerid].LastActive = clock();
				if(p_AcivityInfo[playerid].Reported == true)
				{
					Report(playerid,CHECK_BACK_FROM_INACTIVITY);
					p_AcivityInfo[playerid].Reported = false;
					p_AcivityInfo[playerid].LastTick = clock();
					return p_Frozen[playerid];
				}
				p_AcivityInfo[playerid].Reported = false;
			}
			p_AcivityInfo[playerid].LastTick = clock();
		}
	}
	//Credits go to Sasuke78200 for explaining me why so, and how to detect this kind of cheats
	if(g_IS_TRUE(CHECK_REMOTECONTROL))
	{
		OPUvid = GetPlayerVehicleID( playerid );
		 if(GetPlayerState( playerid ) == PLAYER_STATE_DRIVER && p_IsInVehicle[ playerid ] != OPUvid )
		 {
			 Report(playerid,CHECK_REMOTECONTROL,v_PlayerInVehicle[OPUvid]);
			 return p_Frozen[playerid];
		 }
	}
	return p_Frozen[playerid];
}

PLUGIN_EXPORT bool PLUGIN_CALL OnPlayerStateChange(int playerid, int newstate, int oldstate)
{
	p_IsInVehicle[playerid] = GetPlayerVehicleID(playerid);
	if(newstate == PLAYER_STATE_DRIVER)
	{
		if(GivesParachute[GetVehicleModel(GetPlayerVehicleID(playerid))])
		{
			p_HasWeapon[playerid][46] = true;
			p_WeaponEnabled[playerid][46] = true;
		}
		v_PlayerInVehicle[p_IsInVehicle[playerid]] = playerid;
		if(g_IS_TRUE(CHECK_MASSCARTELEPORT))
		{
			if(oldstate == PLAYER_STATE_DRIVER)
			{
				if((clock()-p_VehicleEnterTime[playerid]) < g_MassTpDelay)
				{
					p_VehicleEnterTime[playerid] = clock();
					Report(playerid,CHECK_MASSCARTELEPORT);
					return true;
				}
			}
			p_VehicleEnterTime[playerid] = clock();
		}
		if(g_IS_TRUE(CHECK_CARJACKHACK))
		{
			for (unordered_set <int>::iterator i = PlayerLoopList.begin(); i != PlayerLoopList.end(); ++i)
			{
				if(p_IsInVehicle[ playerid ] == p_IsInVehicle[ *i ] && GetPlayerState( *i ) == PLAYER_STATE_ONFOOT)
				{
					Report(playerid,CHECK_CARJACKHACK,*i);
					return true;
				}
			}
		}
	}
	else
	{
		if(p_IsInVehicle[playerid] != 0)
		{
			v_PlayerInVehicle[p_IsInVehicle[playerid]] = -1;
			p_IsInVehicle[playerid] = 0;
		}
		if(g_IS_TRUE(CHECK_MASSCARTELEPORT))
		{
			if(oldstate == PLAYER_STATE_DRIVER)
			{
				if((clock()-p_VehicleEnterTime[playerid]) < g_MassTpDelay)
				{
					Report(playerid,CHECK_MASSCARTELEPORT);
					return true;
				}
			}
		}
	}
	return true;
}

PLUGIN_EXPORT bool PLUGIN_CALL OnPlayerSpawn(int playerid)
{
	if(!IsPlayerNPC(playerid))
	{
		int pSkin = GetPlayerSkin(playerid);
		p_HasWeapon[playerid][g_SkinHasWeapon[pSkin][0]] = true;
		p_HasWeapon[playerid][g_SkinHasWeapon[pSkin][1]] = true;
		p_HasWeapon[playerid][g_SkinHasWeapon[pSkin][2]] = true;
		p_HasWeapon[playerid][p_SpawnInfoWeapons[playerid][0]] = true;
		p_HasWeapon[playerid][p_SpawnInfoWeapons[playerid][1]] = true;
		p_HasWeapon[playerid][p_SpawnInfoWeapons[playerid][2]] = true;
		p_SpawnTime[playerid] = clock();
		p_AirBrkCheck[playerid].LastMeasured = p_SpawnTime[playerid];
		GetPlayerPos(playerid,&p_AirBrkCheck[playerid].lastX,&p_AirBrkCheck[playerid].lastY,&p_AirBrkCheck[playerid].lastZ);
		p_AirBrkCheck[playerid].lastX = p_AirBrkCheck[playerid].newX+0.2f;
		p_AirBrkCheck[playerid].lastY = p_AirBrkCheck[playerid].newY+0.2f;
		p_AirBrkCheck[playerid].lastZ = p_AirBrkCheck[playerid].newZ+0.2f;
		p_ABneecnextdetect[playerid] = false;
	}
	return true;
}

PLUGIN_EXPORT bool PLUGIN_CALL OnPlayerDeath(int playerid, int killerid, int reason)
{
	for(int i = 1; i < 48; ++i)
	{
		p_HasWeapon[playerid][i] = false;
	}
	p_HasJetPack[playerid] = false;
	ResetPlayerWeapons(playerid);
	char
		sAnimlib[32],
		sAnimname[32];
	GetAnimationName(p_LastAnimation[playerid], sAnimlib, sizeof (sAnimlib), sAnimname, sizeof (sAnimname));
	if (!strcmp(sAnimlib, "PED"))
	{
		ClearAnimations(playerid,true);
	}
	if(killerid != INVALID_PLAYER_ID)
	{
		if(g_IS_TRUE(CHECK_SPOOFKILL))
		{
			if(p_IS_TRUE(playerid,CHECK_SPOOFKILL))
			{
				float POS[3];
				int killerweap = GetPlayerWeapon(killerid);
				GetPlayerPos(playerid,&POS[0],&POS[1],&POS[2]);
				bool Checks[7];
				float probability = 0.0f;
				switch ( reason ) 
				{
					case 40 : 
					case 43 : 
					case 44 : 
					case 45 : 
					case 46 : 
					Checks[0] = IsPlayerInRangeOfPoint(killerid,10.0f,POS[0],POS[1],POS[2]) == false;
					Checks[1] = killerweap != reason;
					Checks[2] = p_HasWeapon[killerid][reason] == false;
					Checks[3] = IsPlayerStreamedIn(playerid, killerid) == false;
					Checks[4] = g_WeaponEnabled[reason] == false;
					Checks[5] = p_WeaponEnabled[killerid][reason] == false;
					Checks[6] = GetPlayerInterior(playerid) != GetPlayerInterior(killerid);
					if(Checks[0])probability+=10.0f;
					if(Checks[1])probability+=20.0f;
					if(Checks[2])probability+=10.0f;
					if(Checks[3])probability+=20.0f;
					if(Checks[4])probability+=10.0f;
					if(Checks[5])probability+=10.0f;
					if(Checks[6])probability+=20.0f;
					if(Checks[0] || Checks[1] || Checks[2] || Checks[3] || Checks[4] || Checks[5] || Checks[6])
					{
						Report(playerid,CHECK_SPOOFKILL,killerid,probability);
						return true;
					}
					break;
					case 0 : 
					case 1 : 
					case 2 : 
					case 3 : 
					case 4 : 
					case 5 : 
					case 6 : 
					case 7 : 
					case 8 : 
					case 9 : 
					case 10 : 
					case 11 : 
					case 12 : 
					case 13 : 
					case 14 : 
					case 15 : 
					case 41 : 
					case 42 : 
					Checks[0] = IsPlayerInRangeOfPoint(killerid,15.0f,POS[0],POS[1],POS[2]) == false;
					Checks[1] = killerweap != reason;
					Checks[2] = p_HasWeapon[killerid][reason] == false;
					Checks[3] = IsPlayerStreamedIn(playerid, killerid) == false;
					Checks[4] = g_WeaponEnabled[reason] == false;
					Checks[5] = p_WeaponEnabled[killerid][reason] == false;
					Checks[6] = GetPlayerInterior(playerid) != GetPlayerInterior(killerid);
					if(Checks[0])probability+=10.0f;
					if(Checks[1])probability+=20.0f;
					if(Checks[2])probability+=10.0f;
					if(Checks[3])probability+=20.0f;
					if(Checks[4])probability+=10.0f;
					if(Checks[5])probability+=10.0f;
					if(Checks[6])probability+=20.0f;
					if(Checks[0] || Checks[1] || Checks[2] || Checks[3] || Checks[4] || Checks[5] || Checks[6])
					{
						Report(playerid,CHECK_SPOOFKILL,killerid,probability);
						return true;
					}
					break;
					case 16 : 
					case 17 : 
					case 18 : 
					Checks[0] = IsPlayerInRangeOfPoint(killerid,100.0f,POS[0],POS[1],POS[2]) == false;
					Checks[1] = killerweap != reason;
					Checks[2] = p_HasWeapon[killerid][reason] == false;
					Checks[3] = IsPlayerStreamedIn(playerid, killerid) == false;
					Checks[4] = g_WeaponEnabled[reason] == false;
					Checks[5] = p_WeaponEnabled[killerid][reason] == false;
					Checks[6] = GetPlayerInterior(playerid) != GetPlayerInterior(killerid);
					if(Checks[0])probability+=10.0f;
					if(Checks[1])probability+=20.0f;
					if(Checks[2])probability+=10.0f;
					if(Checks[3])probability+=20.0f;
					if(Checks[4])probability+=10.0f;
					if(Checks[5])probability+=10.0f;
					if(Checks[6])probability+=20.0f;
					if(Checks[0] || Checks[1] || Checks[2] || Checks[3] || Checks[4] || Checks[5] || Checks[6])
					{
						Report(playerid,CHECK_SPOOFKILL,killerid,probability);
						return true;
					}				
					break;

					case 22:
					case 23:
					case 24:
					case 25:
					case 26:
					case 27:
					case 28:
					case 29:
					case 30:
					case 31:
					case 32:
					case 33:
					case 34:
					case 35:
					case 36:
					case 37:
					case 38:
					Checks[0] = IsPlayerInRangeOfPoint(killerid,250.0f,POS[0],POS[1],POS[2]) == false;
					Checks[1] = killerweap != reason;
					Checks[2] = p_HasWeapon[killerid][reason] == false;
					Checks[3] = IsPlayerStreamedIn(playerid, killerid) == false;
					Checks[4] = g_WeaponEnabled[reason] == false;
					Checks[5] = p_WeaponEnabled[killerid][reason] == false;
					Checks[6] = GetPlayerInterior(playerid) != GetPlayerInterior(killerid);
					if(Checks[0])probability+=10.0f;
					if(Checks[1])probability+=20.0f;
					if(Checks[2])probability+=10.0f;
					if(Checks[3])probability+=20.0f;
					if(Checks[4])probability+=10.0f;
					if(Checks[5])probability+=10.0f;
					if(Checks[6])probability+=20.0f;
					if(Checks[0] || Checks[1] || Checks[2] || Checks[3] || Checks[4] || Checks[5] || Checks[6])
					{
						Report(playerid,CHECK_SPOOFKILL,killerid,probability);
						return true;
					}	
					break;
				}
			}
		}
		if(g_IS_TRUE(CHECK_SPAWNKILL))
		{
			if(p_IS_TRUE(playerid,CHECK_SPAWNKILL))
			{
				if(clock()-p_SpawnTime[playerid] < TimeDiffForSpawnKill)
				{
					Report(killerid,CHECK_SPAWNKILL,reason,playerid);
					return true;
				}
			}
		}
	}
	return true;
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() 
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload( )
{
	cout << "---Unloading---\r\n\tGamer_Z's Project Bundle: \r\n\t\tAnti Cheat\r\n---UNLOADED---";
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload( AMX *amx ) 
{
	amx_list.remove(amx);
	return AMX_ERR_NONE;
}