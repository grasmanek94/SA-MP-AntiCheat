#if defined ANTICHEAT_PLUGIN_INCLUDED

#else

#define ANTICHEAT_PLUGIN_INCLUDED
#pragma library "AntiCheatPlugin"

#define AC_INCLUDE_VERSION				(25)

#define CHEAT_JETPACK			(1)		// player got jetpack
#define CHEAT_WEAPON			(2)		// player got not allowed/spawned weapon, extraint = weaponid, extraint = type cheat, 1 - weapon not allowed, 0 - spawned weapon
#define CHEAT_SPEED				(4)		// player used speedhack, extraint = vehicleid
#define CHEAT_HEALTHARMOUR      (8)		// needs to be implemented, detected health/armour hack, extraint 0 = health, extraint 1 = armour, extrafloat = amount of current health/armour
#define CHEAT_IPFLOOD			(16)	// too many connections from one ip, extraint = ip in binary format (exploded by '.' 127.0.0.1 <=^=> (1 + (0 << 8) + (0 << 16) + (127 << 24)))
#define CHEAT_PING				(32)	// player has too high ping, default = 65537
#define CHEAT_SPOOFKILL			(64)	// playerid - the one who cheated and died, extraint = the victim, extrafloat - the probability of detection, ranges from 0.0 to 100.0, it's in percentage of success detection, 100.0 = cheater for sure! 0.0 ... 30.0 ~ can be sa-mp bug.
#define CHEAT_SPAWNKILL			(128)	// playerid = killerid, extraint = reason, extraint2 = killedplayerid
#define CHEAT_INACTIVITY		(256)	// player is inactive for too long (default = 3 minutes = 180000 ms), none
#define CHEAT_TELEPORT			(512)	// extraint = percentage as integer (this time ye.. it can only be 50 or 100) of how accurate the detection is, extrafloat - is the calculated speed from XYZ positioning
#define CHEAT_AIRBREAK			(1024)	// extraint = percentage as integer (this time ye.. it can only be 50 or 100) of how accurate the detection is, extrafloat - is the calculated speed from XYZ positioning
#define CHEAT_BACK_FROM_INACTIVITY (2048) // if CHEAT_INACTIVITY is called and then the player is back then this is called.
#define CHEAT_SPECTATE			(4096)	//anti spectate-hack
#define CHEAT_FASTCONNECT		(8192)	//if some playerids connect from one ip very fast twice, or three times (checks if one ip connects in very short delays)
#define CHEAT_REMOTECONTROL		(16384) //extraint = victims playerid, playerid - cheater, detects if playerid manipulated victimids vehicle (eg set super fast speed)
#define CHEAT_MASSCARTELEPORT	(32768) //checks if playerid changed vehicles VERY fast (for teleporting them?)
#define CHEAT_CARJACKHACK		(65536) //check if player uses cheats to steal a car

//note that enabling anti teleport will enable anti airbreak and vice-versa

native FixSet(Fix,bool:enabled=true);//No fixes to enable/disable yet... All included fixes are auto enabled, sorry for that but that's for now the best way imho.
//example - enable color fix - FixSet(FIX_GETPLAYERCOLOR,true);
native CheckSet(ToCheck,playerid = (-1), bool:check=true);//enable/disable checking for jetpack, -1 for all players, -1 overides normal player settins so -1,true will enable normal checking again.
//example - disable speedhack checking for player 5: CheckSet(CHEAT_SPEED,5,false);
native SetMaxPing(ping=65537);//sets the max ping a player can have before being reported
native SetWeaponAllowed(playerid=(-1),weaponid=0,bool:allowed=true);//sets the weaponid to allowed/disallowed, -1 overides all players, no matter what you set to individual players after and before the function
native SetSpawnKillDelay(delay=2000);//Set the delay in miliseconds which will count as spawnkill, if killerid kills playerid within 'delay' miliseconds, a spawnkill will be reported.
native SetInactivityDelay(delay=180000);//Set the delay after how many miliseconds INACTIVITY will be detected.
native UseChatForInactivityMeasurement(bool:use=true);//if false and player sends a message he still will be counted as inactive
native AntiCheatSetUpdateDelay(ticks=200);//anti airbreak and anti teleport breaks if ticks is not 200....
native SetIPConnectDelay(delay=100);//if any ip connects twice faster than 'delay' miliseconds then the OnCheatDetected callback will be called
native SetMaxMassTPDelay(delay=150);//if player changes vehicles within 'delay' miliseconds report him to OnCheatDetected.

forward AC_OnCheatDetected(playerid, type, extraint, Float:extrafloat, extraint2);
/*
	playerid - the id of the player who cheated
	type - the type of cheat, CHEAT_JETPACK,CHEAT_WEAPON,etc...
	extraint - an extra integer variable
	extrafloat - an extra float variable

	NOTE:
	if CHEAT_IPFLOOD (and all other ip checks) is detected, then AC_OnCheatDetected is called BEFORE OnPlayerConnect
	ANY OnPlayerConnect from ANY script, 
	the OnPLayerConnect from the plugin is executing, so do not use global player variables which you assign in OnPlayerConnect yourself!

	example of doing it wrong:


	/////////////WRONG SCRIPT//////////////
	#include <a_samp>
	#include <AntiCheat>

	new GlobalPlayerIP[MAX_PLAYERS][17];
	new GlobalPlayerName[MAX_PLAYERS][33];

	public OnPlayerConnect(playerid)
	{
		GetPlayerName(playerid,GlobalPlayerName,33);
		GetPlayerIP(playerid,GlobalPlayerIP,17);
		return 1;
	}

	//THE WRONG WAY
	public AC_OnCheatDetected(playerid, type, extraint, Float:extrafloat)
	{
		switch(type)
		{
			case CHEAT_IPFLOOD:
			{
				MyBanIP(GlobalPlayerIP);//your custom banip function
				AddNameToBans(GlobalPlayerName);//your custom function to add name to banlist
			}
		}
		return 1;
	}

	//This code will do: 
	BanIP("");
	AddNameToBans("");


	doing it right:

	/////////////GOOD SCRIPT//////////////
	#include <a_samp>
	#include <AntiCheat>

	new GlobalPlayerIP[MAX_PLAYERS][17];
	new GlobalPlayerName[MAX_PLAYERS][33];

	public OnPlayerConnect(playerid)
	{
		GetPlayerName(playerid,GlobalPlayerName,33);
		GetPlayerIP(playerid,GlobalPlayerIP,17);
		return 1;
	}

	//THE GOOD WAY
	public AC_OnCheatDetected(playerid, type, extraint, Float:extrafloat)
	{
		switch(type)
		{
			case CHEAT_IPFLOOD:
			{
				GetPlayerName(playerid,GlobalPlayerName,33);
				GetPlayerIP(playerid,GlobalPlayerIP,17);
				MyBanIP(GlobalPlayerIP);//your custom banip function
				MyAddNameToBans(GlobalPlayerName);//your custom function to add name to banlist
			}
		}
		return 1;
	}
*/
#endif

/*
	-Makes your Money functions safe, in all script, even in script which don't have this include included.
	-Anti Jetpack
	-Anti Weapon hack

	NOTE:
	1) For fully compatible anti weapon hack make sure you don't have pickups which give weapons automticly, instead script the pickups to give weapons using the sa-mp natives.
	2) All veding machines will be removed if you use this plugin (for anti health hack to work)
	3) This plugin uses some Y_Less's fixes from http://forum.sa-mp.com/showthread.php?t=292813
*/
