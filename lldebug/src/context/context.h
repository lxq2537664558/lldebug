/*
 * Copyright (c) 2005-2008  cielacanth <cielacanth AT s60.xrea.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __LLDEBUG_CONTEXT_H__
#define __LLDEBUG_CONTEXT_H__

#include "sysinfo.h"
#include "luainfo.h"
#include "queue_mt.h"
#include "net/command.h"

namespace lldebug {
namespace context {

class MainFrame;

enum TableType {
	TABLETYPE_GLOBAL,
	TABLETYPE_REGISTRY,
};

class Context {
public:
	enum State {
		STATE_INITIAL,
		STATE_NORMAL,
		STATE_STEPOVER,
		STATE_STEPINTO,
		STATE_STEPRETURN,
		STATE_BREAK,
		STATE_QUIT,
	};

public:
	static Context *Create();
	virtual void Delete();

	static Context *Find(lua_State *L);
	virtual void Quit();

	void OutputLuaError(const char *str);
	void OutputError(const std::string &str);
	void OutputLog(const std::string &str);
	void OutputLog(LogType type, const std::string &str);

	int LoadFile(const char *filename);
	int LoadString(const char *str);

	int LuaOpenBase(lua_State *L);
	void LuaOpenLibs(lua_State *L);

	LuaVarList LuaGetFields(TableType type);
	LuaVarList LuaGetFields(const LuaVar &var);
	LuaVarList LuaGetLocals(const LuaStackFrame &stackFrame, bool checkLocal,
							bool checkUpvalue, bool checkEnviron);
	LuaVarList LuaGetStack();
	LuaBacktraceList LuaGetBacktrace();

	int LuaEval(lua_State *L, int level, const std::string &str);
	LuaVarList LuaEvalsToVarList(const string_array &array, const LuaStackFrame &stackFrame);
	LuaVarList LuaEvalToMultiVar(const std::string &str, const LuaStackFrame &stackFrame);
	LuaVar LuaEvalToVar(const std::string &str, const LuaStackFrame &stackFrame);

	/// Get the context ID.
	int GetId() {
		scoped_lock lock(m_mutex);
		return m_id;
	}

	/// Get the current lua_State object.
	lua_State *GetLua() {
		scoped_lock lock(m_mutex);
		return m_coroutines.back().L;
	}

	/// Get the first lua_State object.
	lua_State *GetMainLua() {
		scoped_lock lock(m_mutex);
		return m_lua;
	}

	/// Is debug enable ?
	bool IsDebugEnabled() {
		scoped_lock lock(m_mutex);
		return m_isEnabled;
	}

	/// Set whether the debug is enabled.
	void SetDebugEnable(bool enabled) {
		scoped_lock lock(m_mutex);
		m_isEnabled = enabled;
	}

private:
	explicit Context();
	virtual ~Context();
	virtual int Initialize();
	virtual int CreateDebuggerFrame();
	virtual int LoadConfig();
	virtual int SaveConfig();
	virtual void SetState(State state);
	void OnRemoteCommand(const Command &command);
	int HandleCommand();

	static void SetHook(lua_State *L);
	virtual void HookCallback(lua_State *L, lua_Debug *ar);
	static void s_HookCallback(lua_State *L, lua_Debug *ar);

	class LuaImpl;
	friend class LuaImpl;
	int LuaInitialize(lua_State *L);
	void BeginCoroutine(lua_State *L);
	void EndCoroutine(lua_State *L);

	/// Data parsed the lua error.
	struct LuaErrorData {
		LuaErrorData(const std::string &msg_, const std::string &filekey_,
				  int line_)
			: message(msg_), filekey(filekey_), line(line_) {
		}
		std::string message;
		std::string filekey;
		int line;
	};
	LuaErrorData ParseLuaError(const std::string &str);

private:
	class ContextManager;
	static shared_ptr<ContextManager> ms_manager;
	static int ms_idCounter;

	mutex m_mutex;
	int m_id;
	lua_State *m_lua;
	State m_state;
	bool m_isEnabled;
	int m_updateCount;
	int m_waitUpdateCount;
	bool m_isMustUpdate;

	/**
	 * @brief Saving the call count of each lua_State object.
	 *
	 * Used by the safe 'StepOver' and 'StepReturn', etc....
	 */
	struct CoroutineInfo {
		CoroutineInfo(lua_State *L_ = NULL, int call_ = 0)
			: L(L_), call(call_) {
		}
		lua_State *L;
		int call;
	};
	typedef std::vector<CoroutineInfo> CoroutineList;
	CoroutineList m_coroutines;
	CoroutineInfo m_stepinfo;

	queue_mt<Command> m_readCommands;
	condition m_commandCond;

	shared_ptr<RemoteEngine> m_engine;
	SourceManager m_sourceManager;
	BreakpointList m_breakpoints;
	std::string m_rootFileKey;
};

} // end of namespace context
} // end of namespace lldebug

#endif
