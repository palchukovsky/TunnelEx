 /**************************************************************************
 *   Created: 2007/10/29 13:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__XmlDocument_h__0710291338
#define INCLUDED_FILE__TUNNELEX__XmlDocument_h__0710291338

#include "Format.hpp"
#include "StringFwd.hpp"

#include <libxml/tree.h>
#include <libxml/xmlschemas.h>
#include <libxml/xpath.h>
#include <libxml/xmlsave.h>
#include "CompileWarningsBoost.h"
#	include <boost/noncopyable.hpp>
#	include <boost/shared_ptr.hpp>
#	include <boost/algorithm/string.hpp>
#include "CompileWarningsBoost.h"
#include <string>
#include <vector>

#ifdef _DEBUG
#	pragma comment(lib, "libxml2_dbg.lib")
#else // _DEBUG
#	pragma comment(lib, "libxml2.lib")
#endif // _DEBUG

namespace TunnelEx {
namespace Helpers {
namespace Xml {

	class Exception : public std::exception {
	public:
		explicit Exception(const char *what)
				: std::exception(what) {
			//...//
		}
	};

	inline void Free(void *mem) {
		xmlFree(mem);
	}

	struct DebugData {
		static bool & GetErrorDisablingResetFlag() {
			static bool isReseted = false;
			return isReseted;
		}
		static bool CheckErrorDisablingResetFlag() {
			bool &isReseted = GetErrorDisablingResetFlag(); 
			const bool result = isReseted;
			isReseted = true;
			return result;
		}
	};
	
	inline void SetErrorsHandler(void(*func)(void *ctx, const char *msg, ...)) {
		BOOST_ASSERT(func);
		BOOST_ASSERT(!DebugData::CheckErrorDisablingResetFlag());
		initGenericErrorDefaultFunc(&func);
	}

	class Node;

	//! XML document implementation.
	class Document {

		friend class Schema;
		friend class XPath;

	public:

		class Handler {
		public:
			explicit Handler(xmlDocPtr doc)
					: m_doc(doc) {
				BOOST_ASSERT(m_doc != NULL);
			}
			Handler(const Handler &rhs)
					: m_doc(xmlCopyDoc(rhs.m_doc, true)) {
				BOOST_ASSERT(m_doc != NULL);
			}
			~Handler() {
				xmlFreeDoc(m_doc);
			}
			xmlDocPtr Get() {
				return m_doc;
			}
		private:
			const Handler & operator =(const Handler &);
		private:
			xmlDocPtr m_doc;
		};

		class ParseException : public Exception {
		public:
			explicit ParseException(const char *what)
					: Exception(what) {
				//...//
			}
		};

	protected:

		//! Protected, use Document::CreateNew, Document::LoadFromMemory and Document::LoadFromFile.
		explicit Document(boost::shared_ptr<Handler> handle)
				: m_handler(handle) {
			// BOOST_ASSERT(DebugData::GetErrorDisablingResetFlag());
		}

	public:

		explicit Document(const Document &rhs)
				: m_handler(rhs.m_handler) {
			// BOOST_ASSERT(DebugData::GetErrorDisablingResetFlag());
		}

		const Document & operator =(const Document &rhs) {
			Document(rhs).Swap(*this);
		}

		void Swap(Document &rhs) throw () {
			boost::shared_ptr<Handler> tmpHandler(m_handler);
			m_handler = rhs.m_handler;
			rhs.m_handler = tmpHandler;
		}

		//! Creates new empty document with root element.
		static boost::shared_ptr<Document> CreateNew(const char *rootElemenName) {
			boost::shared_ptr<Handler> handler(
				new Handler(xmlNewDoc(reinterpret_cast<xmlChar *>("1.0"))));
			xmlDocSetRootElement(
				handler->Get(),
				xmlNewNode(NULL, reinterpret_cast<const xmlChar *>(rootElemenName)));
			return boost::shared_ptr<Document>(new Document(handler));
		}

		//! Creates new document, duplicate.
		static boost::shared_ptr<Document> CreateDuplicate(Document &doc) {
			boost::shared_ptr<Handler> handler(
				new Handler(xmlCopyDoc(doc.m_handler->Get(), 1)));
			return boost::shared_ptr<Document>(new Document(handler));
		}

		template<class XmlString>
		static boost::shared_ptr<Document> LoadFromString(const XmlString &xmlString) {
#			pragma warning(push)
#			pragma warning(disable: 4244)
			xmlDocPtr docPtr = xmlReadMemory(
				reinterpret_cast<const char *>(xmlString.GetCStr()),
				xmlString.GetLength() * sizeof(XmlString::value_type),
				NULL,
				NULL,
				0);
#			pragma warning(pop)
			if (docPtr == NULL) {
				throw ParseException(
					"Could not parse XML-string, "
					"string has an invalid format or empty.");
			}
			boost::shared_ptr<Handler> handler(new Handler(docPtr));
			return boost::shared_ptr<Document>(new Document(handler));
		}

		template<>
		static boost::shared_ptr<Document> LoadFromString<std::string>(
					const std::string &xmlString) {
			xmlDocPtr docPtr = xmlReadMemory(
				reinterpret_cast<const char *>(xmlString.c_str()),
				int(xmlString.size() * sizeof(std::string::value_type)),
				NULL,
				NULL,
				0);
			if (docPtr == NULL) {
				throw ParseException(
					"Could not parse XML-string, "
						"string has an invalid format or empty.");
			}
			boost::shared_ptr<Handler> handler(new Handler(docPtr));
			return boost::shared_ptr<Document>(new Document(handler));
		}

		static boost::shared_ptr<Document> LoadFromFile(const std::string &);
		static boost::shared_ptr<Document> LoadFromFile(const std::wstring &);

		void Dump(std::wstring &buffer) const;

		void Dump(TunnelEx::UString &) const;

		void Dump(TunnelEx::WString &buffer) const;

		bool Save(const std::string &fileName) const;
		bool Save(const std::wstring &fileName) const;
		
		boost::shared_ptr<Node> GetRoot();
		boost::shared_ptr<const Node> GetRoot() const {
			return (const_cast<Document *>(this))->GetRoot();
		}
		void SetRoot(boost::shared_ptr<Node>);

		boost::shared_ptr<XPath> GetXPath();
		boost::shared_ptr<const XPath> GetXPath() const {
			return (const_cast<Document *>(this))->GetXPath();
		}

	private:

		boost::shared_ptr<Handler> m_handler;

	};

	//////////////////////////////////////////////////////////////////////////

	typedef std::vector<boost::shared_ptr<Node> > NodeCollection;
	typedef std::vector<boost::shared_ptr<const Node> > ConstNodeCollection;
	
	//! XML-node implementation.
	class Node : private boost::noncopyable {

		friend class Schema;
		friend class XPath;

	public:

		explicit Node(
				boost::shared_ptr<Document::Handler> doc,
				xmlNodePtr node)
				: m_node(node),
				m_doc(doc) {
			//...//
		}

		~Node() throw() {
			//...//
		}

	public:

		boost::shared_ptr<XPath> Node::GetXPath();
		boost::shared_ptr<const XPath> GetXPath() const {
			return (const_cast<Node *>(this))->GetXPath();
		}

		template<class String>
		String & GetName(String &destinationBuffer) const {
			destinationBuffer
				= reinterpret_cast<const String::value_type *>(m_node->name);
			return destinationBuffer;
		}

		template<>
		TunnelEx::WString & GetName(WString &destinationBuffer) const;

		template<>
		std::wstring & GetName(std::wstring &destinationBuffer) const;

		template<class String>
		String & GetContent(String &destinationBuffer) const {
			xmlChar *contentPtr = xmlNodeGetContent(m_node);
			boost::shared_ptr<xmlChar> content(contentPtr, &Free);
			destinationBuffer
				= reinterpret_cast<String::value_type *>(contentPtr);
			return destinationBuffer;
		}

		template<>
		TunnelEx::WString & GetContent(TunnelEx::WString &destinationBuffer) const;

		template<>
		std::wstring & GetContent(std::wstring &destinationBuffer) const;

		void SetContent(const xmlChar *const value) {
			xmlChar *const encodedValuePtr = xmlEncodeEntitiesReentrant(
				m_doc->Get(),
				value);
			BOOST_ASSERT(encodedValuePtr != 0);
			boost::shared_ptr<xmlChar> encodedValue(encodedValuePtr, &Free);
			xmlNodeSetContent(m_node, encodedValuePtr);
		}

		void SetContent(const TunnelEx::String &);

		void SetContent(const TunnelEx::UString &);

		void SetContent(const TunnelEx::WString &);

		void SetContent(const char *);

		void SetContent(const wchar_t *);

		void SetContent(const std::string &);

		void SetContent(const std::wstring &);

		bool HasAttribute(const char *attributeName) const {
			return xmlHasProp(
				m_node, reinterpret_cast<const xmlChar *>(attributeName)) != NULL;
		}

		template<class String>
		String & GetAttribute(
					const char *attributeName,
					String &destinationBuffer)
				const {
			xmlChar *attributeValPtr = xmlGetProp(
				m_node, reinterpret_cast<const xmlChar *>(attributeName));
			if (attributeValPtr == NULL) {
				throw Exception("Unknown XML-attribute.");
			}
			boost::shared_ptr<xmlChar> attributeVal(attributeValPtr, &Free);
			destinationBuffer
				= reinterpret_cast<String::value_type *>(attributeValPtr);
			return destinationBuffer;
		}

		template<>
		TunnelEx::WString & GetAttribute(
				const char *attributeName,
				TunnelEx::WString &destinationBuffer)
			const;

		template<>
		std::wstring & GetAttribute(
				const char *attributeName,
				std::wstring &destinationBuffer)
			const;

		template<typename T>
		void SetAttribute(const char *attributeName, const T &value) {
			xmlSetProp(
				m_node,
				reinterpret_cast<const xmlChar *>(attributeName),
				reinterpret_cast<const xmlChar *>(value));
		}

		template<>
		void SetAttribute(const char *attributeName, const TunnelEx::UString &value);

		template<>
		void SetAttribute(const char *attributeName, const std::wstring &value);

		template<>
		void SetAttribute(const char *attributeName, const TunnelEx::WString &value);

		template<>
		void SetAttribute(const char *attributeName, const std::string &value) {
			SetAttribute(attributeName, value.c_str());
		}

		//! Returns first child element element or empty pointer with NULL
		//! if not exists.
		boost::shared_ptr<const Node> GetChildElement() const {
			return (const_cast<Node *>(this))->GetChildElement();
		}

		//! Returns first child element element or empty pointer with NULL
		//! if not exists.
		boost::shared_ptr<Node> GetChildElement() {
			for (xmlNodePtr i = m_node->children; i; i = i->next) {
				if (i->type == XML_ELEMENT_NODE) {
					return boost::shared_ptr<Node>(new Node(m_doc, i));
				}
			}
			return boost::shared_ptr<Node>();
		}

		//! Returns next element or empty pointer with NULL if not exists.
		boost::shared_ptr<Node> GetNextElement() {
			for (xmlNodePtr i = m_node->next; i; i = i->next) {
				if (i->type == XML_ELEMENT_NODE) {
					return boost::shared_ptr<Node>(new Node(m_doc, i));
				}
			}
			return boost::shared_ptr<Node>();
		}

		//! Returns next element or empty pointer with NULL if not exists.
		boost::shared_ptr<const Node> GetNextElement() const {
			return (const_cast<Node *>(this))->GetNextElement();
		}

		boost::shared_ptr<Node> GetParent() {
			if (!m_node->parent) {
				return boost::shared_ptr<Node>();
			}
			BOOST_ASSERT(m_node->parent->type == XML_ELEMENT_NODE);
			return boost::shared_ptr<Node>(new Node(m_doc, m_node->parent));
		}

		boost::shared_ptr<const Node> GetParent() const {
			return (const_cast<Node *>(this))->GetParent();
		}

		boost::shared_ptr<Node> CreateNewChild(const char *name) {
			return boost::shared_ptr<Node>(new Node(
				m_doc,
				xmlNewChild(
					m_node,
					NULL,
					reinterpret_cast<const xmlChar *>(name),
					NULL)));
		}

	private:

		mutable xmlNodePtr m_node;
		boost::shared_ptr<Document::Handler> m_doc;

	};

	//////////////////////////////////////////////////////////////////////////

	void DumpSchemaValidityError(void *, const char *, ...);
	void DumpSchemaParseError(void *, const char *, ...);

	//! XML-Schema implementation.
	class Schema : private boost::noncopyable {

	public:

		class ParseException : public Document::ParseException {
		public:
			explicit ParseException(const char *what)
					: Document::ParseException(what) {
				//...//
			}
		};
	
	public:
	
		Schema(const std::string &schemaFile) {
			try {
				m_schemaDoc = Document::LoadFromFile(schemaFile);
			} catch (const Document::ParseException &ex) {
				TunnelEx::Format exceptionWhat(
					"Could not load schema document: \"%1%\".");
				exceptionWhat % ex.what();
				throw ParseException(exceptionWhat.str().c_str());
			}
			m_parserContext = xmlSchemaNewDocParserCtxt(
				m_schemaDoc->m_handler->Get());
			BOOST_ASSERT(m_parserContext);
			xmlSchemaSetParserErrors(
				m_parserContext,
				&DumpSchemaParseError,
				&DumpSchemaParseError,
				NULL);
			m_schema = xmlSchemaParse(m_parserContext);
			BOOST_ASSERT(m_schema);
			m_validateContext = xmlSchemaNewValidCtxt(m_schema);
			BOOST_ASSERT(m_validateContext);
		}

		~Schema() {
			xmlSchemaFreeValidCtxt(m_validateContext);
			xmlSchemaFree(m_schema);
			xmlSchemaFreeParserCtxt(m_parserContext);
		}

	public:

		bool Validate(
					Document &doc,
					std::string *validateError = NULL)
				const {
			SetValidateErrorHandler(validateError);
			return xmlSchemaValidateDoc(m_validateContext, doc.m_handler->Get()) == 0;
		}
		
		bool Validate(Node &node, std::string *validateError = NULL) const {
			SetValidateErrorHandler(validateError);
			return xmlSchemaValidateOneElement(m_validateContext, node.m_node) == 0;
		}

	protected:

		void SetValidateErrorHandler(std::string *validateError = NULL) const {
			if (validateError) {
				validateError->clear();
			}
			xmlSchemaSetValidErrors(
				m_validateContext,
				&DumpSchemaValidityError,
				&DumpSchemaValidityError,
				validateError);
		}

	private:

		//! \todo: try to implement with xmlSchemaNewParserCtxt(URL)
		boost::shared_ptr<Document> m_schemaDoc;
		xmlSchemaParserCtxtPtr m_parserContext;
		xmlSchemaPtr m_schema;
		xmlSchemaValidCtxtPtr m_validateContext;

	};

	//! \todo: move to cpp-file when it will be a lib-project
	inline void DumpSchemaValidityError(void *ctx, const char *msg, ...) {
#		ifndef _DEBUG
			if (!ctx) {
				return;
			}
#		endif // _DEBUG
		va_list vl;
		int i;
		va_start(vl, msg);
		union Printable_t {
			int i;
			double f;
			char c;
			char *s;
		} Printable;
		try {
			TunnelEx::Format messagef(msg);
			for (i = 1; msg[i] != '\0'; ++i) {
				switch (msg[i]) {
					case 'i':
						Printable.i = va_arg(vl, int);
						messagef % Printable.i;
						break;
					case 'f':
						Printable.f = va_arg(vl, double);
						messagef % Printable.f;
						break;
					case 'c':
						Printable.c = va_arg(vl, char);
						messagef % Printable.c;
						break;
					case 's':
						Printable.s = va_arg(vl, char*);
						messagef % Printable.s;
						break;
					default:
						break;
				}
			}
			std::string message = messagef.str();
			boost::trim(message);
#			ifndef _DEBUG
				*(reinterpret_cast<std::string*>(ctx)) = message;
#			else // _DEBUG
				if (ctx) {
					*(reinterpret_cast<std::string *>(ctx)) = message;
				}
#			endif // _DEBUG
		} catch (...) {
			BOOST_ASSERT(false);
		}
		va_end(vl);
	}

	//! \todo: move to cpp-file when it will be a lib-project
	inline void DumpSchemaParseError(void *, const char *msg, ...) {
		va_list vl;
		int i;
		va_start(vl, msg);
		union Printable_t {
			int i;
			double f;
			char c;
			char *s;
		} Printable;
		std::string message;
		try {
			TunnelEx::Format messagef(msg);
			for (i = 1; msg[i] != '\0'; ++i) {
				switch (msg[i]) {
					case 'i':
						Printable.i = va_arg(vl, int);
						messagef % Printable.i;
						break;
					case 'f':
						Printable.f = va_arg(vl, double);
						messagef % Printable.f;
						break;
					case 'c':
						Printable.c = va_arg(vl, char);
						messagef % Printable.c;
						break;
					case 's':
						Printable.s = va_arg(vl, char*);
						messagef % Printable.s;
						break;
					default:
						break;
				}
			}
			message = messagef.str();
			boost::trim(message);
		} catch (...) {
			BOOST_ASSERT(false);
		}
		va_end(vl);
		throw Schema::ParseException(message.c_str());
	}

	//////////////////////////////////////////////////////////////////////////

	//! XPath implementation.
	class XPath : private boost::noncopyable {
	
	public:
		
		explicit XPath(boost::shared_ptr<Document::Handler> docHandler)
				: m_doc(docHandler),
				m_context(xmlXPathNewContext(m_doc->Get())) {
			BOOST_ASSERT(m_context);
		}

		explicit XPath(Node &node)
			: m_doc(node.m_doc),
			m_context(xmlXPathNewContext(m_doc->Get())) {
			BOOST_ASSERT(m_context);
			m_context->node = node.m_node;
		}

		~XPath() {
			xmlXPathFreeContext(m_context);
		}

	public:

		//! Returns first node or nil if nothing found.
		boost::shared_ptr<const Node> Query(const char *xpathExpression) const {
			return (const_cast<XPath *>(this))->Query(xpathExpression);
		}

		//! Returns first node or nil if nothing found.
		boost::shared_ptr<Node> Query(const char *xpathExpression) {
			boost::shared_ptr<Node> result;
			boost::shared_ptr<xmlXPathObject> xpathObj(
				EvalExpression(xpathExpression));
			const xmlNodeSet *const nodeSetVal = xpathObj->nodesetval;
			if (xmlXPathNodeSetIsEmpty(nodeSetVal)) {
				return result;
			}
			for (int i = 0; i < nodeSetVal->nodeNr; ++i) {
				xmlNode *const nodeTab = nodeSetVal->nodeTab[i];
				BOOST_ASSERT(nodeTab);
				if (	nodeTab->type == XML_ELEMENT_NODE
						|| nodeTab->type == XML_ATTRIBUTE_NODE) {
					result.reset(new Node(m_doc, nodeTab));
					break;
				}
			}
			return result;
		}

		void Query(
					const char *xpathExpression,
					ConstNodeCollection &result,
					int nodesLimit = 0)
				const {
			(const_cast<XPath *>(this))
				->QueryCollectionImpl(xpathExpression, result, nodesLimit);
		}

		void Query(
					const char *xpathExpression,
					NodeCollection &result,
					int nodesLimit = 0) {
			QueryCollectionImpl(xpathExpression, result, nodesLimit);
		}

	protected:

		boost::shared_ptr<xmlXPathObject> EvalExpression(
					const char *xpathExpression)
				const {
			xmlXPathObjectPtr xpathObjPtr = xmlXPathEvalExpression(
				reinterpret_cast<const xmlChar *>(xpathExpression), m_context);
			if (xpathObjPtr == NULL) {
				TunnelEx::Format exceptionWhat(
					"Unable to evaluate XPath expression \"%1%\".");
				exceptionWhat % xpathExpression;
				throw Exception(exceptionWhat.str().c_str());
			}
			return boost::shared_ptr<xmlXPathObject>(
				xpathObjPtr, &xmlXPathFreeObject);
		}

		template<class Collection>
		void QueryCollectionImpl(
					const char *xpathExpression,
					Collection &result,
					int nodesLimit = 0) {
			Collection resultTmp;
			boost::shared_ptr<xmlXPathObject> xpathObj(
				EvalExpression(xpathExpression));
			const xmlNodeSet *const nodeSetVal = xpathObj->nodesetval;
			if (xmlXPathNodeSetIsEmpty(nodeSetVal)) {
				result.resize(0);
				return;
			}
			if (!nodesLimit || nodesLimit > nodeSetVal->nodeNr) {
				nodesLimit = nodeSetVal->nodeNr;
			}
			resultTmp.reserve(nodesLimit);
			for (int i = 0; i < nodesLimit; ++i) {
				xmlNode *const nodeTab = nodeSetVal->nodeTab[i];
				BOOST_ASSERT(nodeTab);
				if (	nodeTab->type == XML_ELEMENT_NODE
						|| nodeTab->type == XML_ATTRIBUTE_NODE) {
					resultTmp.push_back(
						Collection::value_type(new Node(m_doc, nodeTab)));
				}
			}
			resultTmp.swap(result);
		}

	private:

		boost::shared_ptr<Document::Handler> m_doc;
		xmlXPathContextPtr m_context;

	};

	//////////////////////////////////////////////////////////////////////////

	//! \todo: move to cpp-file when it will be a lib-project
	inline boost::shared_ptr<Node> Document::GetRoot() {
		xmlNodePtr ptr = xmlDocGetRootElement(m_handler->Get());
		if (ptr == NULL) {
			throw ParseException("XML-document has not root element.");
		}
		return boost::shared_ptr<Node>(new Node(m_handler, ptr));
	}

	inline boost::shared_ptr<XPath> Document::GetXPath() {
		return boost::shared_ptr<XPath>(new XPath(m_handler));
	}

	inline boost::shared_ptr<XPath> Node::GetXPath() {
		return boost::shared_ptr<XPath>(new XPath(*this));
	}

} } }

#endif // INCLUDED_FILE__TUNNELEX__XmlDocument_h__0710291338
