#include "cbase.h"
#include "tier1/utlstring.h"
#include "basediscordpresence.h"
#include "filesystem.h"

#ifndef POSIX
#include "discord.h"
discord::Core *g_pDiscord = NULL;
#endif

IRichPresenceClient *rpc = NULL;


#ifndef POSIX

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseDiscordPresence::CBaseDiscordPresence()
	: CAutoGameSystemPerFrame( "Discord RPC" )
{
	m_szMapName[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseDiscordPresence::~CBaseDiscordPresence()
{
	if ( g_pDiscord )
		delete g_pDiscord;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseDiscordPresence::Init()
{
	if ( rpc == NULL )
		rpc = this;

	if ( !GetSupportsPresence() )
		return true;

	Assert( g_pDiscord == NULL );
	auto result = discord::Core::Create( V_atoi64( GetPresenceAppId() ), DiscordCreateFlags_NoRequireDiscord, &g_pDiscord );
	if ( result != discord::Result::Ok )
		return true;

	return InitPresence();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseDiscordPresence::Shutdown()
{
	if ( g_pDiscord )
	{
		delete g_pDiscord;
		g_pDiscord = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseDiscordPresence::Update( float frametime )
{
	if ( g_pDiscord == NULL )
		return;

	UpdatePresence();

	// Update every other tick
	if ( gpGlobals->tickcount % 2 )
		g_pDiscord->RunCallbacks();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CBaseDiscordPresence::GetPresenceAppId( void ) const
{
	const char *ID = "746722719328108615";
	KeyValues *pDiscordRPCID = new KeyValues( "GameInfo" );
	pDiscordRPCID->LoadFromFile( filesystem, "gameinfo.txt" );
	if ( pDiscordRPCID )
	{
		KeyValues *pID = pDiscordRPCID->FindKey( "Discord" );
		if (pID)
		{
			return ID = pID->GetString( "DiscordAppID", "746722719328108615" );
		}
		pID->deleteThis();
		pDiscordRPCID->deleteThis();
	}
	return ID;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseDiscordPresence::GetSupportsPresence( void )
{
	bool IsItOn = 0;
	KeyValues* pDiscordRPCIsUsed = new KeyValues( "GameInfo" );
	pDiscordRPCIsUsed->LoadFromFile( filesystem, "gameinfo.txt" );
	if ( pDiscordRPCIsUsed )
	{
		KeyValues* IsUsedBool = pDiscordRPCIsUsed->FindKey( "Discord" );
		if ( IsUsedBool )
		{
			return IsItOn = IsUsedBool->GetBool( "SupportsDiscordPresence", 1 );
		}
		IsUsedBool->deleteThis();
		pDiscordRPCIsUsed->deleteThis();
	}
	return IsItOn;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CBaseDiscordPresence::GetMapImage( char const *defaultMapIcon )
{
	// TODO: Figure out how this works
	KeyValues *pDiscordRPC = new KeyValues( "Discord" );
	pDiscordRPC->LoadFromFile( filesystem, "scripts/discord_presence.txt" );
	if ( pDiscordRPC )
	{
		KeyValues *pMaps = pDiscordRPC->FindKey( "Maps" );
		if ( pMaps )
		{
			return pMaps->GetString( GetLevelName(), defaultMapIcon );
		}
		pMaps->deleteThis();
		pDiscordRPC->deleteThis();
	}
	return defaultMapIcon;
}

#endif // !POSIX
