#include "Logger.h"
#include "MySinkBackend.h"

// http://www.boost.org/doc/libs/1_54_0/libs/log/doc/html/log/design.html
#include <boost/log/trivial.hpp>
#include <boost/log/common.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes/current_process_id.hpp>
#include <boost/log/attributes/current_process_name.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/utility/record_ordering.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_value.hpp>
#include <boost/log/attributes/attribute_value_impl.hpp>
#include <boost/core/null_deleter.hpp>
#include <Windows.h>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;
using namespace Common;

#define LOG(logger, severity) \
    BOOST_LOG_SEV(logger, severity) << "(" << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << ") "

class MyCurrentThreadIdImpl 
	: public logging::attribute::impl
{
public:
	logging::attribute_value get_value()
	{
		int tid = GetCurrentThreadId();
		return attrs::make_attribute_value(tid);
	}
};

class MyCurrentThreadId :
	public logging::attribute
{
public:
	MyCurrentThreadId() 
		: logging::attribute(new MyCurrentThreadIdImpl())
	{
	}
	explicit MyCurrentThreadId(attrs::cast_source const& source) 
		: logging::attribute(source.as< MyCurrentThreadIdImpl >())
	{
	}
};

class MyCurrentProcessIdImpl
	: public logging::attribute::impl
{
public:
	logging::attribute_value get_value()
	{
		int tid = GetCurrentProcessId();
		return attrs::make_attribute_value(tid);
	}
};

class MyCurrentProcessId :
	public logging::attribute
{
public:
	MyCurrentProcessId()
		: logging::attribute(new MyCurrentProcessIdImpl())
	{
	}
	explicit MyCurrentProcessId(attrs::cast_source const& source)
		: logging::attribute(source.as< MyCurrentProcessIdImpl >())
	{
	}
};

void CLogger::Init()
{
	global_attribute();
	add_console_sink();
	add_vs_debug_output_sink();
	add_text_file_sink();
}

void CLogger::Release()
{
	logging::core::get()->flush();
	logging::core::get()->remove_all_sinks();
}

int CLogger::_level = 0;
void CLogger::SetLevel(int level)
{
	enum severity_level
	{
		trace,
		debug,
		info,
		warning,
		error,
		fatal
	};

	_level = level;
	logging::core::get()->set_filter(boost::log::trivial::severity >= level);
}

int CLogger::GetLevel()
{
	return _level;
}

void CLogger::global_attribute()
{
	logging::core::get()->add_global_attribute("Process", attrs::current_process_name());
	logging::core::get()->add_global_attribute("ProcessID", attrs::current_process_id());
	logging::core::get()->add_global_attribute("MyThreadID", MyCurrentThreadId());
	logging::core::get()->add_global_attribute("MyProcessID", MyCurrentProcessId());
	logging::core::get()->add_global_attribute("TimeStamp", attrs::local_clock());
	logging::core::get()->add_global_attribute("RecordID", attrs::counter<unsigned int>());
}

void CLogger::add_console_sink()
{
	boost::shared_ptr<sinks::text_ostream_backend> backend = boost::make_shared<sinks::text_ostream_backend>();
	backend->add_stream(boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter()));
	backend->auto_flush(true);

	typedef sinks::synchronous_sink<sinks::text_ostream_backend> sink_t;
	boost::shared_ptr<sink_t> sink(new sink_t(backend));

	sink->set_formatter(
		expr::format("[%1% %2%][%3%][%4%] %5%")
		% expr::attr<int>("MyProcessID")
		% expr::attr<int>("MyThreadID")
		% expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S")
		% logging::trivial::severity
		% expr::message
		);
	logging::core::get()->add_sink(sink);
}

void CLogger::add_stream_sink(fpLogConsume fp)
{
	boost::shared_ptr<CMySinkBackend> backend = boost::make_shared<CMySinkBackend>(fp);
	backend->auto_flush(true);

	typedef sinks::synchronous_sink<CMySinkBackend> sink_t;
	boost::shared_ptr<sink_t> sink(new sink_t(backend));

	sink->set_formatter(
		expr::format("{\"Time\":\"%1%\",\"Level\":\"%2%\",\"Message\":\"%3%\"}")
		% expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S")
		% logging::trivial::severity
		% expr::message
		);
	logging::core::get()->add_sink(sink);
}

void CLogger::add_vs_debug_output_sink()
{
	boost::shared_ptr<sinks::debug_output_backend> backend = boost::make_shared<sinks::debug_output_backend>();

	typedef sinks::synchronous_sink<sinks::debug_output_backend> sink_t;
	boost::shared_ptr<sink_t> sink(new sink_t(backend));

	sink->set_formatter(
		expr::format("[%1% %2% %3%][%4%][%5%] %6%")
		% expr::attr<attrs::current_process_name::value_type>("Process")
		% expr::attr<int>("MyProcessID")
		% expr::attr<int>("MyThreadID")
		% expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S")
		% logging::trivial::severity
		% expr::message
		);

	logging::core::get()->add_sink(sink);
}

void CLogger::add_text_file_sink()
{
	boost::shared_ptr< sinks::text_file_backend > file_backend = boost::make_shared< sinks::text_file_backend >(
		keywords::open_mode = (std::ios::out | std::ios::app),
		//keywords::file_name = "log_%Y%m%d_%H%M%S_%N.txt",
		keywords::file_name = "TCPLOG_%Y%m%d.txt",
		keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
		keywords::format = "[%TimeStamp%] (%Severity%) : %Message%"
		);

	file_backend->auto_flush(true);

	typedef sinks::asynchronous_sink<
		sinks::text_file_backend,
		sinks::unbounded_ordering_queue<
		logging::attribute_value_ordering<
		unsigned int,
		std::less< unsigned int >
		>
		>
	> sink_t;

	boost::shared_ptr< sink_t > sink(new sink_t(
		file_backend,
		keywords::order = logging::make_attr_ordering("RecordID", std::less< unsigned int >()),
		keywords::ordering_window = boost::posix_time::seconds(1)
		));
	
	sink->set_formatter(
		expr::format("[%1% %2% %3%][%4%][%5%] %6%")
		% expr::attr<attrs::current_process_name::value_type>("Process")
		% expr::attr<int>("MyProcessID")
		% expr::attr<int>("MyThreadID")
		% expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S")
		% logging::trivial::severity
		% expr::message
		);

	sink->locked_backend()->set_file_collector(sinks::file::make_collector(keywords::target = "logs"));

	logging::core::get()->add_sink(sink);
}

void CLogger::add_text_file_sink_unorder()
{
	boost::shared_ptr< sinks::text_file_backend > file_backend = boost::make_shared< sinks::text_file_backend >(
		keywords::open_mode = (std::ios::out | std::ios::app),
		//keywords::file_name = "log_%Y%m%d_%H%M%S_%N.txt",
		keywords::file_name = "TCPLOG_%Y%m%d.txt",
		keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
		keywords::format = "[%TimeStamp%] (%Severity%) : %Message%"		
		);

	file_backend->auto_flush(true);

	typedef sinks::asynchronous_sink< sinks::text_file_backend > sink_t;
	boost::shared_ptr< sink_t > sink(new sink_t(file_backend));

	sink->set_formatter(
		expr::format("[%1% %2% %3%][%4%][%5%] %6%")
		% expr::attr<attrs::current_process_name::value_type>("Process")
		% expr::attr<int>("MyProcessID")
		% expr::attr<int>("MyThreadID")
		% expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S")
		% logging::trivial::severity
		% expr::message
		);

	sink->locked_backend()->set_file_collector(sinks::file::make_collector(keywords::target = "logs"));

	logging::core::get()->add_sink(sink);
}