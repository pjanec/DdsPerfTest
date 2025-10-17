#include "stdafx.h"
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include "App.h"
#include "imgui.h"
#include "SharedData.h"
#include "NetworkDefs.h"
#include "dds/dds.h"
#include "AllMsgsCtrl.h"
#include "TopicRW.h"
#include "DataMgr.h"
#include "ParticMgr.h"
#include "AppScan.h"
#include "MsgEdit.h"
#include "SubsStatsMgr.h"
#include "CommandMgr.h"

extern HWND g_hWnd;

namespace DdsPerfTest
{

App::App()
	: _appId(getenv("COMPUTERNAME"), GetCurrentProcessId())
{
	Init();

}

App::~App()
{
	Deinit();
}

void App::LoadVersion()
{
	_version.clear();
	std::ifstream vf("version.txt");
	if (!vf.is_open()) return;
	std::getline(vf, _version);
	while (!_version.empty() && (_version.back()=='\r' || _version.back()=='\n' || _version.back()==' ' || _version.back()=='\t')) _version.pop_back();
}

void App::Reset()
{
	Deinit();
	Init();
}

void App::Init()
{
	_msgDefs = MsgDef::ReadListFromFile("MsgDefs.csv");

	LoadVersion();

	_particMgr = std::make_shared<ParticMgr>(this);
	
	_dataMgr = std::make_shared<DataMgr>( this, [this](const SharedData& data) { OnSharedDataReceived(data); } );
	_dataMgr->RestoreSettings();

	_msgEdit = std::make_shared<MsgEdit>( this, _dataMgr->GetLocal() );
	_msgEdit->OnChanged = [this]() { OnMsgEditChanged(); };


	_subsStatsMgr = std::make_shared<SubsStatsMgr>( this );
	
	_appScan = std::make_shared<AppScan>( this );
	_appScan->OnScanFinished = [this](const std::vector<AppId>& apps) { OnAppsFound(apps); };
	_appScan->PublishAppId(_appId);
	_appScan->Scan();

	_allMsgCtrl = std::make_shared<AllMsgsCtrl>(this);

	_commandMgr = std::make_shared<CommandMgr>(this);
	_commandMgr->OnCommand = [this](const Command& cmd) { ProcessCommand(cmd); };

	_sysMonitor = std::make_shared<SysMonitor>(this);
	_sysMonitorView = std::make_shared<SysMonitorView>(this);
	LoadLocalSettings();

	if (_sysMonitor && _sysMonitor->IsMonitorInstance())
	{
		_sysMonitor->SetSampleInterval((int)(_monitorIntervalSec * 1000.0f));
	}

	// we need to manually enable the settings on start to allow editing the settings before
	_dataMgr->GetLocal().Disabled = true;
}

void App::Deinit()
{
	SaveLocalSettings();
	_commandMgr = nullptr;
	_allMsgCtrl = nullptr;
	_subsStatsMgr = nullptr;
	_appScan = nullptr;
	_dataMgr = nullptr;
	_particMgr = nullptr;
	_sysMonitor = nullptr;
	_sysMonitorView = nullptr;
}

void App::Tick()
{
	_allMsgCtrl->Tick();
	_dataMgr->Tick();
	_subsStatsMgr->Tick();
	_commandMgr->Tick();
	_sysMonitorView->Tick();
	_dataMgr->SendAndSaveIfDirty();
}


void App::DrawUI()
{
	ImGui::Begin("App");
	DrawAppPanel();
	ImGui::End();

	ImGui::Begin("Local Settings");
	_allMsgCtrl->DrawUI();
	ImGui::End();

	ImGui::Begin("Subscriber Stats");
	_subsStatsMgr->DrawUI();
	ImGui::End();

	ImGui::Begin("Msg Setting");
	_msgEdit->DrawUI();
	ImGui::End();

	if (_sysMonitorView)
	{
		_sysMonitorView->DrawUI();
	}


}

void App::DrawAppPanel()
{
	if( ImGui::Checkbox("Disabled", &_dataMgr->GetLocal().Disabled ) )
	{
		_dataMgr->SetDirty(); // to publish at the end of tick
	}

	if (ImGui::Button("Collect apps"))
	{
		_appScan->Scan();
	}

	if (ImGui::Checkbox("High Rate Tick", &_useHighRateTick))
	{
		_commandMgr->SendCommand(Command("UseHighRateTick", _useHighRateTick ? "1":"0"));
	}

	if( ImGui::Button( "Reset this app" ) )
	{
		Reset();
	}
	ImGui::SameLine();

	if (ImGui::Button("Reset all"))
	{
		_commandMgr->SendCommand( Command("ResetAll") );
	}

	if (ImGui::Button("Kill all apps"))
	{
		_commandMgr->SendCommand(Command("KillAll"));
	}
}

const AppId& App::GetAppId() const
{
	return _appId;
}

const std::vector<AppId>& App::GetApps() const
{
	return _dataMgr->GetReceived().Apps;
}

void App::UpdateAppIndex( const SharedData& settings )
{
	int index = -1;
	// find the app in the list of apps received in last settings update
	auto& apps = settings.Apps;
	for (int i = 0; i < (int)apps.size(); i++)
	{
		if (apps[i].ComputerName == _appId.ComputerName && apps[i].ProcessId == _appId.ProcessId)
		{
			index = i;
			break;
		}
	}
	_appIndex = index;

	// set window title
	std::string versionPart = _version.empty() ? "" : (" v" + _version);
	std::string title = "DdsPerfTest" + versionPart + " - " + _appId.ComputerName + " - " + std::to_string(_appIndex);
	SetConsoleTitleA(title.c_str());
	SetWindowTextA(g_hWnd, title.c_str());
}

int App::GetAppIndex() const
{
	return _appIndex;
}


void App::ProcessCommand(const Command& cmd)
{
	printf("Received command: '%s', data='%s'\n", cmd.Type.c_str(), cmd.Data.c_str());
	if( cmd.Type == "ResetAll" )
	{
		Reset();
	}
	else
	if (cmd.Type == "UseHighRateTick")
	{
		_useHighRateTick = cmd.Data.substr(0, 1) == "1";
	}
	else
	if (cmd.Type == "KillAll")
	{
		_wantsQuit = true;
	}
}


int App::GetParticipant(int index)
{
	return _particMgr->GetParticipant(index);
}

int App::GetSubsStatsWriter() const
{
	return _subsStatsMgr->GetSubsStatsWriter();
}

void App::OnAppsFound( const std::vector<AppId>& apps )
{
	_dataMgr->GetLocal().Apps = apps;
	_dataMgr->SetDirty(); // to publish at the end of tick
}

void App::OnSharedDataReceived(const SharedData& data)
{
	// find our app in the list of apps found
	UpdateAppIndex(data);

	// update our local data  if it's not us who sent them
	if (data.Sender != GetAppId())
	{
		_dataMgr->GetLocal() = data; // note: local data are accessed by the msg settings editor
		// we do not set dirty here, because we don't want to publish the settings we just received
	}

	// update local msg controllers
	_allMsgCtrl->Set(data);
}

void App::OnMsgEditChanged()
{
	// msg edit has changed the local live data
	
	_dataMgr->SetDirty(); // to publish at the end of tick
}

void App::SaveLocalSettings()
{
	std::ofstream settingsFile("local_settings.ini");
	if (settingsFile.is_open())
	{
		settingsFile << "RecordPerformance=" << (_sysMonitorView ? _sysMonitorView->IsRecordingEnabled() : false) << std::endl;
		settingsFile << "MonitorIntervalSec=" << _monitorIntervalSec << std::endl;
	}
}

void App::LoadLocalSettings()
{
	std::ifstream settingsFile("local_settings.ini");
	if (!settingsFile.is_open()) return;
	
	std::string line;
	while (std::getline(settingsFile, line))
	{
		std::istringstream iss(line);
		std::string key, value;
		if (std::getline(iss, key, '=') && std::getline(iss, value))
		{
			if (key == "RecordPerformance") {
				if (_sysMonitorView) _sysMonitorView->SetRecordingEnabled(value == "1");
			}
			else if (key == "MonitorIntervalSec") {
				_monitorIntervalSec = std::stof(value);
			}
		}
	}
}

} // namespace DdsPerfTest