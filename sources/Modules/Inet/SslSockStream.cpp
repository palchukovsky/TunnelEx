/**************************************************************************
 *   Created: 2010/12/09 22:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "SslSockStream.hpp"
#include "Core/Error.hpp"
#include "Core/Exceptions.hpp"
#include "Core/Log.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Mods::Inet;
using namespace TunnelEx::Helpers::Crypto;

//////////////////////////////////////////////////////////////////////////

extern "C" {
	int TunnelExModsInetSslBioNew(BIO *);
	int TunnelExModsInetSslBioFree(BIO *);
	int TunnelExModsInetSslBioRead(BIO *, char *, int);
	int TunnelExModsInetSslBioWrite(BIO *, const char *, int);
	long TunnelExModsInetSslBioCtrl(BIO *, int, long, void *);
	int TunnelExModsInetSslBioPuts(BIO *, const char *);
}

//////////////////////////////////////////////////////////////////////////

int TunnelExModsInetSslBioNew(BIO *bio) {
  bio->init = 0;
  bio->num = 0;
  bio->ptr = 0;
  bio->flags = 0;
  return 1;
}

int TunnelExModsInetSslBioFree(BIO *bio) {
	if (!bio || !bio->shutdown) {
		return 0;
	}
	bio->ptr = 0;
	bio->init = 0;
	bio->num = 0;
	bio->flags = 0;
	return 1;
}

int TunnelExModsInetSslBioRead(BIO *bio, char *buf, int len) {
	BIO_clear_retry_flags(bio);
	if (bio->init == 0 || bio->ptr == 0 || buf == 0 || len <= 0) {
		return -1;
	}
	SslSockStream &stream = *static_cast<SslSockStream *>(bio->ptr);
	BIO_clear_retry_flags(bio);
	int errVal = 0;
	int retVal = stream.BioRead(buf, len, errVal);
	if (retVal >= 0) {
		assert(retVal != 0);
		return retVal;
	}
	if (errVal == EINPROGRESS) {
		BIO_set_retry_read(bio);
	}
	return -1;
}

int TunnelExModsInetSslBioWrite(BIO *bio, const char *buf, int len) {
	BIO_clear_retry_flags(bio);
	if (bio->init == 0 || bio->ptr == 0 || buf == 0 || len <= 0) {
		return -1;
	}
	SslSockStream &stream = *static_cast<SslSockStream *>(bio->ptr);
	BIO_clear_retry_flags(bio);
	int errVal = 0;
	int retVal = stream.BioWrite(buf, len, errVal);
	if (retVal >= 0) {
		assert(retVal != 0);
		return retVal;
	}
	if (errVal == EINPROGRESS) {
		BIO_set_retry_write(bio);
	}
	return -1;
}

long TunnelExModsInetSslBioCtrl(BIO *bio, int cmd, long num, void *ptr) {
	long ret = 1;
	switch (cmd) {
		case BIO_C_SET_FILE_PTR:
			bio->shutdown = static_cast<int>(num);
			bio->ptr = ptr;
			bio->init = 1;
			break;
		case BIO_CTRL_INFO:
			ret = 0;
			break;
		case BIO_CTRL_GET_CLOSE:
			ret = bio->shutdown;
			break;
		case BIO_CTRL_SET_CLOSE:
			bio->shutdown = static_cast<int>(num);
			break;
		case BIO_CTRL_PENDING:
		case BIO_CTRL_WPENDING:
			ret = 0;
			break;
		case BIO_CTRL_DUP:
		case BIO_CTRL_FLUSH:
			ret = 1;
			break;
		default:
			ret = 0;
			break;
	}
	return ret;
}

int TunnelExModsInetSslBioPuts(BIO *bio, const char *str) {
	return TunnelExModsInetSslBioWrite(
		bio,
		str,
		ACE_Utils::truncate_cast<int>(ACE_OS::strlen (str)));
}

//////////////////////////////////////////////////////////////////////////

std::wstring SslSockStream::Exception::GetLastError() {
	
	const Error sysError(errno);
	const OpenSslError openSslError(OpenSslError::GetLast(true));

	if (!sysError.IsError() && openSslError.GetErrorsNumb() == 0) {
		return L"protocol error";
	} else {
		WFormat message(L"%1% (%2%)");
		if (sysError.IsError() || openSslError.GetErrorsNumb() == 0) {
			if (openSslError.GetErrorsNumb() > 0) {
				Log::GetInstance().AppendDebug(openSslError.GetAsString());
			}
			if (	!sysError.CheckError()
					&& openSslError.CheckError(sysError.GetErrorNo())) {
				const std::string errorStr
					= OpenSslError::ErrorNoToString(sysError.GetErrorNo());
				message % ConvertString<WString>(errorStr.c_str()).GetCStr();
			} else {
				message % sysError.GetString().GetCStr();
			}
		} else {
			message % ConvertString<WString>(openSslError.GetAsString()).GetCStr();
		}
		message % sysError.GetErrorNo();
		return message.str();
	}

}

SslSockStream::Exception::Exception() throw()
		: LocalException(GetLastError().c_str()) {
	//...//
}

SslSockStream::Exception::Exception(const Exception &rhs) throw()
		: LocalException(rhs) {
	//...//
}

SslSockStream::Exception::~Exception() throw() {
	//...//
}

SslSockStream::Exception & SslSockStream::Exception::operator =(
			const Exception &rhs)
		throw() {
	LocalException::operator =(rhs);
	return *this;
}

AutoPtr<LocalException> SslSockStream::Exception::Clone() const {
	return AutoPtr<LocalException>(new Exception(*this));
}

//////////////////////////////////////////////////////////////////////////

SslSockStream::SslSockStream(const ACE_SSL_Context &context)
		: Base(&const_cast<ACE_SSL_Context &>(context)),
		m_isDecryptorEncryptorMode(false),
		m_biorOrig(0),
		m_biowOrig(0) {
	//...//
}

SslSockStream::~SslSockStream() throw() {
	assert(m_biorOrig == 0);
	assert(m_biowOrig == 0);
	assert(GetBuffers().out.size() == 0);
	if (m_biorOrig || m_biowOrig) {
		if (m_biorOrig != m_biowOrig) {
			BIO_free_all(m_biorOrig);
			BIO_free_all(m_biowOrig);
		} else {
			BIO_free_all(m_biorOrig);
		}
	}
}

void SslSockStream::SwitchToDecryptorEncryptorMode() {

	assert(m_biorOrig == 0);
	assert(m_biowOrig == 0);
	if (IsDecryptorEncryptorMode()) {
		return;
	}

	SSL &ssl = *Base::ssl();

	static BIO_METHOD methods = {
		21 | BIO_TYPE_SOURCE_SINK,
		"TunnelExModInetSslSockStream",
		TunnelExModsInetSslBioWrite,
		TunnelExModsInetSslBioRead,
		TunnelExModsInetSslBioPuts,
		0,
		TunnelExModsInetSslBioCtrl,
		TunnelExModsInetSslBioNew,
		TunnelExModsInetSslBioFree,
		0
	};
	BIO *const bio = BIO_new(&methods);
	if (!bio) {
		throw SystemException(
			L"Internal critical error: failed to allocate SSL BIO structure");
	}

	BIO *const biorOrigTmp = SSL_get_rbio(&ssl);
	BIO *const biowOrigTmp = SSL_get_wbio(&ssl);

	BIO *biorOrig = 0;
	BIO *biowOrig = 0;
	if (biorOrigTmp != biowOrigTmp) {
		biorOrig = BIO_dup_chain(biorOrigTmp);
		biowOrig = BIO_dup_chain(biowOrigTmp);
		if (!biorOrig || !biowOrig) {
			if (biorOrig) {
				BIO_free_all(biorOrig);
			}
			if (biowOrig) {
				BIO_free_all(biowOrig);
			}
			BIO_free_all(bio);
			throw SystemException(
				L"Internal critical error: failed to allocate SSL BIO structure");		
		}
	} else if (biorOrigTmp) {
		biorOrig = biowOrig = BIO_dup_chain(biorOrigTmp);
		if (!biorOrig) {
			BIO_free_all(bio);
			throw SystemException(
				L"Internal critical error: failed to allocate SSL BIO structure");		
		}
	} else {
		biorOrig = biowOrig = 0;
	}

	BIO_ctrl(bio, BIO_C_SET_FILE_PTR, 0, this);
	SSL_set_bio(&ssl, bio, bio);

	m_isDecryptorEncryptorMode = true;
	m_biorOrig = biorOrig;
	m_biowOrig = biowOrig;

}

void SslSockStream::SwitchToStreamMode() {
	
	// one-way can switch to this mode without check
	// assert(m_biorOrig != 0);
	// assert(m_biowOrig != 0);
	if (!IsDecryptorEncryptorMode()) {
		return;
	}

	SSL_set_bio(ssl(), m_biorOrig, m_biowOrig);

	m_isDecryptorEncryptorMode = false;
	m_biorOrig = 0;
	m_biowOrig = 0;

}

void SslSockStream::Decrypt(MessageBlock &messageBlock) const {

	assert(GetBuffers().out.size() == 0);
	assert(IsDecryptorEncryptorMode());

	GetBuffers().encryptedMessage = &messageBlock;
	GetBuffers().encryptedMessageReadPos = 0;
	GetBuffers().decryptionSub.resize(messageBlock.GetUnreadedDataSize());
	GetBuffers().decryptionFull.resize(0);

	for ( ; ; ) {
		const ssize_t receviedBytes
			= recv(&GetBuffers().decryptionSub[0], GetBuffers().decryptionSub.size());
		if (receviedBytes == -1) {
			const Error error(errno);
			if (error.GetErrorNo() == EWOULDBLOCK) {
				break;
			}
			GetBuffers().out.resize(0);
			WFormat message(L"Error at SSL data receiving: \"%1% (%2%)\"");
			if (!error.CheckError() && OpenSslError::CheckError(error.GetErrorNo())) {
				const std::string errorStr
					= OpenSslError::ErrorNoToString(error.GetErrorNo());
				message % ConvertString<WString>(errorStr.c_str()).GetCStr();
			} else {
				message % error.GetString().GetCStr();
			}
			message % error.GetErrorNo();
			throw SystemException(message.str().c_str());
		} else if (receviedBytes > 0) {
			GetBuffers().decryptionFull.reserve(
				GetBuffers().decryptionFull.size() + receviedBytes);
			copy(
				GetBuffers().decryptionSub.begin(),
				GetBuffers().decryptionSub.begin() + receviedBytes,
				back_inserter(GetBuffers().decryptionFull));
		} else {
			break;
		}
	}

	assert(
		GetBuffers().encryptedMessage->GetUnreadedDataSize()
		== GetBuffers().encryptedMessageReadPos);

	if (GetBuffers().decryptionFull.size() > 0) {
		messageBlock.SetData(
			&GetBuffers().decryptionFull[0],
			GetBuffers().decryptionFull.size());
	} else {
		messageBlock.SetData(0, 0);
	}

}

void SslSockStream::Encrypt(MessageBlock &messageBlock) const {

	assert(messageBlock.GetUnreadedDataSize() > 0);
	assert(IsDecryptorEncryptorMode());

	GetBuffers().out.resize(0);
	ssize_t sendPos = 0;

	for ( ; ; ) {
		const ssize_t bytesToSend = messageBlock.GetUnreadedDataSize() - sendPos;
		const ssize_t sentBytes
			= send(messageBlock.GetData() + sendPos, bytesToSend);
		if (sentBytes == -1) {
			const Error error(errno);
			assert(error.GetErrorNo() != EWOULDBLOCK);
			if (error.GetErrorNo() == EWOULDBLOCK) {
				break;
			}
			GetBuffers().out.resize(0);
			WFormat message(L"Error at SSL data sending: \"%1% (%2%)\"");
			if (!error.CheckError() && OpenSslError::CheckError(error.GetErrorNo())) {
				const std::string errorStr
					= OpenSslError::ErrorNoToString(error.GetErrorNo());
				message % ConvertString<WString>(errorStr.c_str()).GetCStr();
			} else {
				message % error.GetString().GetCStr();
			}
			message % error.GetErrorNo();
			throw SystemException(message.str().c_str());
		} else if (sentBytes != bytesToSend) {
			assert(sentBytes < bytesToSend);
			sendPos += sentBytes;
		} else {
			assert(sentBytes == bytesToSend);
			break;
		}
	}

	messageBlock.SetData(&GetBuffers().out[0], GetBuffers().out.size());
	GetBuffers().out.resize(0);

}

int SslSockStream::BioWrite(const char *buf, size_t len, int &errVal) {
	assert(IsDecryptorEncryptorMode());
	assert(len > 0);
	try {
		GetBuffers().out.reserve(GetBuffers().out.size() + len);
		copy(buf, buf + len, back_inserter(GetBuffers().out));
		errVal = 0; // Ok, go ahead
		return ACE_Utils::truncate_cast<int>(len);
	} catch (const std::exception &ex) {
		Log::GetInstance().AppendSystemError(ex.what());
	} catch (...) {
		Log::GetInstance().AppendError("Unknown error at SslSockStream::BioWrite.");
	}
	errVal = EINVAL;
	return -1;
}

int SslSockStream::BioRead(char *buf, size_t len, int &errVal) {

	assert(len > 0);
	assert(IsDecryptorEncryptorMode());

	if (!GetBuffers().encryptedMessage) {
		errVal = EINPROGRESS;
		return -1;
	}

	const size_t dataLen = std::min(
		GetBuffers().encryptedMessage->GetUnreadedDataSize() - GetBuffers().encryptedMessageReadPos,
		len);
	if (dataLen == 0) {
		errVal = EINPROGRESS;
		return -1;
	}

	errVal = 0;
	ACE_OS::memcpy(
		buf,
		GetBuffers().encryptedMessage->GetData() + GetBuffers().encryptedMessageReadPos,
		dataLen);
	GetBuffers().encryptedMessageReadPos += dataLen;
	assert(
		GetBuffers().encryptedMessageReadPos
		<= GetBuffers().encryptedMessage->GetUnreadedDataSize());
	
	return ACE_Utils::truncate_cast<int>(dataLen);

}

void SslSockStream::Connect() {

	assert(GetBuffers().out.size() == 0);
	assert(IsDecryptorEncryptorMode());
	assert(!IsConnected());
	if (IsConnected()) {
		return;
	}

	// Check if a connection is already pending for the given SSL
	// structure.
	if (!SSL_in_connect_init(ssl())) {
		assert(!SSL_in_accept_init(ssl()));
		SSL_set_connect_state(ssl());
	}

	GetBuffers().encryptedMessage = 0;

	try {
		ConnectSsl();
	} catch (...) {
		GetBuffers().out.resize(0);
		throw;
	}

	assert(GetBuffers().out.size() > 0);

}

void SslSockStream::Connect(MessageBlock &messageBlock) {

	assert(GetBuffers().out.size() == 0);
	assert(messageBlock.GetUnreadedDataSize() > 0);
	assert(IsDecryptorEncryptorMode());
	assert(!IsConnected());
	if (IsConnected()) {
		return;
	}

	// Check if a connection is already pending for the given SSL
	// structure.
	if (!SSL_in_connect_init(ssl())) {
		assert(!SSL_in_accept_init(ssl()));
		SSL_set_connect_state(ssl());
	}

	GetBuffers().encryptedMessage = &messageBlock;
	GetBuffers().encryptedMessageReadPos = 0;

	try {
		ConnectSsl();
	} catch (...) {
		GetBuffers().out.resize(0);
		throw;
	}

	messageBlock.SetData(
		messageBlock.GetData() + GetBuffers().encryptedMessageReadPos,
		messageBlock.GetUnreadedDataSize() - GetBuffers().encryptedMessageReadPos);

}

void SslSockStream::ConnectSsl() {

	// copy-past from ACE_SSL_SOCK_Connector::ssl_connect (not fully)

	const int status = SSL_connect(ssl());
	
	switch (SSL_get_error(ssl(), status)) {
		case SSL_ERROR_NONE:
		case SSL_ERROR_WANT_WRITE:
		case SSL_ERROR_WANT_READ:
			return;
		case SSL_ERROR_SYSCALL:
			// On some platforms (e.g. MS Windows) OpenSSL does not
			// store the last error in errno so explicitly do so.
			//
			// Explicitly check for EWOULDBLOCK since it doesn't get
			// converted to an SSL_ERROR_WANT_{READ,WRITE} on some
			// platforms. If SSL_connect failed outright, though, don't
			// bother checking more. This can happen if the socket gets
			// closed during the handshake.
			if (	ACE_OS::set_errno_to_last_error() == EWOULDBLOCK
					&& status == -1) {
				// Although the SSL_ERROR_WANT_READ/WRITE isn't getting
				// set correctly, the read/write state should be valid.
				// Use that to decide what to do.
				if (SSL_want_write(ssl()) || SSL_want_read(ssl())) {
					return;
				} else {
					// Doesn't want anything - bail out
					throw Exception();
				}
			} else {
				throw Exception();
			}
			break;
		case SSL_ERROR_ZERO_RETURN:
			// The peer has notified us that it is shutting down via
			// the SSL "close_notify" message so we need to
			// shutdown, too.
		default:
			throw Exception();
	}

	assert(false);

}

void SslSockStream::Accept() {

	assert(GetBuffers().out.size() == 0);
	assert(IsDecryptorEncryptorMode());
	assert(!IsConnected());
	if (IsConnected()) {
		return;
	}

	// Check if a connection is already pending for the given SSL
	// structure.
	if (!SSL_in_accept_init(ssl())) {
		assert(!SSL_in_connect_init(ssl()));
		SSL_set_accept_state(ssl());
	}

	GetBuffers().encryptedMessage = 0;

	try {
		AcceptSsl();
	} catch (...) {
		GetBuffers().out.resize(0);
		throw;
	}

}

void SslSockStream::Accept(MessageBlock &messageBlock) {

	assert(GetBuffers().out.size() == 0);
	assert(messageBlock.GetUnreadedDataSize() > 0);
	assert(IsDecryptorEncryptorMode());
	assert(!IsConnected());
	if (IsConnected()) {
		return;
	}

	// Check if a connection is already pending for the given SSL
	// structure.
	if (!SSL_in_accept_init(ssl())) {
		assert(!SSL_in_connect_init(ssl()));
		SSL_set_accept_state(ssl());
	}

	GetBuffers().encryptedMessage = &messageBlock;
	GetBuffers().encryptedMessageReadPos = 0;

	try {
		AcceptSsl();
	} catch (...) {
		GetBuffers().out.resize(0);
		throw;
	}

	messageBlock.SetData(
		messageBlock.GetData() + GetBuffers().encryptedMessageReadPos,
		messageBlock.GetUnreadedDataSize() - GetBuffers().encryptedMessageReadPos);

}

void SslSockStream::AcceptSsl() {

	// copy-past from ACE_SSL_SOCK_Connector::ssl_connect (not fully)

	const int status = SSL_accept(ssl());
	
	switch (SSL_get_error(ssl(), status)) {
		case SSL_ERROR_NONE:
		case SSL_ERROR_WANT_WRITE:
		case SSL_ERROR_WANT_READ:
			return;
		case SSL_ERROR_SYSCALL:
			// On some platforms (e.g. MS Windows) OpenSSL does not
			// store the last error in errno so explicitly do so.
			//
			// Explicitly check for EWOULDBLOCK since it doesn't get
			// converted to an SSL_ERROR_WANT_{READ,WRITE} on some
			// platforms. If SSL_connect failed outright, though, don't
			// bother checking more. This can happen if the socket gets
			// closed during the handshake.
			if (	ACE_OS::set_errno_to_last_error() == EWOULDBLOCK
					&& status == -1) {
				// Although the SSL_ERROR_WANT_READ/WRITE isn't getting
				// set correctly, the read/write state should be valid.
				// Use that to decide what to do.
				if (SSL_want_write(ssl()) || SSL_want_read(ssl())) {
					return;
				} else {
					// Doesn't want anything - bail out
					throw Exception();
				}
			} else {
				throw Exception();
			}
			break;
		case SSL_ERROR_ZERO_RETURN:
			// The peer has notified us that it is shutting down via
			// the SSL "close_notify" message so we need to
			// shutdown, too.
		default:
			throw Exception();
	}

	assert(false);

}

//////////////////////////////////////////////////////////////////////////
