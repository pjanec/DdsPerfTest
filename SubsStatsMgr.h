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

	class SubsStatsMgr
	{
	public:
		SubsStatsMgr(App* app);
		void Tick();

		void DrawUI();
		int GetSubsStatsWriter() const;

	protected:
		void DrawSubsStats();
		void ReadSubsStats();


	protected:
		App* _app;
		std::shared_ptr<TopicRW> _subsStatRW;
		std::map<PubSubKey, SubsStats> _subsStats;

	};
}