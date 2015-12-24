#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "MySinkBackend.h"
using namespace Common;

CMySinkBackend::CMySinkBackend(fpLogConsume fp)
{
	m_fp = fp;
}

void CMySinkBackend::add_stream(boost::shared_ptr<std::ostream> const& strm)
{
	sinks::basic_text_ostream_backend< char >::add_stream(strm);
}

void CMySinkBackend::remove_stream(boost::shared_ptr<std::ostream> const& strm)
{
	sinks::basic_text_ostream_backend< char >::remove_stream(strm);
}

void CMySinkBackend::auto_flush(bool f)
{
	sinks::basic_text_ostream_backend< char >::auto_flush(f);
}

void CMySinkBackend::consume(logging::record_view const& rec, string_type const& message)
{
	//sinks::basic_text_ostream_backend< char >::consume(rec, message);
	m_fp(message);
}

void CMySinkBackend::flush()
{
	sinks::basic_text_ostream_backend< char >::flush();
}
