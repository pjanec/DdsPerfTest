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

static std::string ParseCsvField(const char*& line) {
    std::string field;
    
    while (*line && (*line == ' ' || *line == '\t')) {
        line++;
    }
    
    if (*line == '"') {
        line++;
        while (*line && *line != '"') {
            if (*line == '\0') break;
            field += *line++;
        }
        if (*line == '"') {
            line++;
        }
        while (*line && *line != ',') line++;
        if (*line == ',') line++;
    } else {
        while (*line && *line != ',' && *line != '\r' && *line != '\n') {
            field += *line++;
        }
        if (*line == ',') line++;
    }
    
    return field;
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
				if (lineNo == 1) continue;
				
				if (line[0] == ';') continue;

				MsgDef msgDef;
				const char* ptr = line;
				
				std::string nameStr = ParseCsvField(ptr);
				std::string reliabilityStr = ParseCsvField(ptr);
				std::string durabilityStr = ParseCsvField(ptr);
				std::string historyStr = ParseCsvField(ptr);
				std::string historyDepthStr = ParseCsvField(ptr);
				std::string readStrategyStr = ParseCsvField(ptr);
				std::string partitionNameStr = ParseCsvField(ptr);
				std::string domainIdStr = ParseCsvField(ptr);

				if (nameStr.empty()) continue;

				msgDef.Name = nameStr;

				if (!_stricmp(reliabilityStr.c_str(), "Reliable")) 
					msgDef.Reliability = DDS_RELIABILITY_RELIABLE;
				else if (!_stricmp(reliabilityStr.c_str(), "BestEffort")) 
					msgDef.Reliability = DDS_RELIABILITY_BEST_EFFORT;
				else
					DDS_FATAL("Unknown reliability: %s\n", reliabilityStr.c_str());

				if (!_stricmp(durabilityStr.c_str(), "TransientLocal")) 
					msgDef.Durability = DDS_DURABILITY_TRANSIENT_LOCAL;
				else if (!_stricmp(durabilityStr.c_str(), "Volatile")) 
					msgDef.Durability = DDS_DURABILITY_VOLATILE;
				else
					DDS_FATAL("Unknown durability: %s\n", durabilityStr.c_str());

				if (!_stricmp(historyStr.c_str(), "KeepAll")) 
					msgDef.History = DDS_HISTORY_KEEP_ALL;
				else if (!_stricmp(historyStr.c_str(), "KeepLast")) 
					msgDef.History = DDS_HISTORY_KEEP_LAST;
				else
					DDS_FATAL("Unknown history: %s\n", historyStr.c_str());

				msgDef.HistoryDepth = atoi(historyDepthStr.c_str());

				if (!_stricmp(readStrategyStr.c_str(), "Poll")) 
					msgDef.ReadStrategy = ssPoll;
				else if (!_stricmp(readStrategyStr.c_str(), "ListenImmed")) 
					msgDef.ReadStrategy = ssListenImmed;
				else if (!_stricmp(readStrategyStr.c_str(), "ListenDefer")) 
					msgDef.ReadStrategy = ssListenDefer;
				else
					DDS_FATAL("Unknown readStrategy: %s\n", readStrategyStr.c_str());

				msgDef.PartitionName = partitionNameStr;
				
				if (!domainIdStr.empty()) {
					msgDef.DomainId = atoi(domainIdStr.c_str());
				}

				result.push_back(msgDef);
			}
			fclose(f);
		}
		return result;
	}
}

