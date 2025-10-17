#include "stdafx.h"
#include "CommandMgr.h"
#include "App.h"
#include "imgui.h"
#include "TopicRW.h"
#include "SharedData.h"
#include "NetworkDefs.h"

namespace DdsPerfTest
{
	CommandMgr::CommandMgr(App* app)
	: _app(app)
	{
		int participant = app->GetParticipant(0);
		_commandRW = std::make_shared<TopicRW>(participant, "Command", &Net_Command_desc, DDS_RELIABILITY_RELIABLE, DDS_DURABILITY_VOLATILE, DDS_HISTORY_KEEP_ALL, 1);
	}

	void CommandMgr::Tick()
	{
		ReadCommands();
	}

	void CommandMgr::SendCommand(const Command& cmd)
	{
		Net_Command sample = {};
		sample.Type = (char*)cmd.Type.c_str();
		sample.Data = (char*)cmd.Data.c_str();
		dds_write(_commandRW->GetWriter(), &sample);
	}

	void CommandMgr::ReadCommands()
	{
		const int MAX_SAMPLES = 10;
		Net_Command* samples[MAX_SAMPLES] = {}; // we want DDS to allocate memory for us (we do not need to care about freeing it)
		dds_sample_info_t infos[MAX_SAMPLES];

		int num = dds_take(_commandRW->GetReader(), (void**)samples, infos, MAX_SAMPLES, MAX_SAMPLES);
		if (num == DDS_RETCODE_NO_DATA)
			return;
		if (num < 0)
			DDS_FATAL("dds_read(Command): %s\n", dds_strretcode(-num));
		for (int i = 0; i < num; i++)
		{
			Net_Command* sample = (Net_Command*)samples[i];

			Command cmd;
			cmd.Type = sample->Type;
			cmd.Data = sample->Data;

			OnCommand(cmd);

		}
	}


}