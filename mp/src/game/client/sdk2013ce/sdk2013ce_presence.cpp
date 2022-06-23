//=============================================================================
//
// Purpose: Discord Game SDK support.
//
//=============================================================================

#include "cbase.h"
#ifndef POSIX
#include "discord.h"
#endif
#include "sdk2013ce_presence.h"
#include "c_team_objectiveresource.h"
#include "engine/imatchmaking.h"
#include "ixboxsystem.h"
#include "fmtstr.h"
#include "steam/steamclientpublic.h"
#include "steam/isteammatchmaking.h"
#include "steam/isteamgameserver.h"
#include "steam/isteamfriends.h"
#include "steam/steam_api.h"
#include "tier0/icommandline.h"
#include <inetchannelinfo.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef POSIX
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static CSDK2013CEDiscordPresence s_drp;

#define DISCORD_COLOR Color( 114, 137, 218, 255 )


discord::Activity CSDK2013CEDiscordPresence::m_Activity{};
discord::User CSDK2013CEDiscordPresence::m_CurrentUser{};

CSDK2013CEDiscordPresence::CSDK2013CEDiscordPresence()
{
	VCRHook_Time( &m_iCreationTimestamp );

	// Setup early so we catch it
	ListenForGameEvent( "server_spawn" );

	rpc = this;
}

//-----------------------------------------------------------------------------
// Purpose: Catch certain events to update the presence
//-----------------------------------------------------------------------------
void CSDK2013CEDiscordPresence::FireGameEvent( IGameEvent* event )
{
	bool bIsDead = false;
	CUtlString name = event->GetName();

	if (name == "server_spawn")
	{
		Q_strncpy( m_szHostName, event->GetString( "hostname" ), DISCORD_FIELD_MAXLEN );
		Q_strncpy( m_szServerInfo, event->GetString( "address" ), DISCORD_FIELD_MAXLEN );
	}

	if (g_pDiscord == NULL)
		return;

	if ( C_BasePlayer::GetLocalPlayer() == nullptr )
		return;

	if ( !engine->IsConnected() )
		return;

	if ( ( name == "player_connect_client" || name == "player_disconnect" ) && g_pGameRules->IsMultiplayer() )
	{
		if ( name == "player_connect_client" )
		{
			int userid = event->GetInt( "userid" );
			if ( UTIL_PlayerByUserId( userid ) == C_BasePlayer::GetLocalPlayer() )
			{
				CSteamID steamID{};
				if ( C_BasePlayer::GetLocalPlayer()->GetSteamID( &steamID ) )
					V_sprintf_safe( m_szSteamID, "%llu", steamID.ConvertToUint64() );

				m_Activity.GetSecrets().SetJoin( m_szServerInfo );
				m_Activity.GetSecrets().SetMatch( m_szSteamID );
			}
		}

		const int maxPlayers = gpGlobals->maxClients;
		int curPlayers = 0;

		IGameResources* gr = GameResources();

		for ( int i = 0; i < maxPlayers; ++i )
		{
			if ( gr && gr->IsConnected( i ) )
			{
				curPlayers++;
			}
		}

		m_Activity.GetParty().GetSize().SetCurrentSize( curPlayers );
		m_Activity.GetParty().GetSize().SetMaxSize( maxPlayers );
	}
	else if ( name == "player_death" )
	{
		int userid = event->GetInt( "userid" );
		if ( UTIL_PlayerByUserId( userid ) != C_BasePlayer::GetLocalPlayer() )
			return;

		bIsDead = true;
	}

	UpdatePresence( true, bIsDead );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSDK2013CEDiscordPresence::Init(void)
{
	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "player_connect_client" );
	ListenForGameEvent( "player_disconnect" );
	ListenForGameEvent( "client_fullconnect" );
	ListenForGameEvent( "client_disconnect" );

	return BaseClass::Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSDK2013CEDiscordPresence::InitPresence( void )
{
	m_updateThrottle.Start( 30.0f );

	if ( g_pDiscord == NULL )
		return true;

	g_pDiscord->SetLogHook(
#ifdef DEBUG
		discord::LogLevel::Debug,
#else
		discord::LogLevel::Warn,
#endif
		&OnLogMessage
	);

	Q_memset( &m_CurrentUser, 0, sizeof( discord::User ) );
	g_pDiscord->UserManager().OnCurrentUserUpdate.Connect( &OnReady );
	
	Q_memset( &m_CurrentUser, 0, sizeof( discord::User ) );
	g_pDiscord->UserManager().OnCurrentUserUpdate.Connect( &OnReady );
	

	char command[512];
	V_snprintf( command, sizeof( command ), "%s -game \"%s\" -novid -steam", CommandLine()->GetParm( 0 ), CommandLine()->ParmValue( "-game" ) );
	g_pDiscord->ActivityManager().RegisterCommand( command );
	//g_pDiscord->ActivityManager().RegisterSteam( engine->GetAppID() );

	g_pDiscord->ActivityManager().OnActivityJoin.Connect( &OnJoinedGame );
	g_pDiscord->ActivityManager().OnActivityJoinRequest.Connect( &OnJoinRequested );
	g_pDiscord->ActivityManager().OnActivitySpectate.Connect( &OnSpectateGame );

	return true;
}

void CSDK2013CEDiscordPresence::Shutdown( void )
{
	BaseClass::Shutdown();

	Q_memset( &m_Activity, 0, sizeof( discord::Activity ) );
	Q_memset( &m_CurrentUser, 0, sizeof( discord::User ) );

	Assert(rpc == this);
	rpc = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Map initialization
//-----------------------------------------------------------------------------
void CSDK2013CEDiscordPresence::LevelInitPostEntity( void )
{
	Q_memset( &m_Activity, 0, sizeof( discord::Activity ) );

	char szGameState[DISCORD_FIELD_MAXLEN];
	Q_snprintf( szGameState, sizeof( szGameState ), "Map: %s", GetLevelName() );
	m_Activity.GetAssets().SetLargeImage( GetMapImage( "sdk2013ce" ) );
	m_Activity.GetAssets().SetLargeText( "Community Made" );

	// TODO: Fix bug with partyId "secrets must be unique"
	/*if ( engine->IsConnected() && g_pGameRules->IsMultiplayer() )
	{
		INetChannelInfo *ni = engine->GetNetChannelInfo();
		if ( ni && ni->GetAddress() && ni->GetName() )
		{
			char partyId[128];
			sprintf( partyId, "Server: %s", ni->GetName() ); // adding -party here because secrets cannot match the party id
			m_Activity.GetParty().SetId(partyId);
			m_Activity.GetSecrets().SetJoin((ni->GetAddress()));
			m_Activity.GetSecrets().SetSpectate((ni->GetAddress()));
		}
	}*/

	m_Activity.SetDetails( m_szHostName );
	m_Activity.SetState( szGameState );
	m_Activity.GetTimestamps().SetStart( m_iCreationTimestamp );

	if ( g_pDiscord )
	{
		g_pDiscord->ActivityManager().UpdateActivity( m_Activity, &OnActivityUpdate );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDK2013CEDiscordPresence::OnReady()
{
	if ( !rpc->GetSupportsPresence() )
	{
		if ( g_pDiscord )
		{
			delete g_pDiscord;
			g_pDiscord = NULL;
		}

		return;
	}

	g_pDiscord->UserManager().GetCurrentUser( &m_CurrentUser );

	ConColorMsg( DISCORD_COLOR, "[DRP] Ready!\n" );
	ConColorMsg( DISCORD_COLOR, "[DRP] User %s#%s - %lld\n", m_CurrentUser.GetUsername(), m_CurrentUser.GetDiscriminator(), m_CurrentUser.GetId() );

	rpc->ResetPresence();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDK2013CEDiscordPresence::OnJoinedGame(char const* joinSecret)
{
	if ( g_pGameRules->IsMultiplayer() )
	{
		ConColorMsg( DISCORD_COLOR, "[DRP] Join Game: %s\n", joinSecret );
		char szCommand[128];
		Q_snprintf( szCommand, sizeof( szCommand ), "connect %s\n", joinSecret );
		engine->ExecuteClientCmd( szCommand );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDK2013CEDiscordPresence::OnSpectateGame(char const* joinSecret)
{
	if ( g_pGameRules->IsMultiplayer() )
	{
		ConColorMsg( DISCORD_COLOR, "[DRP] Spectate Game: %s\n", joinSecret );
		char szCommand[128];
		Q_snprintf( szCommand, sizeof( szCommand ), "connect %s:27020\n", joinSecret ); // We append this with port 27020, for STV.
		engine->ExecuteClientCmd( szCommand );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDK2013CEDiscordPresence::OnJoinRequested(discord::User const& joinRequester)
{
	if ( g_pGameRules->IsMultiplayer() )
	{
		ConColorMsg( DISCORD_COLOR, "[DRP] Join Request: %s#%s\n", joinRequester.GetUsername(), joinRequester.GetDiscriminator() );
		ConColorMsg( DISCORD_COLOR, "[DRP] Join Request Accepted\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDK2013CEDiscordPresence::OnLogMessage(discord::LogLevel logLevel, char const* pszMessage)
{
	switch ( logLevel )
	{
		case discord::LogLevel::Error:
		case discord::LogLevel::Warn:
			Warning( "[DRP] %s\n", pszMessage );
			break;
		default:
			ConColorMsg( DISCORD_COLOR, "[DRP] %s\n", pszMessage );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDK2013CEDiscordPresence::OnActivityUpdate(discord::Result result)
{
#ifdef DEBUG
	ConColorMsg( DISCORD_COLOR, "[DRP] Activity update: %s\n", ( ( result == discord::Result::Ok ) ? "Succeeded" : "Failed" ) );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Reset map details
//-----------------------------------------------------------------------------
void CSDK2013CEDiscordPresence::LevelShutdownPreEntity( void )
{
	ResetPresence();
}

//-----------------------------------------------------------------------------
// Purpose: Revert to default state
//-----------------------------------------------------------------------------
void CSDK2013CEDiscordPresence::ResetPresence( void )
{
	Q_memset( &m_Activity, 0, sizeof( discord::Activity ) );
	Q_memset( &m_Activity, 0, sizeof( discord::Lobby ) );

	m_Activity.SetDetails( "Main Menu" );
	m_Activity.GetAssets().SetLargeImage( "sdk2013ce" );
	m_Activity.GetTimestamps().SetStart( m_iCreationTimestamp );

	if( g_pDiscord )
	{
		g_pDiscord->ActivityManager().UpdateActivity( m_Activity, &OnActivityUpdate );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CSDK2013CEDiscordPresence::GetMatchSecret( void ) const
{
	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CSDK2013CEDiscordPresence::GetJoinSecret( void ) const
{
	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CSDK2013CEDiscordPresence::GetSpectateSecret( void ) const
{
	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDK2013CEDiscordPresence::UpdatePresence( bool bForce, bool bIsDead )
{
	if ( !m_updateThrottle.IsElapsed() && !bForce )
		return;

	m_updateThrottle.Start( RandomFloat( 15.0, 20.0 ) );

	// TODO: Put something here

	g_pDiscord->ActivityManager().UpdateActivity( m_Activity, &OnActivityUpdate );
}

#endif // !POSIX
