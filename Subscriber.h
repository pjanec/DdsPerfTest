#pragma once
#include "dds/dds.h"
#include <string>
#include <memory>
#include <atomic>
#include "SharedData.h"
#include "MsgDef.h"
#include "NetworkDefs.h"

namespace DdsPerfTest
{

class App;
class Timer;
class TopicRW;

class Subscriber
{
	App* _app;
	std::string _msgClass;
	std::string _partitionName;
	int _index;
	int _domainId;
	std::shared_ptr<Timer> _timerSendStats;
	std::shared_ptr<Timer> _timerCalcRate;

	int _participant;
	std::shared_ptr<TopicRW> _topicRW;
	MsgSettings _msgSpec;
	MsgDef _msgDef;

	int _numReadErrors = 0;
	int _numLostMsgs = 0;
	int _numRecvTotal = 0;
	int _numRecvSinceLastRateCalc = 0;
	int _rate = 0;

	dds_listener* _listener = nullptr;
	std::atomic<int> _dataAvailableCount = 0;
	bool _dataAvailable = false;

	std::map<PubKey, int> _lastReceiveSeqNums;

public:
	Subscriber(App* app, std::string msgClass, int index, int domainId, const std::string& partitionName);
	~Subscriber();
	void UpdateSettings(const MsgSettings& spec);
	void Tick();
	void DrawUI();

protected:
	void SendStats( bool dispose );
	void TickReadStrategy();
	void StrategyPoll();
	void StrategyListenImmed();
	void StrategyListenDefer();
	void OnSample( Net_TestMsg& sample, dds_sample_info_t& info);
	void CalcRate();
	void PollAllSamples();
	void OnDataAvailable();

	static void __OnDataAvailable(dds_entity_t reader, void* arg);

};

}