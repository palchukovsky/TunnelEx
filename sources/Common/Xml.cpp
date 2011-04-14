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
#include "Core/String.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Helpers::Xml;

//////////////////////////////////////////////////////////////////////////

void Document::Dump(std::wstring &buffer) const {
	WString wbuffer;
	Dump(wbuffer);
	buffer = wbuffer.GetCStr();
}

void Document::Dump(UString &buffer) const {
	xmlChar *strPtr;
	int size;
	xmlDocDumpMemory(m_handler->Get(), &strPtr, &size);
	boost::shared_ptr<xmlChar> str(strPtr, &Free);
	buffer = strPtr;
}

void Document::Dump(WString &buffer) const {
	xmlChar *strPtr;
	int size;
	xmlDocDumpMemory(m_handler->Get(), &strPtr, &size);
	boost::shared_ptr<xmlChar> str(strPtr, &Free);
	ConvertString(strPtr, buffer);
}

boost::shared_ptr<Document> Document::LoadFromFile(const std::string &xmlFile) {
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

boost::shared_ptr<Document> Document::LoadFromFile(const std::wstring &xmlFile) {
	return LoadFromFile(ConvertString<String>(xmlFile.c_str()).GetCStr());
}

bool Document::Save(const std::string &fileName) const {
	boost::shared_ptr<xmlSaveCtxt> saveCtxt(
		xmlSaveToFilename(fileName.c_str(), "utf-8", XML_SAVE_FORMAT),
		&xmlSaveClose);
	return saveCtxt.get() && xmlSaveDoc(saveCtxt.get(), m_handler->Get()) >= 0;
}

bool Document::Save(const std::wstring &fileName) const {
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

void Node::SetContent(const std::string &value) {
	SetContent(String(value.c_str()));
}

void Node::SetContent(const std::wstring &value) {
	SetContent(WString(value.c_str()));
}

template<>
WString & Node::GetName(WString &destinationBuffer) const {
	UString utf8buffer;
	return ConvertString(GetName(utf8buffer), destinationBuffer);
}

template<>
std::wstring & Node::GetName(std::wstring &destinationBuffer) const {
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
std::wstring & Node::GetContent(std::wstring &destinationBuffer) const {
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
std::wstring & Node::GetAttribute(
			const char *attributeName,
			std::wstring &destinationBuffer)
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
void Node::SetAttribute(const char *attributeName, const std::wstring &value) {
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
			MakeTemplatePtr(&Node::GetName<std::wstring>);
			MakeTemplatePtr(&Node::GetContent<WString>);
			MakeTemplatePtr(&Node::GetContent<std::wstring>);
			MakeTemplatePtr(&Node::GetAttribute<WString>);
			MakeTemplatePtr(&Node::GetAttribute<std::wstring>);
			MakeTemplatePtr(&Node::SetAttribute<UString>);
			MakeTemplatePtr(&Node::SetAttribute<std::wstring>);
			MakeTemplatePtr(&Node::SetAttribute<WString>);
		}
	}
#endif // TEMPLATES_REQUIRE_SOURCE
