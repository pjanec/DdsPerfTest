#include "stdafx.h"
#include "TopicRW.h"

namespace DdsPerfTest
{
	// implement RWBuilder ctor
	TopicRW::TopicRW(
		int participant,
		const char* name,
		const dds_topic_descriptor* topicDescriptor,
		dds_reliability_kind reliability,
		dds_durability_kind durability,
		dds_history_kind history,
		int historyDepth,
		const std::vector<std::string>& partitions,
		bool wantReader,
		bool wantWriter)
	{
		_participant = participant;
		_topicName = name;

		_topic = dds_create_topic(participant, topicDescriptor, name, NULL, NULL);
		if (_topic < 0)
			DDS_FATAL("dds_create_topic %s: %s\n", name, dds_strretcode(-_topic));

		// Create publisher with partition QoS if we want a writer
		int publisher = -1;
		if (wantWriter) {
			dds_qos_t* pub_qos = dds_create_qos();
			
			// Apply partition QoS if partitions are specified
			if (!partitions.empty()) {
				// Convert std::vector<std::string> to array of C strings
				std::vector<const char*> partitionCStrs;
				for (const auto& partition : partitions) {
					partitionCStrs.push_back(partition.c_str());
				}
				dds_qset_partition(pub_qos, partitionCStrs.size(), partitionCStrs.data());
			}
			
			publisher = dds_create_publisher(participant, pub_qos, NULL);
			if (publisher < 0)
				DDS_FATAL("dds_create_publisher %s: %s\n", name, dds_strretcode(-publisher));
			
			dds_delete_qos(pub_qos);
		}

		// Create subscriber with partition QoS if we want a reader
		int subscriber = -1;
		if (wantReader) {
			dds_qos_t* sub_qos = dds_create_qos();
			
			// Apply partition QoS if partitions are specified
			if (!partitions.empty()) {
				// Convert std::vector<std::string> to array of C strings
				std::vector<const char*> partitionCStrs;
				for (const auto& partition : partitions) {
					partitionCStrs.push_back(partition.c_str());
				}
				dds_qset_partition(sub_qos, partitionCStrs.size(), partitionCStrs.data());
			}
			
			subscriber = dds_create_subscriber(participant, sub_qos, NULL);
			if (subscriber < 0)
				DDS_FATAL("dds_create_subscriber %s: %s\n", name, dds_strretcode(-subscriber));
			
			dds_delete_qos(sub_qos);
		}

		// Create QoS for DataWriter/DataReader
		dds_qos_t* qos = dds_create_qos();
		dds_qset_reliability(qos, reliability, DDS_SECS(10));
		dds_qset_durability(qos, durability);
		dds_qset_history(qos, history, historyDepth);

		if( wantWriter )
		{
			_writer = dds_create_writer(publisher, _topic, qos, NULL);
			if (_writer < 0)
				DDS_FATAL("dds_create_writer %s: %s\n", name, dds_strretcode(-_writer));
		}

		if( wantReader )
		{
			_reader = dds_create_reader(subscriber, _topic, qos, NULL);
			if (_reader < 0)
				DDS_FATAL("dds_create_reader %s: %s\n", name, dds_strretcode(-_reader));
		}	

		dds_delete_qos(qos);
	}

	// implement RWBuilder dtor
	TopicRW::~TopicRW()
	{
		if (_reader >= 0)
		{
			auto rc = dds_delete(_reader);
			if (rc != DDS_RETCODE_OK)
				DDS_FATAL("dds_delete(reader %s)  %s\n", _topicName.c_str(), dds_strretcode(-rc));
		}
		if (_writer >= 0)
		{
			auto rc = dds_delete(_writer);
			if (rc != DDS_RETCODE_OK)
				DDS_FATAL("dds_delete(writer %s)  %s\n", _topicName.c_str(), dds_strretcode(-rc));
		}
		if (_topic >= 0)
		{
			auto rc = dds_delete(_topic);
			if (rc != DDS_RETCODE_OK)
				DDS_FATAL("dds_delete(topic %s)  %s\n", _topicName.c_str(), dds_strretcode(-rc));
		}
	}
}