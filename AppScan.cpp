#include "stdafx.h"
#include "AppScan.h"
#include "App.h"
#include "TopicRW.h"

namespace DdsPerfTest
{
	AppScan::AppScan(App* app)
	: _app(app)
	{
		int participant = _app->GetParticipant(0);
		_appIdRW = std::make_shared<TopicRW>(participant, "AppId", &Net_AppId_desc, DDS_RELIABILITY_RELIABLE, DDS_DURABILITY_TRANSIENT_LOCAL, DDS_HISTORY_KEEP_LAST, 1 );
	}

	AppScan::~AppScan()
	{
		_appIdRW = nullptr;
	}

	void AppScan::Scan()
	{
		// read all alive samples and store to _appIds

		int reader = _appIdRW->GetReader();

		const int MAX_SAMPLES = 100;
		Net_AppId* appIdPtrs[MAX_SAMPLES] = { 0 }; // we want DDS to allocate memory for us (we do not need to care about freeing it)
		dds_sample_info_t infos[MAX_SAMPLES];

		int num = dds_read(reader, (void**)appIdPtrs, infos, MAX_SAMPLES, MAX_SAMPLES);
		if (num < 0)
			DDS_FATAL("dds_read: %s\n", dds_strretcode(-num));

		_appIds.clear();
		for (int i = 0; i < num; i++)
		{
			auto& si = infos[i];
			if (si.instance_state & DDS_ALIVE_INSTANCE_STATE)
			{
				printf("Found App: %s %d\n", appIdPtrs[i]->ComputerName, appIdPtrs[i]->ProcessId);
				_appIds.push_back(AppId(appIdPtrs[i]->ComputerName, appIdPtrs[i]->ProcessId));
			}
		}

		// sort appIds by name and pid
		std::sort(_appIds.begin(), _appIds.end(), [](const AppId& a, const AppId& b) -> bool
			{
				if (a.ComputerName == b.ComputerName)
					return a.ProcessId < b.ProcessId;
				else
					return a.ComputerName < b.ComputerName;
			});

		OnScanFinished( _appIds );
	}

	void AppScan::PublishAppId(const AppId& appId)
	{
		Net_AppId sample = { 0 };
		sample.ComputerName = (char*)appId.ComputerName.c_str();
		sample.ProcessId = appId.ProcessId;
		dds_write(_appIdRW->GetWriter(), &sample);
	}


}