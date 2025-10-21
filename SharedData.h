#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>
#include "dds/dds.h"

namespace DdsPerfTest
{
	const int MAX_APPS = 1000;

	struct AppId
	{
		std::string ComputerName;
		int ProcessId;

		// default ctor that zeroes the contents
		AppId() : ComputerName(""), ProcessId(0) {}

		// ctor with field initialization
		AppId(const char* computerName, int processId) : ComputerName(computerName), ProcessId(processId) {}

		// equality operator
		bool operator==(const AppId& other) const
		{
			return ComputerName == other.ComputerName && ProcessId == other.ProcessId;
		}
	};


	// settings for an instance of a message class - interactively editable
	struct MsgSettings
	{
		std::string Name;
		int Disabled;
		bool Opened;
		int Rate;
		int Size;
		std::vector<int> SubsCnt;
		std::vector<int> PublCnt;
		bool AllSubsDisabled;
		bool AllPublDisabled;


		// default ctor that zeroes the contents
		MsgSettings()
		: Name(""), Disabled(0), Opened(false), Rate(0), Size(0), SubsCnt(), PublCnt(), AllSubsDisabled(false), AllPublDisabled(false)
		{
		}

		// ctor with fields
		MsgSettings(std::string name, int disabled, bool opened, int rate, int size, const std::vector<int>& subsCnt, const std::vector<int>& publCnt, bool allSubsDisabled, bool allPublDisabled)
			: Disabled(disabled), Name(name), Opened(opened), Rate(rate), Size(size), SubsCnt(subsCnt), PublCnt(publCnt), AllSubsDisabled(allSubsDisabled), AllPublDisabled(allPublDisabled)
		{
		}
	};

	struct SharedData
	{
		AppId Sender;
		bool Disabled = false; // true = do not apply any settings
		std::vector<AppId> Apps;
		std::map<std::string, MsgSettings> Msgs;
	};

	struct PubSubKey
	{
		std::string MsgName;
		int AppIndex;
		int InAppIndex;	// index of subscriber/publisher within an app

		// equality operator
		bool operator==(const PubSubKey& other) const
		{
			return MsgName == other.MsgName && AppIndex == other.AppIndex && InAppIndex == other.InAppIndex;
		}

		// less operator
		bool operator<(const PubSubKey& other) const
		{
			if (MsgName < other.MsgName)
				return true;
			if (MsgName > other.MsgName)
				return false;
			if (AppIndex < other.AppIndex)
				return true;
			if (AppIndex > other.AppIndex)
				return false;
			return InAppIndex < other.InAppIndex;
		}

		// hash function
		struct Hash
		{
			size_t operator()(const PubSubKey& key) const
			{
				return std::hash<std::string>()(key.MsgName) ^ std::hash<int>()(key.AppIndex) ^ std::hash<int>()(key.InAppIndex);
			}
		};

	};

	
	// key of a publisher for a concrete message class
	// use to identify publisher within a subscriber
	struct PubKey
	{
		int AppIndex;
		int InAppIndex;	// index of subscriber/publisher within an app

		// equality operator
		bool operator==(const PubKey& other) const
		{
			return AppIndex == other.AppIndex && InAppIndex == other.InAppIndex;
		}

		// less operator
		bool operator<(const PubKey& other) const
		{
			if (AppIndex < other.AppIndex)
				return true;
			if (AppIndex > other.AppIndex)
				return false;
			return InAppIndex < other.InAppIndex;
		}

		// hash function
		struct Hash
		{
			size_t operator()(const PubKey& key) const
			{
				return std::hash<int>()(key.AppIndex) ^ std::hash<int>()(key.InAppIndex);
			}
		};

	};

	struct SubsStats
	{
		PubSubKey Key;
		std::string PartitionName;  // NEW: Partition name(s) for this subscriber
		int Received;
		int Rate;
		int Lost;
	};

	struct Command
	{
		std::string Type;
		std::string Data;

		Command() {}
		Command(const char* type, const char* data=nullptr) : Type(type), Data(data?data:"") {}
	};

}

template <>
struct std::hash<DdsPerfTest::PubSubKey>
{
	size_t operator()(const DdsPerfTest::PubSubKey& key) const
	{
		return DdsPerfTest::PubSubKey::Hash()(key);
	}
};

template <>
struct std::hash<DdsPerfTest::PubKey>
{
	size_t operator()(const DdsPerfTest::PubKey& key) const
	{
		return DdsPerfTest::PubKey::Hash()(key);
	}
};

