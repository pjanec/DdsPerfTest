#pragma once
#include "dds/dds.h"
#include <string>
#include <vector>

namespace DdsPerfTest
{
	// creates a topic, writer and reader
	class TopicRW
	{
	public:
		TopicRW(
			int participant,
			const char* name,
			const dds_topic_descriptor* topicDescriptor,
			dds_reliability_kind reliability,
			dds_durability_kind durability,
			dds_history_kind history,
			int historyDepth,
			const std::vector<std::string>& partitions = {},  // NEW: partition names
			bool wantReader = true,
			bool wantWriter = true);
		~TopicRW();

		int GetParticipant() const { return _participant; }
		int GetTopic() const { return _topic; }
		int GetWriter() const { return _writer; }
		int GetReader() const { return _reader; }
		std::string GetTopicName() const { return _topicName; }

	protected:
		std::string _topicName;
		int _participant = -1;
		int _topic = -1;
		int _writer = -1;
		int _reader = -1;

	};
}