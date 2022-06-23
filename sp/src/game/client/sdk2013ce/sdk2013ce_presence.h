//=============================================================================
//
// Purpose: Discord Game SDK support.
//
//=============================================================================

#ifndef SDK2013CE_PRESENCE_H
#define SDK2013CE_PRESENCE_H
#ifdef _WIN32
#pragma once
#endif

#include "GameEventListener.h"
#include "basepresence.h"
#include "hl2orange.spa.h"
#include "basediscordpresence.h"
#include "types.h"
#include "util_shared.h"

#ifndef POSIX
class CSDK2013CEDiscordPresence : public CBaseDiscordPresence, public CGameEventListener
{
	DECLARE_CLASS_GAMEROOT( CSDK2013CEDiscordPresence, CBaseDiscordPresence );
public:

	CSDK2013CEDiscordPresence();
	virtual ~CSDK2013CEDiscordPresence() {};

	virtual void FireGameEvent( IGameEvent *event );

	virtual bool		Init( void );
	virtual void		Shutdown( void );
	virtual void		LevelInitPostEntity( void );
	virtual void		LevelShutdownPreEntity( void );

	bool				InitPresence( void ) OVERRIDE;
	void				ResetPresence( void ) OVERRIDE;
	void				UpdatePresence( void ) OVERRIDE { UpdatePresence( false, false ); }
	char const*			GetMatchSecret( void ) const OVERRIDE;
	char const*			GetJoinSecret( void ) const OVERRIDE;
	char const*			GetSpectateSecret( void ) const OVERRIDE;

private:
	void				UpdatePresence( bool bForce, bool bIsDead );
	char const*			GetEncryptionKey( void ) const OVERRIDE { return "XwRJxjCc"; }

	char m_szHostName[ DISCORD_FIELD_MAXLEN ];
	char m_szServerInfo[ DISCORD_FIELD_MAXLEN ];
	char m_szSteamID[ DISCORD_FIELD_MAXLEN ];

	RealTimeCountdownTimer m_updateThrottle;
	long m_iCreationTimestamp;

	static discord::Activity m_Activity;
	static discord::User m_CurrentUser;

	static void OnReady();
	static void OnJoinedGame( char const *joinSecret );
	static void OnSpectateGame( char const *joinSecret );
	static void OnJoinRequested( discord::User const &joinRequester );
	static void OnLogMessage( discord::LogLevel logLevel, char const *pszMessage );
	static void OnActivityUpdate( discord::Result result );
};
#endif // !POSIX

#endif // SDK2013CE_PRESENCE_H
