 /**************************************************************************
 *   Created: 2010/10/31 20:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Xml.cpp 1129 2011-02-22 17:28:50Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#ifdef TUNNELEX_CORE
#	include "Core/String.hpp"
#else // #ifdef TUNNELEX_CORE
#	include <TunnelEx/String.hpp>
#endif // #ifdef TUNNELEX_CORE

using namespace std;
using namespace boost;
using namespace TunnelEx;
using namespace TunnelEx::Helpers::Xml;

//////////////////////////////////////////////////////////////////////////

void Document::Dump(wstring &buffer) const {
	WString wbuffer;
	Dump(wbuffer);
	buffer = wbuffer.GetCStr();
}

void Document::Dump(UString &buffer) const {
	xmlChar *strPtr;
	int size;
	xmlDocDumpMemory(m_handler->Get(), &strPtr, &size);
	shared_ptr<xmlChar> str(strPtr, &Free);
	buffer = strPtr;
}

void Document::Dump(WString &buffer) const {
	xmlChar *strPtr;
	int size;
	xmlDocDumpMemory(m_handler->Get(), &strPtr, &size);
	shared_ptr<xmlChar> str(strPtr, &Free);
	ConvertString(strPtr, buffer);
}

shared_ptr<Document> Document::LoadFromFile(const string &xmlFile) {
	xmlDocPtr docPtr = xmlReadFile(xmlFile.c_str(), NULL, 0);
	if (docPtr == NULL) {
		TunnelEx::Format exceptionWhat(
			"Failed to parse XML-file \"%1%\","
				" file has an invalid format or does not exist.");
		exceptionWhat % xmlFile;
		throw ParseException(exceptionWhat.str().c_str());
	}
	boost::shared_ptr<Handler> handler(new Handler(docPtr));
	return boost::shared_ptr<Document>(new Document(handler));
}

shared_ptr<Document> Document::LoadFromFile(const wstring &xmlFile) {
	return LoadFromFile(ConvertString<String>(xmlFile.c_str()).GetCStr());
}

bool Document::Save(const string &fileName) const {
	shared_ptr<xmlSaveCtxt> saveCtxt(
		xmlSaveToFilename(fileName.c_str(), "utf-8", XML_SAVE_FORMAT),
		&xmlSaveClose);
	return saveCtxt.get() && xmlSaveDoc(saveCtxt.get(), m_handler->Get()) >= 0;
}

bool Document::Save(const wstring &fileName) const {
	return Save(ConvertString<String>(fileName.c_str()).GetCStr());
}

//////////////////////////////////////////////////////////////////////////

void Node::SetContent(const String &value) {
	SetContent(ConvertString<UString>(value));
}

void Node::SetContent(const UString &value) {
	SetContent(value.GetCStr());
}

void Node::SetContent(const WString &value) {
	SetContent(ConvertString<UString>(value));
}

void Node::SetContent(const char *value) {
	SetContent(String(value));
}

void Node::SetContent(const wchar_t *value) {
	SetContent(WString(value));
}

void Node::SetContent(const string &value) {
	SetContent(String(value.c_str()));
}

void Node::SetContent(const wstring &value) {
	SetContent(WString(value.c_str()));
}

template<>
WString & Node::GetName(WString &destinationBuffer) const {
	UString utf8buffer;
	return ConvertString(GetName(utf8buffer), destinationBuffer);
}

template<>
wstring & Node::GetName(wstring &destinationBuffer) const {
	UString utf8buffer;
	WString wideBuffer;
	destinationBuffer = ConvertString(GetName(utf8buffer), wideBuffer).GetCStr();
	return destinationBuffer;
}

template<>
WString & Node::GetContent(WString &destinationBuffer) const {
	UString utf8buffer;
	return ConvertString(GetContent(utf8buffer), destinationBuffer);
}

template<>
wstring & Node::GetContent(wstring &destinationBuffer) const {
	UString utf8buffer;
	WString wideBuffer;
	destinationBuffer = ConvertString(GetContent(utf8buffer), wideBuffer).GetCStr();
	return destinationBuffer;
}

template<>
WString & Node::GetAttribute(
			const char *attributeName,
			WString &destinationBuffer)
		const {
	UString utf8buffer;
	return ConvertString(GetAttribute(attributeName, utf8buffer), destinationBuffer);
}

template<>
wstring & Node::GetAttribute(
			const char *attributeName,
			wstring &destinationBuffer)
		const {
	UString utf8buffer;
	WString wideBuffer;
	destinationBuffer = ConvertString(
			GetAttribute(attributeName, utf8buffer),
			wideBuffer)
		.GetCStr();
	return destinationBuffer;
}

template<>
void Node::SetAttribute(const char *attributeName, const UString &value) {
	SetAttribute(attributeName, value.GetCStr());
}

template<>
void Node::SetAttribute(const char *attributeName, const wstring &value) {
	UString utf8Value;
	SetAttribute(attributeName, ConvertString(value.c_str(), utf8Value));
}

template<>
void Node::SetAttribute(const char *attributeName, const WString &value) {
	UString utf8Value;
	SetAttribute(attributeName, ConvertString(value, utf8Value));
}

#if TEMPLATES_REQUIRE_SOURCE != 0
	//! Only for template instantiation.
	namespace {
		template<typename T>
		void MakeTemplatePtr(T) {
			//...//
		}
		void MakeTemplateInstantiation() {
			MakeTemplatePtr(&Node::GetName<WString>);
			MakeTemplatePtr(&Node::GetName<wstring>);
			MakeTemplatePtr(&Node::GetContent<WString>);
			MakeTemplatePtr(&Node::GetContent<wstring>);
			MakeTemplatePtr(&Node::GetAttribute<WString>);
			MakeTemplatePtr(&Node::GetAttribute<wstring>);
			MakeTemplatePtr(&Node::SetAttribute<UString>);
			MakeTemplatePtr(&Node::SetAttribute<wstring>);
			MakeTemplatePtr(&Node::SetAttribute<WString>);
		}
	}
#endif // TEMPLATES_REQUIRE_SOURCE
