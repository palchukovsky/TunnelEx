/**************************************************************************
 *   Created: 2007/02/13 3:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "Log.hpp"
#include "String.hpp"
#include "PosixTime.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;
using namespace TunnelEx;
using namespace TunnelEx::Singletons;

//////////////////////////////////////////////////////////////////////////

//! Log implementation.
class TunnelEx::Singletons::LogPolicy::Implementation : private boost::noncopyable {

public:

	typedef ACE_Thread_Mutex Mutex;
	typedef ACE_Guard<Mutex> Lock;
	
	typedef Helpers::MultipleStream<char> Stream;
	
	struct LevelInfo {

		LevelInfo()
				: isRegistrationOn(false),
				name("<?>"),
				lastOccurTime(boost::gregorian::date(1970, 1, 1)) {
			//...//
		}

		const char *name;
		bool isRegistrationOn;
		pt::ptime lastOccurTime;
		
	};

	typedef std::vector<LevelInfo> Levels;

public:
	
	Implementation()
			: m_levels(LOG_LEVEL_LEVELS_COUNT),
			m_size(0) {
		m_levels[LOG_LEVEL_UNKNOWN].name = "Unknown";
		m_levels[LOG_LEVEL_TRACK].name = "Tracking";
		m_levels[LOG_LEVEL_DEBUG].name = "Debug";
		m_levels[LOG_LEVEL_INFO].name = "Information";
		m_levels[LOG_LEVEL_WARN].name = "Warning";
		m_levels[LOG_LEVEL_ERROR].name = "Error";
		m_levels[LOG_LEVEL_SYSTEM_ERROR].name = "System error";
		m_levels[LOG_LEVEL_FATAL_ERROR].name = "Fatal error";
	}

	~Implementation() {
		//...//
	}

public:

	bool IsLevelRegistrationOn(const LogLevel levelId) const throw() {
		return GetLevelInfo(levelId).isRegistrationOn;
	}

	void SetLevelRegistrationState(LogLevel levelId, bool state) throw() {
		GetLevelInfo(levelId).isRegistrationOn = state; 
	}

	void SetMinimumRegistrationLevel(LogLevel levelId) throw() {
		Lock lock(m_levelsMutex);
		const Levels::iterator begin = m_levels.begin();
		const Levels::iterator end = m_levels.end();
		for (Levels::iterator i = begin; i != end; ++i) {
			i->isRegistrationOn = distance(begin, i) >= levelId;
		}
	}

	LevelInfo & GetLevelInfo(LogLevel levelId) throw() {
		assert(static_cast<size_t>(levelId) < m_levels.size());
		assert(levelId < LOG_LEVEL_LEVELS_COUNT);
		if (levelId >= LOG_LEVEL_LEVELS_COUNT) {
			levelId = LOG_LEVEL_UNKNOWN;
		}
		return m_levels[levelId];
	}

	const LevelInfo & GetLevelInfo(LogLevel levelId) const throw() {
		return const_cast<Implementation *>(this)->GetLevelInfo(levelId);
	}

	void CheckAndAppend(const LogLevel levelId, const std::string &message) throw() {
		LevelInfo &levelInfo = GetLevelInfo(levelId);
		if (!levelInfo.isRegistrationOn) {
			return;
		}
		Append(levelInfo, message);
	}

	void Append(LevelInfo &level, const std::string &message) throw() {
		try {
			const pt::ptime occurTime(pt::microsec_clock::local_time());
			Lock lock(m_streamMutex);
			m_stream
				<< occurTime
				<< ' ' << std::setw(12) << level.name
				<< ": " << message;
			if (*message.rbegin() != '.') {
				m_stream << '.';
			}
			m_stream << std::endl;
			++m_size;
			if (level.lastOccurTime < occurTime) {
				level.lastOccurTime = occurTime;
			}
		} catch (...) {
			assert(false);
		}
	}

	bool AttachFile(const std::wstring &filePath) throw() {
		try {
			try {
				create_directories(fs::wpath(filePath).branch_path());
			} catch (const fs::filesystem_error &ex) {
				Log::GetInstance().AppendError(
					(Format("Log: could not create directory for attached file: \"%1%\".") % ex.what()).str());
			}
			Lock lock(m_streamMutex);
			return m_stream.AttachFile(
				ConvertString<String>(filePath.c_str()).GetCStr());
		} catch (...) {
			return false;
		}
	}

	bool DetachFile(const std::wstring &filePath) throw() {
		try {
			Lock lock(m_streamMutex);
			return m_stream.DetachFile(
				ConvertString<String>(filePath.c_str()).GetCStr());
		} catch (...) {
			return false;
		}
	}

	bool AttachStdoutStream() throw() {
		try {
			Lock lock(m_streamMutex);
			return m_stream.AttachStream(std::cout);
		} catch (...) {
			return false;
		}
	}

	bool DetachStdoutStream() throw() {
		try {
			Lock lock(m_streamMutex);
			return m_stream.DetachStream(std::cout);
		} catch (...) {
			return false;
		}
	}

	bool AttachStderrStream() throw() {
		try {
			Lock lock(m_streamMutex);
			return m_stream.AttachStream(std::cerr);
		} catch (...) {
			return false;
		}
	}

	bool DetachStderrStream() throw() {
		try {
			Lock lock(m_streamMutex);
			return m_stream.DetachStream(std::cerr);
		} catch (...) {
			return false;
		}
	}

	LogLevel ResolveLevel(const char *nameToResolvePch) const throw() {
		try {
			std::string nameToResolve = nameToResolvePch;
			boost::to_lower(nameToResolve);
			//! @todo: optimize search
			const Levels::const_iterator begin = m_levels.begin();
			const Levels::const_iterator end = m_levels.end();
			for (Levels::const_iterator i = begin; i != end; ++i) {
				std::string name = i->name;
				boost::to_lower(name);
				if (name == nameToResolve) {
					return static_cast<LogLevel>(distance(begin, i));
				}
			}
		} catch (...) {
			//...//
		}
		return LOG_LEVEL_UNKNOWN;
	}
	
	unsigned long long GetSize() const {
		return m_size;
	}

	TimeT GetLastOccurTime(const LogLevel levelId) const {
		return Helpers::ConvertPosixTimeToTimeT(GetLevelInfo(levelId).lastOccurTime);
	}

private:

	Mutex m_streamMutex;
	Stream m_stream;
	
	Mutex m_levelsMutex;
	Levels m_levels;
	
	unsigned long long m_size;

};

//////////////////////////////////////////////////////////////////////////

LogPolicy::LogPolicy()
		: m_pimpl(new Implementation) {
	//...//
}

LogPolicy::~LogPolicy() {
	delete m_pimpl;
}

#ifdef LOG_LEVEL_TRACK_REGISTRATION_AVAILABLE
	void LogPolicy::AppendTracking(
				const char *className,
				const char *methodName,
				const char *file,
				const unsigned int line)
			throw() {
		Implementation::LevelInfo &level = m_pimpl->GetLevelInfo(LOG_LEVEL_TRACK);
		if (!level.isRegistrationOn) {
			return;
		}
		std::ostringstream oss;
		oss << ACE_OS::thr_self() << " -> ";
		if (!className[0] == 0) {
			oss << className << "::";
		}
		oss << methodName << " (" << file << ":" << line << ")";
		m_pimpl->Append(level, oss.str());
	}
#endif // LOG_LEVEL_TRACK_REGISTRATION_AVAILABLE

void LogPolicy::AppendDebug(const std::string &message) throw() {
	if (!IsDebugRegistrationOn()) {
		return;
	}
	try {
		std::ostringstream oss;
		oss << ACE_OS::thr_self() << " " << message;
		m_pimpl->Append(m_pimpl->GetLevelInfo(LOG_LEVEL_DEBUG), oss.str());
	} catch (...) {
		assert(false);
		m_pimpl->Append(m_pimpl->GetLevelInfo(LOG_LEVEL_DEBUG), message);
	}
}
void LogPolicy::AppendInfo(const std::string &message) throw() {
	m_pimpl->CheckAndAppend(LOG_LEVEL_INFO, message);
}

void LogPolicy::AppendWarn(const std::string &message) throw() {
	m_pimpl->CheckAndAppend(LOG_LEVEL_WARN, message);
}

void LogPolicy::AppendError(const std::string &message) throw() {
	m_pimpl->CheckAndAppend(LOG_LEVEL_ERROR, message);
}

void LogPolicy::AppendSystemError(const std::string &message) throw() {
	m_pimpl->CheckAndAppend(LOG_LEVEL_SYSTEM_ERROR, message);
}

void LogPolicy::AppendFatalError(const std::string &message) throw() {
	m_pimpl->CheckAndAppend(LOG_LEVEL_FATAL_ERROR, message);
}

void LogPolicy::SetLevelRegistrationState(LogLevel level, bool state) throw() {
	m_pimpl->SetLevelRegistrationState(level, state);
}

void LogPolicy::SetMinimumRegistrationLevel(LogLevel levelId) throw() {
	m_pimpl->SetMinimumRegistrationLevel(levelId);
}

bool LogPolicy::IsDebugRegistrationOn() const throw() {
	return m_pimpl->IsLevelRegistrationOn(LOG_LEVEL_DEBUG);
}

bool LogPolicy::IsInfoRegistrationOn() const throw() {
	return m_pimpl->IsLevelRegistrationOn(LOG_LEVEL_INFO);
}

bool LogPolicy::IsWarnsRegistrationOn() const throw() {
	return m_pimpl->IsLevelRegistrationOn(LOG_LEVEL_WARN);
}

bool LogPolicy::IsCommonErrorsRegistrationOn() const throw() {
	return m_pimpl->IsLevelRegistrationOn(LOG_LEVEL_ERROR);
}

bool LogPolicy::IsSystemErrorsRegistrationOn() const throw() {
	return m_pimpl->IsLevelRegistrationOn(LOG_LEVEL_SYSTEM_ERROR);
}

bool LogPolicy::IsFatalErrorsRegistrationOn() const  throw() {
	return m_pimpl->IsLevelRegistrationOn(LOG_LEVEL_FATAL_ERROR);
}

bool LogPolicy::AttachFile(const std::wstring &filePath) throw() {
	return m_pimpl->AttachFile(filePath);
}

bool LogPolicy::DetachFile(const std::wstring &filePath) throw() {
	return m_pimpl->DetachFile(filePath);
}

bool LogPolicy::AttachStdoutStream() throw() {
	return m_pimpl->AttachStdoutStream();
}

bool LogPolicy::DetachStdoutStream() throw() {
	return m_pimpl->DetachStdoutStream();
}

bool LogPolicy::AttachStderrStream() throw() {
	return m_pimpl->AttachStderrStream();
}

bool LogPolicy::DetachStderrStream() throw() {
	return m_pimpl->DetachStderrStream();
}

LogLevel LogPolicy::ResolveLevel(const char *levelName) const throw() {
	return m_pimpl->ResolveLevel(levelName);
}

unsigned long long LogPolicy::GetSize() const {
	return m_pimpl->GetSize();
}

TimeT LogPolicy::GetLastWarnTime() const {
	return m_pimpl->GetLastOccurTime(LOG_LEVEL_WARN);
}

TimeT LogPolicy::GetLastErrorTime() const {
	return std::max(
		m_pimpl->GetLastOccurTime(LOG_LEVEL_ERROR),
		std::max(
			m_pimpl->GetLastOccurTime(LOG_LEVEL_FATAL_ERROR),
			m_pimpl->GetLastOccurTime(LOG_LEVEL_SYSTEM_ERROR)));
}

#if TEMPLATES_REQUIRE_SOURCE != 0
#	include "Singleton.cpp"
	//! Only for template instantiation.
	void MakeLogTemplateInstantiation() {
		Log::GetInstance();
	}
#endif // TEMPLATES_REQUIRE_SOURCE
