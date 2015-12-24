#include <boost/shared_ptr.hpp>
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
#include "Logger.h"

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

namespace Common
{
	class CMySinkBackend : public sinks::basic_text_ostream_backend < char >
	{
	public:
		explicit CMySinkBackend(fpLogConsume fp);
		void add_stream(boost::shared_ptr<std::ostream> const& strm);
		void remove_stream(boost::shared_ptr<std::ostream> const& strm);
		void auto_flush(bool f = true);
		void consume(logging::record_view const& rec, string_type const& formatted_message);
		void flush();
	private:
		fpLogConsume m_fp;
	};
}