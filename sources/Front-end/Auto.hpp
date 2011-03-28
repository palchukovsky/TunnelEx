/**************************************************************************
 *   Created: 2008/02/12 23:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Auto.hpp 994 2010-09-22 15:24:27Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__AutoBusy_h__0802122347
#define INCLUDED_FILE__TUNNELEX__AutoBusy_h__0802122347

class AutoBusy : private boost::noncopyable {

public:

	explicit AutoBusy(wxWindow &window)
			: m_window(window) {
		if (++m_refCount == 1) {
			m_oldCursor.reset(new wxCursor(window.GetCursor()));
			m_window.SetCursor(*wxHOURGLASS_CURSOR);
		}
	}

	~AutoBusy() {
		--m_refCount;
		if (m_oldCursor.get()) {
			BOOST_ASSERT(m_refCount == 0);
			m_window.SetCursor(*m_oldCursor);
		}
	}

private:

	wxWindow &m_window;
	std::auto_ptr<wxCursor> m_oldCursor;
	static size_t m_refCount;

};

#endif // INCLUDED_FILE__TUNNELEX__AutoBusy_h__0802122347
