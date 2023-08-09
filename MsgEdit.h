#pragma once
#include "dds/dds.h"
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include "SharedData.h"



namespace DdsPerfTest
{
	class App;

	// edits the shared settings
	// notifies on change
	class MsgEdit
	{
	public:
		MsgEdit(App* app, SharedData& liveData );
		void DrawUI();
		std::function<void()> OnChanged;


	protected:
		bool DrawMsgSettings(MsgSettings& msgSpec, bool& wantRemove);
		bool DrawPerAppCnt(const char* name, std::vector<int>& perAppCnts, bool& allDisabled);
		void NewMsgSpec(std::string msgClass);
		void DrawAllSettings();

	protected:
		App* _app;
		SharedData& _edited; // local copy of settings, changed from UI and published in master mode
		bool _selectingNewMsg = false;
	};
}