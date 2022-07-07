#include "cbase.h"
#include "srceng_angelscript.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
//------------------------------------------------------------------------------
// Purpose: Disables all NPCs
//------------------------------------------------------------------------------
void CC_PrintVersion(void)
{
	Msg("Version: %s\n", ANGELSCRIPT_VERSION_STRING);
}
static ConCommand as_version("as_version", CC_PrintVersion, "", FCVAR_CLIENTDLL);
#endif

#ifdef  GAME_DLL
CAngelScript g_AngelScript_Server;
const char* m_sName = "GAME_DLL";
#elif	CLIENT_DLL
CAngelScript g_AngelScript_Client;
const char* m_sName = "CLIENT_DLL";
#endif //  GAME_DLL

void MessageCallback( const asSMessageInfo *msg, void *param )
{
	const char *type = "ERR";
	if ( msg->type == asMSGTYPE_WARNING )
	{
		type = "WARN";
		Warning( "CAngelScript (%s): %s (%d, %d) : %s : %s\n", m_sName, msg->section, msg->row, msg->col, type, msg->message );
	}
	else if ( msg->type == asMSGTYPE_INFORMATION )
	{
		type = "INFO";
		Msg( "CAngelScript (%s): %s (%d, %d) : %s : %s\n", m_sName, msg->section, msg->row, msg->col, type, msg->message );
	}
	else
	{
		Error( "CAngelScript (%s): %s (%d, %d) : %s : %s\n", m_sName, msg->section, msg->row, msg->col, type, msg->message );
	}
}

void PrintString()
{
	Msg( "CAngelScript (%s): %s\n", m_sName, "Poooooop" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAngelScript::CAngelScript()
{
	m_ctx = NULL;
	m_hScriptEngine = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAngelScript::Init()
{
	Msg("CAngelScript (%s): Initalizing version %s...\n", m_sName, ANGELSCRIPT_VERSION_STRING);

	// Create the script engine
	m_hScriptEngine = asCreateScriptEngine();

	if (m_hScriptEngine == NULL)
	{
		Warning("CAngelScript (%s): Initalizion failed due to null script engine pointer...\n", m_sName);
		Shutdown();
	}
	else
	{
		// Set the message callback to receive information on errors in human readable form.
		int r = m_hScriptEngine->SetMessageCallback( asFUNCTION( MessageCallback ), 0, asCALL_CDECL );

		if ( r < 0 )
		{
			Warning( "CAngelScript (%s): Initalizion failed due to invalid script message callback code...\n", m_sName );
			Shutdown();
		}

		// Register the function that we want the scripts to call 
		r = m_hScriptEngine->RegisterGlobalFunction( "void Print()", asFUNCTION( PrintString ), asCALL_CDECL );

		if ( r < 0 )
		{
			Warning( "CAngelScript (%s): Failed to register\n", m_sName );
			Shutdown();
		}

		m_ctx = m_hScriptEngine->CreateContext();
		if ( m_ctx == 0 )
		{
			Warning( "CAngelScript (%s): Failed to create the context\n", m_sName );
			Shutdown();
		}

		// The CScriptBuilder helper is an add-on that loads the file,
		// performs a pre-processing pass if necessary, and then tells
		// the engine to build a script module.
		CScriptBuilder builder;
		r = builder.StartNewModule(m_hScriptEngine, "MyModule");
		if (r < 0)
		{
			// If the code fails here it is usually because there
			// is no more memory to allocate the module
			Warning( "CAngelScript (%s): Unrecoverable error while starting a new module.\n", m_sName);
			Shutdown();
		}
		r = builder.AddSectionFromFile( "C:/Users/Dot/Documents/GitHub/source-sdk-2013-ce/sp/game/mod_sdk2013ce/scripts/angelscripts/script.as" );
		if (r < 0)
		{
			// The builder wasn't able to load the file. Maybe the file
			// has been removed, or the wrong name was given, or some
			// preprocessing commands are incorrectly written.
			Warning( "CAngelScript (%s): Please correct the errors in the script and try again.\n", m_sName);
			Shutdown();
		}
		r = builder.BuildModule();
		if (r < 0)
		{
			// An error occurred. Instruct the script writer to fix the 
			// compilation errors that were listed in the output stream.
			Warning( "CAngelScript (%s): Please correct the errors in the script and try again.\n", m_sName);
			Shutdown();
		}

	}
}

void CAngelScript::Shutdown()
{
	m_ctx->Release();
	m_hScriptEngine->ShutDownAndRelease();
}
