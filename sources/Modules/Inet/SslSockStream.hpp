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
		void SwitchToStreamMode();

		bool IsDecryptorEncryptorMode() const {
			return m_isDecryptorEncryptorMode;
		}

		void Decrypt(MessageBlock &) const;
		void Encrypt(MessageBlock &) const;

		const Buffer & GetEncryptorDecryptorAnswer() const {
			return GetBuffers().out;
		}
		void ResetEncryptorDecryptorAnswer() {
			GetBuffers().out.resize(0);
		}

	public:

		int BioWrite(const char *, size_t, int &);
		int BioRead(char *, size_t, int &);

	protected:

		BufferSet & GetBuffers() const throw() {
			return m_buffers;
		}

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

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__SslSockStream_hpp__1012092215
