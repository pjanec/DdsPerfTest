#pragma once
#include "dds/dds.h"
#include <string>
#include <functional>
#include "SharedData.h"
#include "App.h"
#include "TopicRW.h"


namespace DdsPerfTest
{
	// manages network shared data - provides a copy, notifies about new incoming ones etc.
	class DataMgr
	{
	public:
		DataMgr(App* app, int participant, std::function<void(const SharedData&)> onReceived );
		void Tick();

		const SharedData& GetReceived() const { return _received; }
		SharedData& GetLocal() { return _local; }
		bool IsDirty() const { return _localDirty; }
		void SetDirty(bool dirty = true) { _localDirty = dirty; }

		void SendAndSaveIfDirty();

		void Send();
		void SaveSettings();
		void RestoreSettings();

	protected:
		void ReadSettings();
		void SendSettings(SharedData& settings);
		void SaveSettings( const SharedData& settings);
		void RestoreSettings(SharedData& settings);


	protected:
		App* _app;
		SharedData _received; // most recently received
		SharedData _local; // locally edited data to be sent
		bool _localDirty; // true if local data was changed and needs to be sent

		std::shared_ptr<TopicRW> _sharedDataRW;

		std::function<void(const SharedData&)> _onReceived;

	};
}