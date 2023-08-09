#pragma once
#include "dds/dds.h"
#include <string>
#include <functional>
#include <vector>


namespace DdsPerfTest
{
	class App;

	class ParticMgr
	{
	public:
		ParticMgr(App* app);
		~ParticMgr();
		int GetParticipant(int index);

	protected:
		int CreateParticipant();

	protected:
		App* _app;
		std::vector<int> _ParticMgr;

	};
}