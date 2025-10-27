#include "stdafx.h"
#include "DataMgr.h"
#include "ParticMgr.h"
#include "App.h"
#include "TopicRW.h"
#include <set>

namespace DdsPerfTest
{
	ParticMgr::ParticMgr(App* app)
		: _app(app)
	{
	}

	ParticMgr::~ParticMgr()
	{
		for (auto& domain_pair : _participantsByDomain)
		{
			for (auto participant : domain_pair.second)
			{
				if (participant >= 0)
				{
					int rc = dds_delete(participant);
					if (rc != DDS_RETCODE_OK)
						DDS_FATAL("dds_delete(participant): %s\n", dds_strretcode(-rc));
				}
			}
		}
		_participantsByDomain.clear();
	}

	int ParticMgr::CreateParticipant(dds_domainid_t domainId)
	{
		int participant = dds_create_participant(domainId, NULL, NULL);
		if (participant < 0)
		{
			DDS_FATAL("dds_create_participant on domain %d: %s\n", domainId, dds_strretcode(-participant));
		}
		return participant;
	}

	int ParticMgr::GetParticipant(int index, dds_domainid_t domainId)
	{
		auto& participants = _participantsByDomain[domainId];

		if (index >= (int)participants.size())
		{
			participants.resize(index + 1, -1);
		}

		if (participants[index] < 0)
		{
			participants[index] = CreateParticipant(domainId);
		}

		return participants[index];
	}

	void ParticMgr::CleanupUnusedDomains(const std::set<dds_domainid_t>& activeDomains)
	{
		for (auto it = _participantsByDomain.begin(); it != _participantsByDomain.end(); )
		{
			dds_domainid_t domainId = it->first;

			if (domainId == 0)
			{
				++it;
				continue;
			}

			if (activeDomains.find(domainId) == activeDomains.end())
			{
				printf("Cleaning up unused participants for Domain ID: %d\n", domainId);

				for (auto participant : it->second)
				{
					if (participant >= 0)
					{
						int rc = dds_delete(participant);
						if (rc != DDS_RETCODE_OK)
						{
							fprintf(stderr, "dds_delete(participant) for domain %d failed: %s\n", domainId, dds_strretcode(-rc));
						}
					}
				}

				it = _participantsByDomain.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

}