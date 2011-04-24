/**************************************************************************
 *   Created: 2007/12/27 23:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__MultipleStream_h__0712272322
#define INCLUDED_FILE__TUNNELEX__MultipleStream_h__0712272322

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <fstream>
#include <set>
#include <map>
#include <vector>

namespace TunnelEx { namespace Helpers {

	template<typename C, typename T, size_t BufferSize = 0x100>
	class MultipleStreamBuffer
		: public std::basic_streambuf<typename C, typename T> {

	private:

		typedef std::basic_streambuf<C, T> buffer_type;
		typedef typename buffer_type::int_type int_type;
		typedef std::basic_ios<C, T> basic_ios_type;

	public:

		MultipleStreamBuffer()
		: basic_streambuf()
		, m_buffer(BufferSize)
		, m_buffers() {
			setp(&m_buffer[0], &m_buffer[0] + BufferSize);
		}

		~MultipleStreamBuffer() {
			pubsync();
		}

	public:

		bool attach(buffer_type* b) {
			if (m_buffers.find(b) != m_buffers.end()) {
				return true;
			}
			return m_buffers.insert(b).second;
		}

		bool attach(basic_ios_type const& s) {
			return attach(s.rdbuf());
		}

		bool detach(buffer_type* b) {
			return m_buffers.erase(b) > 0;
		}

		bool detach(basic_ios_type const& s) {
			return detach(s.rdbuf());
		}

	private:

		int sync() {
			//! \todo: see the TEX-71, maybe you can implement it better
			if (traits_type::eq_int_type(traits_type::eof(), overflow())) {
				std::for_each(
					m_buffers.begin(), m_buffers.end(),
					boost::bind(&buffer_type::pubsync, _1));
				return 0;
			}
			return -1;
		}

		int_type overflow(int_type c = traits_type::eof()) {
#			pragma warning(push)
#			pragma warning(disable: 4244)
			//! \todo: potential error on 64-bit systems
			if (const std::streamsize chars = pptr() - pbase()) {
				std::for_each(
					m_buffers.begin(), m_buffers.end(),
					boost::bind(&buffer_type::sputn, _1, pbase(), chars));
				setp(pbase(), epptr());
			}
#			pragma warning(pop)
			if (!traits_type::eq_int_type(c, traits_type::eof())) {
				const char_type e(traits_type::to_char_type(c));
				std::for_each(
					m_buffers.begin(), m_buffers.end(),
					boost::bind(&buffer_type::sputc, _1, e));
			}
			return c;
		}

	private:

		std::vector<C> m_buffer;
		std::set<buffer_type*> m_buffers;

	};

	//////////////////////////////////////////////////////////////////////////

	template<typename Elem>
	class MultipleStream
		: public std::basic_ostream<typename Elem, std::char_traits<typename Elem> > {

	public:

		typedef MultipleStream<typename Elem> Me;
		typedef std::basic_ostream<typename Elem, std::char_traits<typename Elem> > Base;
		typedef typename Base::char_type char_type;

	public:
		
		MultipleStream()
				: Base(&m_buffer) {
			//...//
		}
		
		~MultipleStream() throw() {
			try {
				const Files::const_iterator end = m_files.end();
				for (Files::iterator i = m_files.begin(); i != end; ++i) {
					DetachStream(*i->second);
				}
			} catch (...) {
				assert(false);
			}
		}

	public:

		bool AttachStream(const Base &stream) {
			return m_buffer.attach(stream);
		}
		
		bool DetachStream(const Base &stream) {
			return m_buffer.detach(stream);
		}
		
		bool AttachFile(const std::basic_string<char_type> & filePath) {
			if (m_files.find(filePath) != m_files.end()) {
				return true;
			}
			typedef std::basic_ofstream<
					char_type,
					std::char_traits<char_type> >
				FileStream;
			boost::shared_ptr<Base> f(
				new FileStream(
					filePath.c_str(),
					std::ios::out | std::ios::app | std::ios::ate));
			if (*f && AttachStream(*f)) {
				m_files[filePath] = f;
				return true;
			} else {
				return false;
			}
		}
		
		bool DetachFile(const std::basic_string<char_type>& filePath) {
			const Files::iterator pos = m_files.find(filePath);
			if (pos == m_files.end()) {
				return false;
			}
			DetachStream(*pos->second);
			m_files.erase(pos);
			return true;
		}

	private:

		typedef std::map<std::basic_string<char_type>, boost::shared_ptr<Base> > Files;

		Files m_files;
		MultipleStreamBuffer<char_type, std::char_traits<char_type> > m_buffer;

	};

	//////////////////////////////////////////////////////////////////////////

} }

#endif // INCLUDED_FILE__TUNNELEX__MultipleStream_h__0712272322