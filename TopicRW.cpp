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
		bool wantReader,
		bool wantWriter)
	{
		_participant = participant;
		_topicName = name;

		_topic = dds_create_topic(participant, topicDescriptor, name, NULL, NULL);
		if (_topic < 0)
			DDS_FATAL("dds_create_topic %s: %s\n", name, dds_strretcode(-_topic));

		dds_qos_t* qos = dds_create_qos();
		dds_qset_reliability(qos, reliability, DDS_SECS(10));
		dds_qset_durability(qos, durability);
		dds_qset_history(qos, history, 1);

		if( wantWriter )
		{
			_writer = dds_create_writer(participant, _topic, qos, NULL);
			if (_writer < 0)
				DDS_FATAL("dds_create_writer %s: %s\n", name, dds_strretcode(-_writer));
		}

		if( wantReader )
		{
			_reader = dds_create_reader(participant, _topic, qos, NULL);
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