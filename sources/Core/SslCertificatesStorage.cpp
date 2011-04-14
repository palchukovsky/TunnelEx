/**************************************************************************
 *   Created: 2010/11/06 17:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: SslCertificatesStorage.cpp 1129 2011-02-22 17:28:50Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "SslCertificatesStorage.hpp"
#include "Exceptions.hpp"
#include "String.hpp"
#include "Log.hpp"

namespace fs = boost::filesystem;
using namespace TunnelEx;
using namespace TunnelEx::Helpers;
using namespace TunnelEx::Helpers::Crypto;

//////////////////////////////////////////////////////////////////////////

class SslCertificatesStorage::Implementation : private boost::noncopyable {

private:

	typedef ACE_RW_Mutex StorageMutex;
	typedef ACE_Read_Guard<StorageMutex> StorageReadLock; 
	typedef ACE_Write_Guard<StorageMutex> StorageWriteLock;

	typedef ACE_RW_Mutex KeyMutex;
	typedef ACE_Read_Guard<KeyMutex> KeyReadLock; 
	typedef ACE_Write_Guard<KeyMutex> KeyWriteLock;

	struct CertificateInfo {
		WString id;
		fs::wpath path;
		boost::shared_ptr<X509Shared> certificate;
	};

	typedef boost::unordered_map<
			WString,
			CertificateInfo,
			Helpers::StringHasher<WString> >
		Storage;

public:

	explicit Implementation(
				const WString &path,
				const unsigned char *key,
				const size_t keySize)
			: m_dbPath(path.GetCStr()),
			m_dbModificationFilePath(m_dbPath / L"db"),
			m_dbModificationTime(-1),
			m_subKey(key, key + keySize) {
		//...//
	}

protected:

	void Reload() {
		try {
			Storage storage;
			const fs::wdirectory_iterator end;
			const time_t modificationTime
				= last_write_time(m_dbModificationFilePath);
			for (fs::wdirectory_iterator i(m_dbPath); i != end; ++i) {
				if (	!is_directory(i->status())
						&& Helpers::StringUtil::IsUuid(i->path().stem())) {
					LoadCertificate(i->path(), storage);
				}			
			}
			StorageWriteLock lock(m_storageMutex);
			storage.swap(m_storage);
			m_dbModificationTime = modificationTime;
		} catch (const boost::system::system_error &ex) {
			Format message("Failed to open SSL certificates storage: %1%.");
			message % ex.what();
			Log::GetInstance().AppendError(message.str());
			throw LocalException(L"Failed to open SSL certificates storage");
		}
	}

	void LoadCertificate(const fs::wpath &file, Storage &storage) const {
		BOOST_ASSERT(storage.find(file.stem().c_str()) == storage.end());
		try {
			if (boost::iequals(file.extension(), L".p12")) {
				LoadPrivateCertificate(file, storage);
			} else if (
					boost::iequals(file.extension(), L".cer")
					|| boost::iequals(file.extension(), L".crt")) {
				LoadSharedCertificate(file, storage);
			}
		} catch (const LocalException &ex) {
			Format message("Failed to load SSL certificate %1%: %2%.");
			message % ConvertString<String>(file.filename().c_str()).GetCStr();
			message %  ConvertString<String>(ex.GetWhat()).GetCStr();
			Log::GetInstance().AppendError(message.str());
		} catch (const Crypto::Exception &ex) {
			Format message("Failed to load SSL certificate %1%: %2%.");
			message % ConvertString<String>(file.filename().c_str()).GetCStr();
			message % ex.what();
			Log::GetInstance().AppendError(message.str());
			try {
				const fs::wpath newName = file.string() + L".error";
				if (exists(newName)) {
					Format message("Removing %1% to backup %2%...");
					message % ConvertString<String>(newName.string().c_str()).GetCStr();
					message % ConvertString<String>(file.string().c_str()).GetCStr();
					Log::GetInstance().AppendWarn(message.str());
					remove(newName);
				}
				rename(file, newName);
			} catch (const boost::system::system_error &ex) {
				Log::GetInstance().AppendSystemError(ex.what());
			}
		}
	}

	void LoadPrivateCertificate(const fs::wpath &file, Storage &storage) const {
		
		Log::GetInstance().AppendDebug(
			"Opening private SSL certificate file \"%1%\"...",
			ConvertString<String>(file.filename().c_str()).GetCStr());

		ifstream f(
			ConvertString<String>(file.string().c_str()).GetCStr(),
			std::ios::binary);
		if (!f) {
			throw LocalException(L"Failed to open file");
		}

		f.seekg(0, std::ios::end);
		std::vector<unsigned char> buffer(unsigned int(f.tellg()));
		f.seekg(0, std::ios::beg);
		f.read(reinterpret_cast<char *>(&buffer[0]), buffer.size());
		f.close();

		CertificateInfo info;
		info.id = file.stem().c_str();
		info.path = file;
		info.certificate
			= Pkcs12(&buffer[0], buffer.size()).GetCertificate(GetKey());

		storage[info.id] = info;

	}

	void LoadSharedCertificate(const fs::wpath &file, Storage &storage) const {

		Log::GetInstance().AppendDebug(
			"Opening SSL certificate file \"%1%\"...",
			ConvertString<String>(file.filename().c_str()).GetCStr());

		ifstream f(
			ConvertString<String>(file.string().c_str()).GetCStr(),
			std::ios::binary);
		if (!f) {
			throw LocalException(L"Failed to open file");
		}

		f.seekg(0, std::ios::end);
		std::vector<unsigned char> buffer(unsigned int(f.tellg()));
		f.seekg(0, std::ios::beg);
		f.read(reinterpret_cast<char *>(&buffer[0]), buffer.size());
		f.close();

		CertificateInfo info;
		info.id = file.stem().c_str();
		info.path = file;
		info.certificate.reset(new X509Shared(&buffer[0], buffer.size()));

		storage[info.id] = info;

	}

	std::string GetKey() const {

		{
			KeyReadLock lock(m_keyMutex);
			if (!m_key.empty()) {
				return m_key;
			}
		}
		
		KeyWriteLock lock(m_keyMutex);
		if (!m_key.empty()) {
			return m_key;
		}
		/*
			IxL!RyQ,B2ujVjI(W`Kyb's gT%QKHo=P?,3Wt*P}25YPCf-[xTAq0Qm{l<'jE7:l&SV"G9>cLP}8bDG@K2+hFj?<jGG&cESG9(^#

			length: 101
		 */
		std::vector<unsigned char> key;
		key.resize(44); key[43] = 'Y'; key[2] = 'L'; key[10] = 'u';
		key.resize(58); key[57] = 'l'; key.resize(96); key[95] = 'S';
		key[21] = '\''; key[34] = ','; key[31] = '='; key[52] = 'q';
		key[83] = '+'; key[92] = '&'; key[80] = '@'; key[75] = '}';
		key[50] = 'T'; key[67] = 'V'; key[71] = '>'; key[91] = 'G';
		key[88] = '<'; key[45] = 'C'; key[66] = 'S'; key.resize(98);
		key[97] = '9'; key[61] = 'E'; key[89] = 'j'; key[72] = 'c';
		key[55] = 'm'; key[22] = 's'; key[59] = '\''; key[18] = 'K';
		key[47] = '-'; key[6] = 'Q'; key[69] = 'G'; key[53] = '0';
		key[84] = 'h'; key[0] = 'I'; key[20] = 'b'; key[33] = '?';
		key.resize(99); key[98] = '('; key[64] = 'l'; key[14] = 'I';
		key[15] = '('; key[90] = 'G'; key[54] = 'Q'; key[13] = 'j';
		key[46] = 'f'; key[48] = '['; key[16] = 'W'; key[5] = 'y';
		key[30] = 'o'; key[44] = 'P'; key[87] = '?'; key[7] = ',';
		key[58] = '<'; key[12] = 'V'; key[49] = 'x'; key[35] = '3';
		key[28] = 'K'; key[40] = '}'; key[41] = '2'; key[29] = 'H';
		key[93] = 'c'; key[42] = '5'; key[62] = '7'; key[8] = 'B';
		key[78] = 'D'; key.resize(101); key[100] = '#'; key[56] = '{';
		key[37] = 't'; key[96] = 'G'; key[73] = 'L'; key[82] = '2';
		key[94] = 'E'; key[39] = 'P'; key[63] = ':'; key[70] = '9';
		key[74] = 'P'; key[85] = 'F'; key[36] = 'W'; key[99] = '^';
		key[38] = '*'; key[26] = '%'; key[81] = 'K'; key[19] = 'y';
		key[76] = '8'; key[60] = 'j'; key[17] = '`'; key[79] = 'G';
		key[86] = 'j'; key[23] = ' '; key[1] = 'x'; key[32] = 'P';
		key[27] = 'Q'; key[9] = '2'; key[24] = 'g'; key[25] = 'T';
		key[4] = 'R'; key[11] = 'j'; key[77] = 'b'; key[3] = '!';
		key[68] = '"'; key[51] = 'A'; key[65] = '&';

		size_t token = 0;
		std::vector<unsigned char>::iterator i = key.begin();
		foreach (char ch, key) {
			ch ^= m_subKey[token++ % m_subKey.size()];
			*i = ch;
			++i;
		}
		BOOST_ASSERT(i == key.end());
		std::string result(key.begin(), key.end());
		result.swap(const_cast<Implementation *>(this)->m_key);
		
		return m_key;

	}

	void CheckDb() {
		try {
			if (!exists(m_dbPath)) {
				return;
			}
			if (!exists(m_dbModificationFilePath)) {
				CheckDbExists();
			} else if (last_write_time(m_dbModificationFilePath) == m_dbModificationTime) {
				return;
			}
			Reload();
		} catch (const boost::system::system_error &ex) {
			Format message("Failed to check SSL certificates storage: %1%.");
			message % ex.what();
			Log::GetInstance().AppendError(message.str());
			throw LocalException(L"Failed to check SSL certificates storage");
		}
	}

	void CheckDbExists() {
		try {
			if (exists(m_dbPath) && exists(m_dbModificationFilePath)) {
				return;
			}
			StorageWriteLock lock(m_storageMutex);
			if (!exists(m_dbPath)) {
				Log::GetInstance().AppendDebug(
					"Creating SSL certificates storage \"%1%\"...",
					ConvertString<String>(m_dbPath.string().c_str()).GetCStr());
				create_directories(m_dbPath);
			}
			if (!exists(m_dbModificationFilePath)) {
				String pathStr;
				ConvertString(m_dbModificationFilePath.string().c_str(), pathStr);
				ofstream(pathStr.GetCStr(), std::ios::trunc);
				last_write_time(m_dbModificationFilePath, 0);
			}
		} catch (const boost::system::system_error &ex) {
			Format message("Failed to create SSL certificates storage: %1%.");
			message % ex.what();
			Log::GetInstance().AppendError(message.str());
			throw LocalException(L"Failed to create SSL certificates storage");
		}
	}

	void TouchDb(const StorageWriteLock &) {
		while (last_write_time(m_dbModificationFilePath) == time(0)) {
			ACE_OS::sleep(1);
		}
		last_write_time(m_dbModificationFilePath, time(0));
	}

public:

	UniquePtr<SslCertificateIdCollection> GetInstalledIds() const {
		const_cast<Implementation *>(this)->CheckDb();
		StorageReadLock lock(m_storageMutex);
		UniquePtr<SslCertificateIdCollection> result(
			new SslCertificateIdCollection(m_storage.size()));
		foreach (const Storage::value_type &info, m_storage) {
			result->Append(info.second.id);
		}
		return result;
	};

	UniquePtr<X509Shared> GetCertificate(const WString &id) const {
		const_cast<Implementation *>(this)->CheckDb();
		StorageReadLock lock(m_storageMutex);
		const Storage::const_iterator pos = m_storage.find(id);
		if (pos == m_storage.end()) {
			lock.release();
			Log::GetInstance().AppendDebug(
				"SSL certificate with ID \"%1%\" does not exist in local storage.",
				ConvertString<String>(id).GetCStr());
			throw NotFoundException(
				L"Failed to find SSL certificate in local storage");
		}
		return UniquePtr<X509Shared>(new X509Shared(*pos->second.certificate));
	}

	bool IsPrivateCertificate(const WString &id) const {
		const_cast<Implementation *>(this)->CheckDb();
		StorageReadLock lock(m_storageMutex);
		const Storage::const_iterator pos = m_storage.find(id);
		if (pos == m_storage.end()) {
			lock.release();
			Log::GetInstance().AppendDebug(
				"SSL certificate with ID \"%1%\" does not exist in local storage.",
				ConvertString<String>(id).GetCStr());
			throw NotFoundException(
				L"Failed to find SSL certificate in local storage");
		}
		return dynamic_cast<X509Private *>(pos->second.certificate.get()) != 0;
	}

	UniquePtr<X509Private> GetPrivateCertificate(const WString &id) const {
		const_cast<Implementation *>(this)->CheckDb();
		StorageReadLock lock(m_storageMutex);
		const Storage::const_iterator pos = m_storage.find(id);
		if (pos == m_storage.end()) {
			lock.release();
			Log::GetInstance().AppendDebug(
				"SSL certificate with ID \"%1%\" does not exist in local storage.",
				ConvertString<String>(id).GetCStr());
			throw NotFoundException(
				L"Failed to find SSL certificate in local storage");
		}
		X509Private *const privateCertificate
			= dynamic_cast<X509Private *>(pos->second.certificate.get());
		if (!privateCertificate) {
			lock.release();
			Log::GetInstance().AppendDebug(
				"SSL certificate \"%1%\" has no private key.",
				ConvertString<String>(id).GetCStr());
			throw NotFoundException(L"SSL certificate has no private key");
		}
		return UniquePtr<X509Private>(new X509Private(*privateCertificate));
	}

	void Insert(const X509Shared &certificate) {
		CheckDbExists();
		const std::wstring fileName = Helpers::Uuid().GetAsString() + L".cer";
		const fs::wpath filePath = m_dbPath / fileName;
		try {
			StorageWriteLock lock(m_storageMutex);
			std::ofstream f(
				ConvertString<String>(filePath.string().c_str()).GetCStr(),
				std::ios::trunc | std::ios::binary);
			if (!f) {
				Format message("Failed to save SSL certificate into %1%.");
				message % ConvertString<String>(filePath.string().c_str()).GetCStr();
				Log::GetInstance().AppendSystemError(message.str());
				throw SystemException(L"Failed to save SSL certificate");
			}
			certificate.Export(f);
			TouchDb(lock);
		} catch (const Crypto::Exception &ex) {
			Format message("Failed to save SSL certificate: %1%.");
			message % ex.what();
			Log::GetInstance().AppendError(message.str());
			try {
				StorageWriteLock lock(m_storageMutex);
				if (exists(filePath)) {
					remove(filePath);
					TouchDb(lock);
				}
			} catch (const boost::system::system_error &ex) {
				Log::GetInstance().AppendSystemError(ex.what());
			}
			throw LocalException(L"Failed to save SSL certificate");
		}
	}

	void Insert(const X509Private &certificate) {
		CheckDbExists();
		const std::wstring fileName = Helpers::Uuid().GetAsString() + L".p12";
		const fs::wpath filePath = m_dbPath / fileName;
		try {
			StorageWriteLock lock(m_storageMutex);
			Pkcs12 pkcs12(certificate, "", GetKey());
			ofstream f(
				ConvertString<String>(filePath.string().c_str()).GetCStr(),
				std::ios::trunc | std::ios::binary);
			if (!f) {
				Format message("Failed to save SSL certificate into %1%.");
				message % ConvertString<String>(filePath.string().c_str()).GetCStr();
				Log::GetInstance().AppendSystemError(message.str());
				throw SystemException(L"Failed to save SSL certificate");
			}
			pkcs12.Export(f);
			f.close();
			TouchDb(lock);
		} catch (const Crypto::Exception &ex) {
			Format message("Failed to save SSL certificate: %1%.");
			message % ex.what();
			Log::GetInstance().AppendError(message.str());
			try {
				StorageWriteLock lock(m_storageMutex);
				if (exists(filePath)) {
					remove(filePath);
					TouchDb(lock);
				}
			} catch (const boost::system::system_error &ex) {
				Log::GetInstance().AppendSystemError(ex.what());
			}
			throw LocalException(L"Failed to save SSL certificate");
		}
	}

	void Delete(const WString &id) {
		CheckDbExists();
		StorageWriteLock lock(m_storageMutex);
		DoDelete(lock, id);
		TouchDb(lock);
	}

	void Delete(const SslCertificateIdCollection &ids) {
		CheckDbExists();
		Log::GetInstance().AppendDebug("Deleting SSL certificates...");
		StorageWriteLock lock(m_storageMutex);
		for (size_t i = 0; i < ids.GetSize(); ++i) {
			DoDelete(lock, ids[i]);
		}
		TouchDb(lock);
	}

protected:

	void DoDelete(const StorageWriteLock &, const WString &id) {
		Log::GetInstance().AppendDebug(
			"Deleting SSL certificate %1%...",
			ConvertString<String>(id).GetCStr());
		const Storage::const_iterator pos = m_storage.find(id);
		if (pos == m_storage.end()) {
			Log::GetInstance().AppendDebug(
				"SSL certificate %1% not exists.",
				ConvertString<String>(id).GetCStr());
			return;
		}
		try {
			if (exists(pos->second.path)) {
				remove(pos->second.path);
			} else {
				Log::GetInstance().AppendDebug(
					"SSL certificate %1% file not exists.",
					ConvertString<String>(id).GetCStr());
			}
			m_storage.erase(pos);
		} catch (const boost::system::system_error &ex) {
			Format message("Failed to delete SSL certificate %1%: %2%.");
			message % ConvertString<String>(id.GetCStr()).GetCStr() % ex.what();
			Log::GetInstance().AppendSystemError(message.str());
			throw LocalException(L"Failed to delete SSL certificate");
		}
	}

private:

	mutable StorageMutex m_storageMutex;
	Storage m_storage;
	const fs::wpath m_dbPath;
	const fs::wpath m_dbModificationFilePath;
	time_t m_dbModificationTime;
	mutable StorageMutex m_keyMutex;
	std::vector<unsigned char> m_subKey;
	std::string m_key;

};

//////////////////////////////////////////////////////////////////////////

SslCertificatesStorage::SslCertificatesStorage(
			const WString &path,
			const unsigned char *key,
			const size_t keySize)
		: m_pimpl(new Implementation(path, key, keySize)) {
	//...//
}

SslCertificatesStorage::~SslCertificatesStorage() throw() {
	delete m_pimpl;
}

UniquePtr<SslCertificateIdCollection> SslCertificatesStorage::GetInstalledIds() const {
	return m_pimpl->GetInstalledIds();
};

UniquePtr<X509Shared> SslCertificatesStorage::GetCertificate(
			const WString &id)
		const {
	return m_pimpl->GetCertificate(id);
}

bool SslCertificatesStorage::IsPrivateCertificate(
			const WString &certificateId)
		const {
	return m_pimpl->IsPrivateCertificate(certificateId);
}

UniquePtr<X509Private> SslCertificatesStorage::GetPrivateCertificate(
			const WString &id)
		const {
	return m_pimpl->GetPrivateCertificate(id);
}

void SslCertificatesStorage::Insert(const X509Shared &certificate) {
	m_pimpl->Insert(certificate);
}

void SslCertificatesStorage::Insert(const X509Private &certificate) {
	m_pimpl->Insert(certificate);
}

void SslCertificatesStorage::Delete(const WString &id) {
	m_pimpl->Delete(id);
}

void SslCertificatesStorage::Delete(const SslCertificateIdCollection &ids) {
	m_pimpl->Delete(ids);
}

//////////////////////////////////////////////////////////////////////////

#if TEMPLATES_REQUIRE_SOURCE != 0
#	include "Collection.cpp"
	namespace {
		//! Only for template instantiation.
		void MakeTemplateInstantiation() {
			TunnelEx::Helpers::MakeCollectionTemplateInstantiation<SslCertificateIdCollection>();
		}
	}
#endif // TEMPLATES_REQUIRE_SOURCE

//////////////////////////////////////////////////////////////////////////
