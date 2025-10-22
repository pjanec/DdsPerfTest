#pragma once
#include "dds/dds.h"
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <set>
#include <map>
#include <random>
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

		// Expose targeted app indices for main table highlighting
		const std::set<int>& GetTargetedAppIndices() const { return _controlPanel.targetedAppIndices; }

	protected:
		bool DrawMsgSettings(MsgSettings& msgSpec, bool& wantRemove);
		bool DrawPerAppCnt(const char* name, std::vector<int>& perAppCnts, bool& allDisabled);
		bool DrawPubSubTable(MsgSettings& msgSpec);  // NEW: Combined Pub/Sub table
		void NewMsgSpec(std::string msgClass);
		void DrawAllSettings();

		// Control Panel methods
		bool DrawControlPanel(MsgSettings& msgSpec);
		bool DrawTargetComputersSection();
		bool DrawPatternLogicSection(); 
		bool DrawActionExecutionSection(MsgSettings& msgSpec);
		void UpdateLivePreview();
		void ApplyPatternToInstances(MsgSettings& msgSpec);
		std::vector<int> ApplyPattern(const std::vector<int>& availableIndices);

	protected:
		App* _app;
		SharedData& _edited; // local copy of settings, changed from UI and published in master mode
		bool _selectingNewMsg = false;

		// ------ extended app count interface ------
		bool _appCntInterface_Open = false;
		bool _appCntInterface_Add = false;
		bool _appCntInterface_Remove = false;
		std::vector<int> _appCntInterface_Tmp;
		// ----------------------------------------

		// Control Panel State
		struct ControlPanelState {
			// Section 1: Target Computers
			std::set<std::string> selectedComputers;
			bool targetSectionOpen = false;
			
			// Section 2: Pattern Logic  
			enum class PatternType { FirstN, RangeM_N, EveryNth, RandomN };
			PatternType selectedPattern = PatternType::FirstN;
			enum class PatternScope { PerComputer, Global };
			PatternScope selectedScope = PatternScope::PerComputer;
			bool patternSectionOpen = false;
			
			// Pattern parameters
			int paramN = 1;
			int paramM = 1;
			
			// Section 3: Action & Execution
			enum class ActionType { Set, Increment, Decrement };
			ActionType pubAction = ActionType::Set;
			ActionType subAction = ActionType::Set;
			int pubValue = 1;
			int subValue = 1;
			
			// Cached random selection for persistence
			std::vector<int> cachedRandomSelection;
			int lastRandomN = -1;
			
			// Live preview data
			std::set<int> targetedAppIndices;
			int previewCount = 0;
		} _controlPanel;

		// Random number generator for random pattern
		mutable std::mt19937 _rng{std::random_device{}()};
	};
}