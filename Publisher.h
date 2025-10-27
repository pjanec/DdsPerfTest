#pragma once
#include "dds/dds.h"
#include <string>
#include <memory>
#include "SharedData.h"

namespace DdsPerfTest
{

class App;
class TopicRW;
class Timer;

class Publisher
{
	App* _app;
	std::string _msgClass;
	std::string _partitionName;
	int _index;
	int _domainId;

	int _participant;
	std::shared_ptr<TopicRW> _topicRW;
	MsgSettings _msgSpec;

	std::shared_ptr<Timer> _sendTimer;

	int _seqNum = 0;

public:
	Publisher( App* app, std::string msgClass, int index, int domainId, const std::string& partitionName );
	~Publisher();
	void UpdateSettings( const MsgSettings& spec );
	void Tick();
	void DrawUI();

protected:
	void SendMsg();
};

}