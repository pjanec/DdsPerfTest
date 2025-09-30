#include "stdafx.h"
#include "MsgDef.h"
#include "dds/dds.h"
#include <sstream>

namespace DdsPerfTest
{
	std::vector<std::string> MsgDef::ParsePartitions() const
	{
		std::vector<std::string> partitions;
		if (PartitionName.empty()) {
			return partitions; // Return empty vector for default partition
		}

		std::stringstream ss(PartitionName);
		std::string partition;
		while (std::getline(ss, partition, ',')) {
			// Trim whitespace from partition names
			partition.erase(0, partition.find_first_not_of(" \t\n\r\""));
			partition.erase(partition.find_last_not_of(" \t\n\r\"") + 1);
			if (!partition.empty()) {
				partitions.push_back(partition);
			}
		}
		return partitions;
	}

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
				
				// Skip comment lines starting with ';'
				if (line[0] == ';') continue;

				MsgDef msgDef;
				char name[1000] = { 0 };
				char reliability[1000] = {0};
				char durability[1000] = { 0 };
				char history[1000] = { 0 };
				int historyDepth = { 0 };
				char readStrategy[1000] = { 0 };
				char partitionName[1000] = { 0 };

				int fieldsRead = sscanf(line, "%[^,],%[^,],%[^,],%[^,],%d,%[^,],%[^\n]", 
					name, reliability, durability, history, &historyDepth, readStrategy, partitionName);

				if (fieldsRead >= 6)  // At least the original 6 fields
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

					// Handle partition name (7th field, optional)
					if (fieldsRead >= 7) {
						msgDef.PartitionName = partitionName;
					}

					result.push_back(msgDef);
				}
			}
			fclose(f);
		}
		return result;
	}
}

