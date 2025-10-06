#pragma once
#include "dds/dds.h"
#include <vector>
#include <memory>
#include "MsgDef.h"
#include "SharedData.h"
#include "SysMonitor.h"
#include "SysMonitorView.h"

namespace DdsPerfTest
{

class AllMsgsCtrl;
class TopicRW;
class DataMgr;
class ParticMgr;
class MsgEdit;
class AppScan;
class SubsStatsMgr;
class CommandMgr;

class App
{
public:
	App();
	~App();
	void Tick();
	void DrawUI();
	void Reset();
	
public:
	// index 0 is for the first tier of publishers/subscribers on machine
	// index 1 is for the second tier of publishers/subscribers on machine
	int GetParticipant( int index );
	
	std::vector<MsgDef>& GetMsgDefs() { return _msgDefs; }
	const AppId& GetAppId() const;
	std::shared_ptr<AllMsgsCtrl> GetController() const { return _allMsgCtrl; }
	int GetAppIndex() const;
	void UpdateAppIndex(const SharedData& settings);
	const std::vector<AppId>& GetApps() const;
	bool UseHighRateTick() const { return _useHighRateTick; }
	bool WantsQuit() const { return _wantsQuit; }
	int GetSubsStatsWriter() const;

	std::shared_ptr<SysMonitor> GetSysMonitor() { return _sysMonitor; }
	float& GetMonitorIntervalSec() { return _monitorIntervalSec; }


protected:
	void Init();
	void Deinit();
	void ProcessCommand( const Command& cmd );

	void DrawAppPanel();

	void SaveLocalSettings();
	void LoadLocalSettings();

	void OnSharedDataReceived(const SharedData& data);
	void OnAppsFound(const std::vector<AppId>& apps);
	void OnMsgEditChanged();

protected:
	AppId _appId;
	int _appIndex = -1; // index in the list of apps
	bool _useHighRateTick = false;
	bool _wantsQuit = false;

	std::vector<MsgDef> _msgDefs;
	std::shared_ptr<ParticMgr> _particMgr;
	std::shared_ptr<AppScan> _appScan;
	std::shared_ptr<DataMgr> _dataMgr;
	std::shared_ptr<AllMsgsCtrl> _allMsgCtrl;
	std::shared_ptr<MsgEdit> _msgEdit;
	std::shared_ptr<SubsStatsMgr> _subsStatsMgr;
	std::shared_ptr<CommandMgr> _commandMgr;

	std::shared_ptr<SysMonitor> _sysMonitor;
	std::shared_ptr<SysMonitorView> _sysMonitorView;
	float _monitorIntervalSec = 1.0f; // Monitoring interval in seconds


};


}