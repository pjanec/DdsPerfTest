#pragma once
#include "dds/dds.h"
#include <string>
#include <functional>
#include <vector>
#include <map>
#include <set>


namespace DdsPerfTest
{
	class App;

	class ParticMgr
	{
	public:
		ParticMgr(App* app);
		~ParticMgr();
		int GetParticipant(int index, dds_domainid_t domainId);
		void CleanupUnusedDomains(const std::set<dds_domainid_t>& activeDomains);

	protected:
		int CreateParticipant(dds_domainid_t domainId);

	protected:
		App* _app;
		// Map of domain IDs to a vector of participant handles
		std::map<dds_domainid_t, std::vector<int>> _participantsByDomain;

	};
}