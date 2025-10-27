#include "stdafx.h"
#include "Subscriber.h"
#include "imgui.h"
#include "App.h"
#include "NetworkDefs.h"
#include "Timer.h"
#include "TopicRW.h"
#include "MsgDef.h"
#include "NetworkDefs.h"
#include <sstream>

namespace DdsPerfTest
{

// implement app here
Subscriber::Subscriber(App* app, std::string msgClass, int index, int domainId, const std::string& partitionName) :
	_app(app),
	_msgClass(msgClass),
	_partitionName(partitionName),
	_index(index),
	_domainId(domainId)
{
	printf("Subscriber::Subscriber(%s, %d, Domain: %d)\n", msgClass.c_str(), index, domainId);
	_timerSendStats = std::make_shared<Timer>([this]() -> void { this->SendStats(false); }, 500000);
	_timerCalcRate = std::make_shared<Timer>([this]() -> void { this->CalcRate(); }, 500000);

	auto& msgDefs = _app->GetMsgDefs();

	auto msgDefIt = std::find_if(msgDefs.begin(), msgDefs.end(), [msgClass](const MsgDef& msgDef) { return msgDef.Name == msgClass; });
	if (msgDefIt == msgDefs.end())
		DDS_FATAL("msgDef %s not found\n", msgClass.c_str());
	_msgDef = *msgDefIt;

	_participant = _app->GetParticipant(_index, _domainId);

	std::vector<std::string> partitions;
	std::stringstream ss(_partitionName);
	std::string partition;
	while (std::getline(ss, partition, ',')) {
		partition.erase(0, partition.find_first_not_of(" \t\n\r"));
		partition.erase(partition.find_last_not_of(" \t\n\r") + 1);
		if (!partition.empty()) {
			partitions.push_back(partition);
		}
	}

	// create topic with partition support
	_topicRW = std::make_shared<TopicRW>(
		_participant,
		msgClass.c_str(),
		&Net_TestMsg_desc,
		_msgDef.Reliability,
		_msgDef.Durability,
		_msgDef.History,
		_msgDef.HistoryDepth,
		partitions,  // Pass partition information
		true,   // wantReader
		false); // wantWriter

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
	if (_msgDef.ReadStrategy == ssListenDefer)
	{
		StrategyListenDefer();
	}
}

void Subscriber::StrategyPoll()
{
	PollAllSamples();
}

void Subscriber::StrategyListenImmed()
{
	// the samples are processed directly in the listener callback
}

void Subscriber::StrategyListenDefer()
{
	if (_dataAvailable)
	{
		_dataAvailable = false;
		PollAllSamples();
	}
}

void Subscriber::PollAllSamples()
{
	const int MAX_SAMPLES = 1;
	Net_TestMsg* samples[MAX_SAMPLES] = { 0 }; // DDS allocates memory for us
	dds_sample_info_t infos[MAX_SAMPLES];

	while (true)
	{
		auto ret = dds_take(_topicRW->GetReader(), (void**)samples, infos, MAX_SAMPLES, MAX_SAMPLES);
		if (ret < 0)
		{
			_numReadErrors++;
			// todo: handle error
			break;
		}
		else if (ret == 0)
		{
			// no more samples
			break;
		}
		else
		{
			// process sample
			auto sample = samples[0];
			OnSample(*sample, infos[0]);
		}
	}
}

void Subscriber::OnSample( Net_TestMsg& sample, dds_sample_info_t& info)
{
	if (info.valid_data && info.instance_state == DDS_ALIVE_INSTANCE_STATE)
	{
		// check for lost messages (seqnum gap)
		PubKey pubKey = { sample.AppId, sample.InAppIndex };
		if (_lastReceiveSeqNums.find(pubKey) != _lastReceiveSeqNums.end())
		{
			int lastSeqNum = _lastReceiveSeqNums[pubKey];
			int expectedSeqNum = lastSeqNum + 1;
			if (sample.SeqNum != expectedSeqNum)
			{
				int lost = sample.SeqNum - expectedSeqNum;
				if (lost > 0)
				{
					_numLostMsgs += lost;
				}
			}
		}
		_lastReceiveSeqNums[pubKey] = sample.SeqNum;

		_numRecvTotal++;
		_numRecvSinceLastRateCalc++;
	}
}

void Subscriber::CalcRate()
{
	_rate = _numRecvSinceLastRateCalc * 2; // we are called every 500ms, so multiply by 2 to get per-second rate
	_numRecvSinceLastRateCalc = 0;
}

void Subscriber::SendStats( bool dispose )
{
	Net_SubsStats stats;
	stats.AppIndex = _app->GetAppIndex();
	stats.MsgClass = dds_string_dup(_msgClass.c_str());
	stats.InAppIndex = _index;
	stats.DomainId = _domainId;
	stats.PartitionName = dds_string_dup(_partitionName.c_str());
	stats.Received = _numRecvTotal;
	stats.Rate = _rate;
	stats.Lost = _numLostMsgs;

	auto writer = _app->GetSubsStatsWriter();
	dds_return_t rc;
	if( dispose )
		rc = dds_dispose(writer, &stats);
	else
		rc = dds_write(writer, &stats);
	
	if( rc < 0 )
		printf("SendStats failed: %s\n", dds_strretcode(-rc));
}

void Subscriber::OnDataAvailable()
{
	_dataAvailableCount++;

	if (_msgDef.ReadStrategy == ssListenImmed)
	{
		StrategyListenImmed();
		PollAllSamples();
	}
	else
	if (_msgDef.ReadStrategy == ssListenDefer)
	{
		_dataAvailable = true;
	}
}

void Subscriber::__OnDataAvailable(dds_entity_t reader, void* arg)
{
	(void)reader;
	auto self = (Subscriber*)arg;
	self->OnDataAvailable();
}

void Subscriber::UpdateSettings(const MsgSettings& spec)
{
	_msgSpec = spec;
}

void Subscriber::DrawUI()
{
}

}
