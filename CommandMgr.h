#pragma once
#include "dds/dds.h"
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include "SharedData.h"



namespace DdsPerfTest
{
	class App;
	class TopicRW;

	class CommandMgr
	{
	public:
		CommandMgr(App* app, int participant);
		void Tick();
		std::function<void(const Command&)> OnCommand;
		void SendCommand(const Command& cmd);


	protected:
		void ReadCommands();

	protected:
		App* _app;
		std::shared_ptr<TopicRW> _commandRW;

	};
}