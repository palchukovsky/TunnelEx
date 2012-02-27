/**************************************************************************
 *   Created: 2010/12/09 22:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__SslSockStream_hpp__1012092215
#define INCLUDED_FILE__TUNNELEX__SslSockStream_hpp__1012092215

#include "Core/Exceptions.hpp"
#include "Core/MessageBlock.hpp"

namespace TunnelEx { namespace Mods { namespace Inet {

	class SslSockStream : public ACE_SSL_SOCK_Stream {

	public:

		class Exception : public TunnelEx::LocalException {
		public:
			Exception() throw();
			Exception(const Exception &) throw();
			virtual ~Exception() throw();
			Exception & operator =(const Exception &) throw();
			virtual ::TunnelEx::AutoPtr<::TunnelEx::LocalException> Clone() const;
		private:
			static std::wstring GetLastError();
		};

		typedef ACE_SSL_SOCK_Stream Base;

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;

	public:

		typedef std::vector<char> Buffer;

	protected:

		struct BufferSet {
			const MessageBlock *encryptedMessage;
			size_t encryptedMessageReadPos;
			Buffer decryptionSub;
			Buffer decryptionFull;
			Buffer out;
		};

	public:

		SslSockStream(const ACE_SSL_Context &context);
		~SslSockStream() throw();

	public:

		Mutex & GetMutex() {
			return m_mutex;
		}

	public:

		/** @throw Error
		  */
		void Connect();
		/** @throw Error
		  */
		void Connect(MessageBlock &);
		/** @throw Error
		  */
		void Accept();
		/** @throw Error
		  */
		void Accept(MessageBlock &);

		bool IsConnected() const {
			return SSL_is_init_finished(ssl());
		}

		void SwitchToDecryptorEncryptorMode();
		void SwitchToStreamMode() throw();

		bool IsDecryptorEncryptorMode() const throw() {
			return m_isDecryptorEncryptorMode;
		}

		void Decrypt(const MessageBlock &) const;
		void Encrypt(const MessageBlock &) const;

		const Buffer & GetDecrypted() const {
			return m_buffers.decryptionFull;
		}

		const Buffer & GetEncrypted() const {
			return m_buffers.out;
		}
		void ClearEncrypted() {
			m_buffers.out.resize(0);
		}

	public:

		int BioWrite(const char *, size_t, int &);
		int BioRead(char *, size_t, int &);

	private:

		/** @throw Error
		  */
		void ConnectSsl();
		/** @throw Error
		  */
		void AcceptSsl();

	private:

		mutable BufferSet m_buffers;

		bool m_isDecryptorEncryptorMode;
		BIO *m_biorOrig;
		BIO *m_biowOrig;

		Mutex m_mutex;

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__SslSockStream_hpp__1012092215
