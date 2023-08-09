#include "stdafx.h"
#include "DataMgr.h"
#include "ParticMgr.h"
#include "App.h"
#include "TopicRW.h"

namespace DdsPerfTest
{
	ParticMgr::ParticMgr(App* app)
		: _app(app)
	{
	}

	ParticMgr::~ParticMgr()
	{
		for (auto participant : _ParticMgr)
		{
			int rc = dds_delete(participant);
			if (rc != DDS_RETCODE_OK)
				DDS_FATAL("dds_delete(participant): %s\n", dds_strretcode(-rc));
		}
		_ParticMgr.clear();
	}

	int ParticMgr::CreateParticipant()
	{
		// create participant
		int participant = dds_create_participant(DDS_DOMAIN_DEFAULT, NULL, NULL);
		if (participant < 0)
		{
			DDS_FATAL("dds_create_participant: %s\n", dds_strretcode(-participant));
		}
		return participant;
	}

	int ParticMgr::GetParticipant(int index)
	{
		if (index >= (int)_ParticMgr.size())
		{
			// create the missing ones
			for (int i = (int)_ParticMgr.size(); i <= index; i++)
				_ParticMgr.push_back( CreateParticipant() );
		}
		return _ParticMgr[index];
	}

}