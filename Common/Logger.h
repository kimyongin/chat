#pragma once
#include <string>
#include <functional>

namespace Common
{
	typedef std::function<void(const std::string& message)> fpLogConsume;
	class CLogger
	{
	public:
		static void Init();
		static void Release();
		static void SetLevel(int level);
		static int GetLevel();
		static void add_stream_sink(fpLogConsume fp);
	private:
		static void global_attribute();
		static void add_console_sink();		
		static void add_vs_debug_output_sink();
		static void add_text_file_sink();
		static void add_text_file_sink_unorder();
	private:
		static int _level;
	};
}