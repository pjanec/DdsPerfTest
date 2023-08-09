#include "stdafx.h"
#include "MsgDef.h"
#include "dds/dds.h"

namespace DdsPerfTest
{
	std::vector<MsgDef> MsgDef::ReadListFromFile(std::string fileName)
	{
		std::vector<MsgDef> result;
		FILE* f = fopen(fileName.c_str(), "r");
		int lineNo = 0;
		if (f)
		{
			char line[1024];
			while (fgets(line, sizeof(line), f))
			{
				lineNo++;
				if (lineNo == 1) continue; // skip the header line

				MsgDef msgDef;
				char name[1000] = { 0 };
				char reliability[1000] = {0};
				char durability[1000] = { 0 };
				char history[1000] = { 0 };
				int historyDepth = { 0 };
				char readStrategy[1000] = { 0 };

				if (sscanf(line, "%[^,],%[^,],%[^,],%[^,],%d,%[^\n]", name, reliability, durability, history, &historyDepth, readStrategy) == 6)
				{
					msgDef.Name = name;

					if( !_stricmp(reliability, "Reliable") ) msgDef.Reliability = DDS_RELIABILITY_RELIABLE;
					else
					if (!_stricmp(reliability, "BestEffort")) msgDef.Reliability = DDS_RELIABILITY_BEST_EFFORT;
					else
						DDS_FATAL("Unknown reliability: %s\n", reliability);

					if (!_stricmp(durability, "TransientLocal")) msgDef.Durability = DDS_DURABILITY_TRANSIENT_LOCAL;
					else
					if (!_stricmp(durability, "Volatile")) msgDef.Durability = DDS_DURABILITY_VOLATILE;
					else
						DDS_FATAL("Unknown durability: %s\n", durability);

					if (!_stricmp(history, "KeepAll")) msgDef.History = DDS_HISTORY_KEEP_ALL;
					else
					if (!_stricmp(history, "KeepLast")) msgDef.History = DDS_HISTORY_KEEP_LAST;
					else
						DDS_FATAL("Unknown history: %s\n", history);

					msgDef.HistoryDepth = historyDepth;

					if (!_stricmp(readStrategy, "Poll")) msgDef.ReadStrategy = ssPoll;
					else
					if (!_stricmp(readStrategy, "ListenImmed")) msgDef.ReadStrategy = ssListenImmed;
					else
					if (!_stricmp(readStrategy, "ListenDefer")) msgDef.ReadStrategy = ssListenDefer;
					else
						DDS_FATAL("Unknown readStrategy: %s\n", readStrategy);


					result.push_back(msgDef);
				}
			}
			fclose(f);
		}
		return result;
	}
}

