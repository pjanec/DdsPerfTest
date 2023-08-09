#include "stdafx.h"
#include "Subscriber.h"
#include "imgui.h"
#include "App.h"
#include "NetworkDefs.h"
#include "Timer.h"
#include "TopicRW.h"
#include "MsgDef.h"
#include "NetworkDefs.h"

namespace DdsPerfTest
{

// implement app here
Subscriber::Subscriber(App* app, std::string msgClass, int index) :
	_app(app),
	_msgClass(msgClass),
	_index(index)
{
	printf("Subscriber::Subscriber(%s, %d)\n", msgClass.c_str(), index);
	_timerSendStats = std::make_shared<Timer>([this]() -> void { this->SendStats(false); }, 500000);
	_timerCalcRate = std::make_shared<Timer>([this]() -> void { this->CalcRate(); }, 500000);

	auto& msgDefs = _app->GetMsgDefs();

	// find msg by name
	auto msgDefIt = std::find_if(msgDefs.begin(), msgDefs.end(), [msgClass](const MsgDef& msgDef) { return msgDef.Name == msgClass; });
	if (msgDefIt == msgDefs.end())
		DDS_FATAL("msgDef %s not found\n", msgClass.c_str());
	_msgDef = *msgDefIt;

	_participant = _app->GetParticipant(_index);

	// create topic
	_topicRW = std::make_shared<TopicRW>(
		_participant,
		msgClass.c_str(),
		&Net_TestMsg_desc,
		_msgDef.Reliability,
		_msgDef.Durability,
		_msgDef.History,
		_msgDef.HistoryDepth,
		true,
		false);

	// setup listener
	if( _msgDef.ReadStrategy == ssListenImmed || _msgDef.ReadStrategy == ssListenDefer )
	{
		_listener = dds_create_listener( this ); 
		dds_lset_data_available(_listener, &Subscriber::__OnDataAvailable);

		dds_set_listener( _topicRW->GetReader(), _listener );
	}
}

Subscriber::~Subscriber()
{
	if (_listener )
	{
		dds_set_listener(_topicRW->GetReader(), nullptr);
		dds_delete_listener( _listener );
		_listener = nullptr;
	}

	printf("Subscriber::~Subscriber(%s, %d)\n", _msgClass.c_str(), _index);
	SendStats(true);
}

void Subscriber::Tick()
{
	// receive msg from publisher of the same msgclass
	// check if no msg is lost
	// count the rate of received msgs using floating average
	// from time to time publish own status (rate, lost msgs etc.)
	_timerSendStats->Tick();
	_timerCalcRate->Tick();

	TickReadStrategy();
}

void Subscriber::TickReadStrategy()
{
	if (_msgDef.ReadStrategy == ssPoll)
	{
		StrategyPoll();
	}
	else
	if (_msgDef.ReadStrategy == ssListenImmed)
	{
		StrategyListenImmed();
	}
	else
	if (_msgDef.ReadStrategy == ssListenDefer)
	{
		StrategyListenDefer();
	}
}

void Subscriber::PollAllSamples()
{
	while(true)	// keep reading until there is some data
	{
		const int MAX_SAMPLES = 10;
		Net_TestMsg* samples[MAX_SAMPLES] = { 0 }; // we want DDS to allocate memory for us (we do not need to care about freeing it)
		dds_sample_info_t infos[MAX_SAMPLES];

		int num = dds_take(_topicRW->GetReader(), (void**)samples, infos, MAX_SAMPLES, MAX_SAMPLES);
		
		if (num == DDS_RETCODE_NO_DATA)
			break;

		if (num == 0)
			break;

		if (num < 0)
		{
			printf("dds_read(%s)): %s\n", _msgSpec.Name.c_str(), dds_strretcode(-num));
			_numReadErrors++;
		}

		for (int i = 0; i < num; i++)
		{
			auto* sample = samples[i];
			OnSample(*sample, infos[i]);
		}
	}
}

void Subscriber::StrategyPoll()
{
	PollAllSamples();
}

void Subscriber::OnDataAvailable()
{
	if( _msgDef.ReadStrategy == ssListenImmed )
	{
		PollAllSamples();
	}
	else
	if (_msgDef.ReadStrategy == ssListenDefer)
	{
		_dataAvailableCount++;
	}
}


void Subscriber::StrategyListenImmed()
{
	// nothing here, polling aready done in OnDataAvailable
}

void Subscriber::StrategyListenDefer()
{
	if (_dataAvailableCount > 0)
	{
		PollAllSamples();
		_dataAvailableCount--;
	}
}


void Subscriber::OnSample(Net_TestMsg& sample, dds_sample_info_t& info)
{
	_numRecvTotal++;
	_numRecvSinceLastRateCalc++;

	PubKey key;
	key.AppIndex = sample.AppId;
	key.InAppIndex = sample.InAppIndex;

	// find key in last received samples
	auto it = _lastReceiveSeqNums.find(key);
	if (it != _lastReceiveSeqNums.end())
	{						
		// check if seqnum is correct
		int expected = it->second + 1;
		if (sample.SeqNum != expected)
		{
			_numLostMsgs++;
			printf("%s:%d: Seqnum mismatch: expected %d, got %d\n", _msgClass.c_str(), _index, expected, sample.SeqNum);
		}
		it->second = sample.SeqNum;
	}
	else // first sample from this publisher, remember the seqnum
	{
		_lastReceiveSeqNums[key] = sample.SeqNum;
	}

}



void Subscriber::SendStats( bool dispose )
{
	Net_SubsStats stats = {0};

	stats.InAppIndex = _index;
	stats.AppIndex = _app->GetAppIndex();
	stats.MsgClass = (char*) _msgClass.c_str();
	stats.Received = _numRecvTotal;
	stats.Rate = _rate;
	stats.Lost = _numLostMsgs;


	int writer = _app->GetSubsStatsWriter();
	if( dispose )
	{
		dds_writedispose(writer, &stats);
		dds_unregister_instance(writer, &stats);
	}
	else
	{
		dds_write(writer, &stats);
	}
}


void Subscriber::DrawUI()
{
    ImGui::Begin("Subscriber");
    ImGui::Button("Hi!");
    ImGui::End();

}

void Subscriber::UpdateSettings(const MsgSettings& spec)
{
	_msgSpec = spec;
}

void Subscriber::CalcRate()
{
	int deltaUsec = (int) _timerCalcRate->GetElapsedMicroseconds();
	if( deltaUsec == 0 )
	{
		return; // WTF?
	}
	_rate = (long long) _numRecvSinceLastRateCalc * 1000000L / deltaUsec;

	_numRecvSinceLastRateCalc = 0;
}

void Subscriber::__OnDataAvailable(dds_entity_t reader, void* arg)
{
	Subscriber* p = (Subscriber*)arg;

	p->OnDataAvailable();
}

}
