#pragma once
#include "dds/dds.h"
#include <vector>
#include <map>
#include <string>
#include "NetworkDefs.h"
#include <memory>
#include "SharedData.h"

namespace DdsPerfTest
{

class App;
class MsgController;
class TopicRW;

/// <summary>
/// One per app;
/// Reads the settings from network and applies them to local controller (one per msgclass).
/// (A MsgClass represents unique set of QoS settings, reading strategy etc.)
/// Allows to edit the settings, publish them & save on each change (last app wins)
/// </summary>
class AllMsgsCtrl
{
public:
	AllMsgsCtrl( App* app );
	~AllMsgsCtrl();

	void Tick();
	void DrawUI();

	void Set(const SharedData& sharedData);

protected:
	void DrawLocalSettings();

protected:
	App* _app;
	std::map<std::string, std::shared_ptr<MsgController>> _msgControllers;



};

}