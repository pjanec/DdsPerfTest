#pragma once
#include <string>
#include <vector>
#include "dds/dds.h"

namespace DdsPerfTest
{
	enum EReadStrategy
	{
		ssPoll,	// polled in main loop
		ssListenImmed,	// sample taked right from the listener's callback
		ssListenDefer, // notification queued, sample polled in main loop if there is a notification
		ssWaitset
	};

	class MsgDef
	{
	public:
		std::string Name;
		dds_reliability_kind Reliability;
		dds_durability_kind Durability;
		dds_history_kind History;
		int HistoryDepth;
		EReadStrategy ReadStrategy;

		static std::vector<MsgDef> ReadListFromFile(std::string fileName);

		// default zeroing constructor
		MsgDef()
			: Name(""), Reliability(DDS_RELIABILITY_RELIABLE), Durability(DDS_DURABILITY_VOLATILE), History(DDS_HISTORY_KEEP_ALL), HistoryDepth(0), ReadStrategy(ssPoll)
		{
		}
		
		// ctor with fields
		MsgDef(std::string name, dds_reliability_kind reliability, dds_durability_kind durability, dds_history_kind history, int historyDepth, EReadStrategy readStrategy)
			: Name(name), Reliability(reliability), Durability(durability), History(history), HistoryDepth(historyDepth), ReadStrategy(readStrategy)
		{
		}
	};

}

