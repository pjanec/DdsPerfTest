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


	class AppScan
	{
	public:
		AppScan(App* app);
		~AppScan();

		void Scan();
		const std::vector<AppId>& GetApps() const { return _appIds; }
		std::function<void( const std::vector<AppId>& )> OnScanFinished;

		void PublishAppId( const AppId& appId );

	protected:
		App* _app;
		std::shared_ptr<TopicRW> _appIdRW;

		// all applications detected on the network
		std::vector<AppId> _appIds;

	};
}