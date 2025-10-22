#include "stdafx.h"
#include "MsgEdit.h"
#include "App.h"
#include "imgui.h"
#include <algorithm>
#include <random>

namespace DdsPerfTest
{
	MsgEdit::MsgEdit(App* app, SharedData& liveData)
	    : _app(app)
        , _edited(liveData)
	{
    }

    void MsgEdit::DrawUI()
    {
        DrawAllSettings();
    }

    void MsgEdit::NewMsgSpec(std::string msgClass)
    {
        MsgSettings msg;
        msg.Name = msgClass;
        msg.Disabled = true; // must be enabled manually after creation (to avoid immeadiate applicatio of this new msg settings)

        _edited.Msgs[msgClass] = msg;
    }

    bool MsgEdit::DrawMsgSettings(MsgSettings& msgSpec, bool& wantRemove)
    {
        bool changed = false;

        ImGui::SetNextItemOpen(msgSpec.Opened);
        if (ImGui::CollapsingHeader(msgSpec.Name.c_str()))
        {
            msgSpec.Opened = true;
            ImGui::Indent();
            
            // Find the message definition to get partition info
            auto& msgDefs = _app->GetMsgDefs();
            auto msgDefIt = std::find_if(msgDefs.begin(), msgDefs.end(), 
                [&msgSpec](const MsgDef& msgDef) { return msgDef.Name == msgSpec.Name; });
            
            // Show partition information (read-only)
            if (msgDefIt != msgDefs.end()) {
                std::string partitionDisplay = msgDefIt->PartitionName.empty() ? "(default)" : msgDefIt->PartitionName;
                ImGui::InputText("Partition", (char*)partitionDisplay.c_str(), partitionDisplay.size(), ImGuiInputTextFlags_ReadOnly);
            }
            
            changed |= ImGui::Checkbox("Disabled", ((bool*)&msgSpec.Disabled));
            changed |= ImGui::SliderInt("Rate", &msgSpec.Rate, 0, 1000, "%d");
            changed |= ImGui::SliderInt("Size", &msgSpec.Size, 0, 100000, "%d");

            // Use the new combined publisher/subscriber table
            changed |= DrawPubSubTable(msgSpec);

            if (ImGui::Button("Remove"))
            {
                wantRemove = true;
            }
            ImGui::Unindent();

        }
        else msgSpec.Opened = false;

        return changed;
    }

    bool MsgEdit::DrawPerAppCnt(const char* name, std::vector<int>& perAppCnts, bool& allDisabled)
    {
        // This method is no longer used in the new implementation
        // The logic has been moved to DrawPubSubTable
        return false;
    }

    bool MsgEdit::DrawPubSubTable(MsgSettings& msgSpec)
    {
        bool changed = false;

        // Calculate totals for both publishers and subscribers
        int totalPubs = 0, totalSubs = 0;
        for (int count : msgSpec.PublCnt) totalPubs += count;
        for (int count : msgSpec.SubsCnt) totalSubs += count;
        
        std::string counts = "(Pubs: " + std::to_string(totalPubs) + ", Subs: " + std::to_string(totalSubs) + ")";

        // Main collapsible header for Publishers & Subscribers
        bool headerOpen = ImGui::CollapsingHeader("Publishers & Subscribers");
        
        // Draw counts as right-aligned text overlaid on the same line as the header
        ImVec2 headerPos = ImGui::GetItemRectMin();
        ImVec2 headerSize = ImGui::GetItemRectSize();
        ImVec2 textSize = ImGui::CalcTextSize(counts.c_str());
        ImVec2 textPos = ImVec2(headerPos.x + headerSize.x - textSize.x, headerPos.y);
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), counts.c_str());
        
        if (headerOpen) {
            ImGui::Indent();
            
            // ===== DDS APPLICATION CONTROL PANEL =====
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "DDS Command Center");
            ImGui::Separator();
            
            changed |= DrawControlPanel(msgSpec);
            
            ImGui::Separator();
            // ===== END CONTROL PANEL =====

            // Publishers section
            ImGui::Text("Publishers:");
            ImGui::SameLine();
            
            // Input field for publisher count
            static std::map<std::string, int> pubSetValues; // Store per message type
            if (pubSetValues.find(msgSpec.Name) == pubSetValues.end()) {
                pubSetValues[msgSpec.Name] = 1; // Default value
            }
            
            ImGui::SetNextItemWidth(60);
            ImGui::InputInt("##pubSetValue", &pubSetValues[msgSpec.Name], 0, 0, ImGuiInputTextFlags_CharsDecimal);
            if (pubSetValues[msgSpec.Name] < 0) pubSetValues[msgSpec.Name] = 0;
            
            ImGui::SameLine();
            if (ImGui::Button("Set All Pubs")) {
                for (size_t i = 0; i < _edited.Apps.size() && i < msgSpec.PublCnt.size(); i++) {
                    msgSpec.PublCnt[i] = pubSetValues[msgSpec.Name];
                }
                changed = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear All Pubs")) {
                for (size_t i = 0; i < msgSpec.PublCnt.size(); i++) {
                    msgSpec.PublCnt[i] = 0;
                }
                changed = true;
            }
            
            // Subscribers section
            ImGui::Text("Subscribers:");
            ImGui::SameLine();
            
            // Input field for subscriber count
            static std::map<std::string, int> subSetValues; // Store per message type
            if (subSetValues.find(msgSpec.Name) == subSetValues.end()) {
                subSetValues[msgSpec.Name] = 1; // Default value
            }
            
            ImGui::SetNextItemWidth(60);
            ImGui::InputInt("##subSetValue", &subSetValues[msgSpec.Name], 0, 0, ImGuiInputTextFlags_CharsDecimal);
            if (subSetValues[msgSpec.Name] < 0) subSetValues[msgSpec.Name] = 0;
            
            ImGui::SameLine();
            if (ImGui::Button("Set All Subs")) {
                for (size_t i = 0; i < _edited.Apps.size() && i < msgSpec.SubsCnt.size(); i++) {
                    msgSpec.SubsCnt[i] = subSetValues[msgSpec.Name];
                }
                changed = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear All Subs")) {
                for (size_t i = 0; i < msgSpec.SubsCnt.size(); i++) {
                    msgSpec.SubsCnt[i] = 0;
                }
                changed = true;
            }
            
            ImGui::NewLine();
            if (ImGui::Checkbox("Disable All Publishers", &msgSpec.AllPublDisabled)) {
                changed = true;
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Disable All Subscribers", &msgSpec.AllSubsDisabled)) {
                changed = true;
            }
            
            ImGui::Separator();
            
            // Ensure both vectors are properly sized
            if (msgSpec.PublCnt.size() < _edited.Apps.size()) {
            msgSpec.PublCnt.resize(_edited.Apps.size(), 0);
            }
            if (msgSpec.SubsCnt.size() < _edited.Apps.size()) {
            msgSpec.SubsCnt.resize(_edited.Apps.size(), 0);
            }
            
            // Calculate adaptive table height based on number of apps
            int numApps = (int)std::max(msgSpec.PublCnt.size(), msgSpec.SubsCnt.size());
            float rowHeight = 25.0f;
            float headerHeight = 30.0f;
            float maxTableHeight = 500.0f;  // Increased from 300px
            float minTableHeight = 150.0f;  // Minimum height to show at least 5-6 rows
            
            // Adaptive height: show more rows for larger deployments
            float idealHeight = headerHeight + (numApps * rowHeight) + 20.0f; // +20 for padding
            float actualHeight = std::max(minTableHeight, std::min(maxTableHeight, idealHeight));
            
            // Show info about scrolling for large deployments
            if (numApps > 15) {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                    "Showing %d apps (scroll to see all)", numApps);
            }
            
            // Single table for both publishers and subscribers
            if (ImGui::BeginTable("PubSubTable", 4, 
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | 
                ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable |
                ImGuiTableFlags_SizingFixedFit,  // Better column sizing
                ImVec2(0, actualHeight))) {
                
                // Table headers
                ImGui::TableSetupColumn("App", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("Computer:PID", ImGuiTableColumnFlags_WidthStretch, 200.0f);
                ImGui::TableSetupColumn("Pubs", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableSetupColumn("Subs", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableSetupScrollFreeze(0, 1); // Freeze header row
                ImGui::TableHeadersRow();
                
                // Table rows - highlight targeted rows
                for (int i = 0; i < numApps; i++) {
                    bool haveApp = i < (int)_edited.Apps.size();
                    bool isTargeted = _controlPanel.targetedAppIndices.count(i) > 0;
                    
                    ImGui::TableNextRow();
                    ImGui::PushID(i);
                    
                    // Highlight targeted rows
                    if (isTargeted) {
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, 
                            ImGui::GetColorU32(ImVec4(0.3f, 0.7f, 0.3f, 0.3f)));
                    }
                    
                    // App index column
                    ImGui::TableNextColumn();
                    if (!haveApp) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5, 0.5, 0.5, 1));
                    }
                    ImGui::Text("%d", i);
                    
                    // Computer name column
                    ImGui::TableNextColumn();
                    if (haveApp) {
                        ImGui::Text("%s:%d", _edited.Apps[i].ComputerName.c_str(), _edited.Apps[i].ProcessId);
                    } else {
                        ImGui::Text("[not available]");
                    }
                    
                    // Publishers count column
                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-1);
                    if (i < (int)msgSpec.PublCnt.size()) {
                        char pubBuf[100];
                        sprintf(pubBuf, "%d", msgSpec.PublCnt[i]);
                        if (ImGui::InputText("##pub", pubBuf, sizeof(pubBuf), 
                            ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll)) {
                            int val = atoi(pubBuf);
                            if (val < 0) val = 0;
                            msgSpec.PublCnt[i] = val;
                            changed = true;
                        }
                    }
                    
                    // Subscribers count column
                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-1);
                    if (i < (int)msgSpec.SubsCnt.size()) {
                        char subBuf[100];
                        sprintf(subBuf, "%d", msgSpec.SubsCnt[i]);
                        if (ImGui::InputText("##sub", subBuf, sizeof(subBuf), 
                            ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll)) {
                            int val = atoi(subBuf);
                            if (val < 0) val = 0;
                            msgSpec.SubsCnt[i] = val;
                            changed = true;
                        }
                    }
                    
                    if (!haveApp) {
                        ImGui::PopStyleColor();
                    }
                    
                    ImGui::PopID();
                }
                
                ImGui::EndTable();
            }
            
            ImGui::Unindent();
        }
        
        return changed;
    }

    bool MsgEdit::DrawControlPanel(MsgSettings& msgSpec)
    {
        bool changed = false;
        
        changed |= DrawTargetComputersSection();
        changed |= DrawPatternLogicSection();
        changed |= DrawActionExecutionSection(msgSpec);
        
        // Update live preview whenever anything changes
        if (changed) {
            UpdateLivePreview();
        }
        
        return changed;
    }

    bool MsgEdit::DrawTargetComputersSection()
    {
        bool changed = false;
        
        // Dynamic header with selected count
        int selectedCount = (int)_controlPanel.selectedComputers.size();
        std::string headerLabel = "Target Computers (" + std::to_string(selectedCount) + " selected)";
        
        ImGui::SetNextItemOpen(_controlPanel.targetSectionOpen);
        if (ImGui::CollapsingHeader(headerLabel.c_str())) {
            _controlPanel.targetSectionOpen = true;
            ImGui::Indent();
            
            // Quick action buttons
            if (ImGui::Button("Select All")) {
                for (const auto& app : _edited.Apps) {
                    _controlPanel.selectedComputers.insert(app.ComputerName);
                }
                changed = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Select None")) {
                _controlPanel.selectedComputers.clear();
                changed = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Invert Selection")) {
                std::set<std::string> allComputers;
                for (const auto& app : _edited.Apps) {
                    allComputers.insert(app.ComputerName);
                }
                std::set<std::string> newSelection;
                std::set_difference(allComputers.begin(), allComputers.end(),
                                  _controlPanel.selectedComputers.begin(), _controlPanel.selectedComputers.end(),
                                  std::inserter(newSelection, newSelection.begin()));
                _controlPanel.selectedComputers = newSelection;
                changed = true;
            }
            
            ImGui::Separator();
            
            // Group computers by prefix (assuming names like "Computer01", "Computer02", etc.)
            std::map<std::string, std::vector<std::string>> computerGroups;
            for (const auto& app : _edited.Apps) {
                // Extract prefix (everything before the last digits)
                std::string name = app.ComputerName;
                std::string prefix = name;
                
                // Find where digits start from the end
                size_t digitStart = name.length();
                while (digitStart > 0 && std::isdigit(name[digitStart-1])) {
                    digitStart--;
                }
                if (digitStart < name.length()) {
                    prefix = name.substr(0, digitStart);
                }
                
                computerGroups[prefix].push_back(name);
            }
            
            // Draw grouped computer selection
            for (auto& [prefix, computers] : computerGroups) {
                // Sort computers within group
                std::sort(computers.begin(), computers.end());
                
                // Remove duplicates
                computers.erase(std::unique(computers.begin(), computers.end()), computers.end());
                
                // Group header with master checkbox
                bool allSelected = true;
                bool noneSelected = true;
                for (const auto& comp : computers) {
                    bool isSelected = _controlPanel.selectedComputers.count(comp) > 0;
                    if (isSelected) noneSelected = false;
                    if (!isSelected) allSelected = false;
                }
                
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
                if (ImGui::TreeNodeEx(prefix.c_str(), flags)) {
                    ImGui::SameLine();
                    
                    // Use regular checkbox instead of mixed-value checkbox for older ImGui
                    bool masterChecked = allSelected;
                    std::string masterLabel = "##master_" + prefix;
                    if (!allSelected && !noneSelected) {
                        // Indicate mixed state with text color
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.0f, 1.0f));
                        masterLabel = "[-] " + masterLabel;
                    }
                    
                    if (ImGui::Checkbox(masterLabel.c_str(), &masterChecked)) {
                        for (const auto& comp : computers) {
                            if (masterChecked) {
                                _controlPanel.selectedComputers.insert(comp);
                            } else {
                                _controlPanel.selectedComputers.erase(comp);
                            }
                        }
                        changed = true;
                    }
                    
                    if (!allSelected && !noneSelected) {
                        ImGui::PopStyleColor();
                    }
                    
                    ImGui::Indent();
                    // Individual computer checkboxes
                    for (const auto& comp : computers) {
                        bool isSelected = _controlPanel.selectedComputers.count(comp) > 0;
                        if (ImGui::Checkbox(comp.c_str(), &isSelected)) {
                            if (isSelected) {
                                _controlPanel.selectedComputers.insert(comp);
                            } else {
                                _controlPanel.selectedComputers.erase(comp);
                            }
                            changed = true;
                        }
                    }
                    ImGui::Unindent();
                    ImGui::TreePop();
                }
            }
            
            ImGui::Unindent();
        } else {
            _controlPanel.targetSectionOpen = false;
        }
        
        return changed;
    }

    bool MsgEdit::DrawPatternLogicSection()
    {
        bool changed = false;
        
        ImGui::SetNextItemOpen(_controlPanel.patternSectionOpen);
        if (ImGui::CollapsingHeader("Pattern Logic")) {
            _controlPanel.patternSectionOpen = true;
            ImGui::Indent();
            
            // Pattern template selector
            const char* patternNames[] = { "First N apps", "Apps M..N", "Every N-th app", "N random apps" };
            int currentPattern = (int)_controlPanel.selectedPattern;
            if (ImGui::Combo("Pattern", &currentPattern, patternNames, IM_ARRAYSIZE(patternNames))) {
                _controlPanel.selectedPattern = (ControlPanelState::PatternType)currentPattern;
                changed = true;
            }
            
            // Pattern scope selector (disabled for random pattern)
            bool scopeDisabled = (_controlPanel.selectedPattern == ControlPanelState::PatternType::RandomN);
            if (scopeDisabled) {
                ImGui::Text("Scope: (Random pattern uses Global scope)");
            } else {
                ImGui::Text("Scope:");
                ImGui::SameLine();
                bool perComputer = (_controlPanel.selectedScope == ControlPanelState::PatternScope::PerComputer);
                if (ImGui::RadioButton("Per-Computer", perComputer)) {
                    _controlPanel.selectedScope = ControlPanelState::PatternScope::PerComputer;
                    changed = true;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Global", !perComputer)) {
                    _controlPanel.selectedScope = ControlPanelState::PatternScope::Global;
                    changed = true;
                }
            }
            
            // Unified parameter block
            ImGui::Separator();
            ImGui::Text("Parameters:");
            
            if (ImGui::InputInt("N", &_controlPanel.paramN)) {
                if (_controlPanel.paramN < 0) _controlPanel.paramN = 0;
                // Clear cached random selection if N changed for random pattern
                if (_controlPanel.selectedPattern == ControlPanelState::PatternType::RandomN &&
                    _controlPanel.paramN != _controlPanel.lastRandomN) {
                    _controlPanel.cachedRandomSelection.clear();
                    _controlPanel.lastRandomN = _controlPanel.paramN;
                }
                changed = true;
            }
            
            if (_controlPanel.selectedPattern == ControlPanelState::PatternType::RangeM_N) {
                if (ImGui::InputInt("M", &_controlPanel.paramM)) {
                    if (_controlPanel.paramM < 0) _controlPanel.paramM = 0;
                    changed = true;
                }
            }
            
            ImGui::Unindent();
        } else {
            _controlPanel.patternSectionOpen = false;
        }
        
        return changed;
    }

    bool MsgEdit::DrawActionExecutionSection(MsgSettings& msgSpec)
    {
        bool changed = false;
        
        // This section is always open (non-collapsible)
        ImGui::Text("Action & Execution");
        ImGui::Separator();
        
        // Live preview
        UpdateLivePreview(); // Ensure it's current
        std::string previewText = "Preview: This pattern will target " + 
                                 std::to_string(_controlPanel.previewCount) + " instances.";
        ImGui::TextWrapped("%s", previewText.c_str());
        
        ImGui::Separator();
        
        // Action configuration
        const char* actionNames[] = { "Set", "Increment", "Decrement" };
        
        // Publishers line
        ImGui::Text("Publishers:");
        ImGui::SameLine();
        int pubAction = (int)_controlPanel.pubAction;
        ImGui::SetNextItemWidth(100);
        if (ImGui::Combo("##pubaction", &pubAction, actionNames, IM_ARRAYSIZE(actionNames))) {
            _controlPanel.pubAction = (ControlPanelState::ActionType)pubAction;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::InputInt("##pubvalue", &_controlPanel.pubValue);
        if (_controlPanel.pubValue < 0) _controlPanel.pubValue = 0;
        
        // Subscribers line  
        ImGui::Text("Subscribers:");
        ImGui::SameLine();
        int subAction = (int)_controlPanel.subAction;
        ImGui::SetNextItemWidth(100);
        if (ImGui::Combo("##subaction", &subAction, actionNames, IM_ARRAYSIZE(actionNames))) {
            _controlPanel.subAction = (ControlPanelState::ActionType)subAction;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::InputInt("##subvalue", &_controlPanel.subValue);
        if (_controlPanel.subValue < 0) _controlPanel.subValue = 0;
        
        ImGui::Separator();
        
        // Execution button - disable by changing style instead of using PushItemFlag
        bool buttonDisabled = (_controlPanel.previewCount == 0);
        if (buttonDisabled) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        
        bool buttonClicked = ImGui::Button("Apply Configuration", ImVec2(-1, 40));
        if (buttonClicked && !buttonDisabled) {
            ApplyPatternToInstances(msgSpec);
            changed = true;
        }
        
        if (buttonDisabled) {
            ImGui::PopStyleVar();
        }
        
        return changed;
    }

    void MsgEdit::UpdateLivePreview()
    {
        _controlPanel.targetedAppIndices.clear();
        _controlPanel.previewCount = 0;
        
        if (_controlPanel.selectedComputers.empty()) {
            return;
        }
        
        if (_controlPanel.selectedScope == ControlPanelState::PatternScope::PerComputer) {
            // Apply pattern per computer
            for (const std::string& computerName : _controlPanel.selectedComputers) {
                std::vector<int> computerAppIndices;
                for (int i = 0; i < (int)_edited.Apps.size(); i++) {
                    if (_edited.Apps[i].ComputerName == computerName) {
                        computerAppIndices.push_back(i);
                    }
                }
                
                auto targetedIndices = ApplyPattern(computerAppIndices);
                _controlPanel.targetedAppIndices.insert(targetedIndices.begin(), targetedIndices.end());
            }
        } else {
            // Global scope: create master list from all targeted computers
            std::vector<int> allTargetedAppIndices;
            for (int i = 0; i < (int)_edited.Apps.size(); i++) {
                if (_controlPanel.selectedComputers.count(_edited.Apps[i].ComputerName) > 0) {
                    allTargetedAppIndices.push_back(i);
                }
            }
            
            auto targetedIndices = ApplyPattern(allTargetedAppIndices);
            _controlPanel.targetedAppIndices.insert(targetedIndices.begin(), targetedIndices.end());
        }
        
        _controlPanel.previewCount = (int)_controlPanel.targetedAppIndices.size();
    }

    std::vector<int> MsgEdit::ApplyPattern(const std::vector<int>& availableIndices)
    {
        std::vector<int> result;
        
        switch (_controlPanel.selectedPattern) {
            case ControlPanelState::PatternType::FirstN: {
                int count = std::min(_controlPanel.paramN, (int)availableIndices.size());
                for (int i = 0; i < count; i++) {
                    result.push_back(availableIndices[i]);
                }
                break;
            }
            
            case ControlPanelState::PatternType::RangeM_N: {
                int start = std::max(0, _controlPanel.paramM - 1); // Convert to 0-based
                int end = std::min(_controlPanel.paramN, (int)availableIndices.size());
                for (int i = start; i < end; i++) {
                    result.push_back(availableIndices[i]);
                }
                break;
            }
            
            case ControlPanelState::PatternType::EveryNth: {
                if (_controlPanel.paramN > 0) {
                    for (int i = 0; i < (int)availableIndices.size(); i += _controlPanel.paramN) {
                        result.push_back(availableIndices[i]);
                    }
                }
                break;
            }
            
            case ControlPanelState::PatternType::RandomN: {
                // Use cached selection if available and still valid
                if (_controlPanel.cachedRandomSelection.empty() || 
                    _controlPanel.lastRandomN != _controlPanel.paramN) {
                    
                    std::vector<int> indices = availableIndices;
                    std::shuffle(indices.begin(), indices.end(), _rng);
                    
                    int count = std::min(_controlPanel.paramN, (int)indices.size());
                    _controlPanel.cachedRandomSelection.assign(indices.begin(), indices.begin() + count);
                    _controlPanel.lastRandomN = _controlPanel.paramN;
                }
                
                // Filter cached selection to only include currently available indices
                for (int idx : _controlPanel.cachedRandomSelection) {
                    if (std::find(availableIndices.begin(), availableIndices.end(), idx) != availableIndices.end()) {
                        result.push_back(idx);
                    }
                }
                break;
            }
        }
        
        return result;
    }

    void MsgEdit::ApplyPatternToInstances(MsgSettings& msgSpec)
    {
        // Ensure vectors are properly sized
        if (msgSpec.PublCnt.size() < _edited.Apps.size()) {
            msgSpec.PublCnt.resize(_edited.Apps.size(), 0);
        }
        if (msgSpec.SubsCnt.size() < _edited.Apps.size()) {
            msgSpec.SubsCnt.resize(_edited.Apps.size(), 0);
        }
        
        // Apply publisher action
        for (int appIndex : _controlPanel.targetedAppIndices) {
            if (appIndex < (int)msgSpec.PublCnt.size()) {
                switch (_controlPanel.pubAction) {
                    case ControlPanelState::ActionType::Set:
                        msgSpec.PublCnt[appIndex] = _controlPanel.pubValue;
                        break;
                    case ControlPanelState::ActionType::Increment:
                        msgSpec.PublCnt[appIndex] += _controlPanel.pubValue;
                        break;
                    case ControlPanelState::ActionType::Decrement:
                        msgSpec.PublCnt[appIndex] = std::max(0, msgSpec.PublCnt[appIndex] - _controlPanel.pubValue);
                        break;
                }
            }
        }
        
        // Apply subscriber action
        for (int appIndex : _controlPanel.targetedAppIndices) {
            if (appIndex < (int)msgSpec.SubsCnt.size()) {
                switch (_controlPanel.subAction) {
                    case ControlPanelState::ActionType::Set:
                        msgSpec.SubsCnt[appIndex] = _controlPanel.subValue;
                        break;
                    case ControlPanelState::ActionType::Increment:
                        msgSpec.SubsCnt[appIndex] += _controlPanel.subValue;
                        break;
                    case ControlPanelState::ActionType::Decrement:
                        msgSpec.SubsCnt[appIndex] = std::max(0, msgSpec.SubsCnt[appIndex] - _controlPanel.subValue);
                        break;
                }
            }
        }
    }

    void MsgEdit::DrawAllSettings()
    {
        bool changed = false;

        std::vector<std::string> toRemove;
        // show list of message types
        for (auto& msgKV : _edited.Msgs)
        {
            auto& msg = msgKV.second;
            ImGui::PushID(&msg);
            bool wantRemove = false;;
            changed |= DrawMsgSettings(msg, wantRemove);
            if (wantRemove) toRemove.push_back(msg.Name);
            ImGui::PopID();
        }

        if (ImGui::Button("+"))
        {
            _selectingNewMsg = true;
        }

        if (_selectingNewMsg)
        {
            // imgui combo containing a list of msgdefs
            auto& msgDefs = _app->GetMsgDefs();
            if (ImGui::BeginCombo("MsgDefs", "Select"))
            {
                for (int i = 0; i < (int)msgDefs.size(); i++)
                {
                    auto& msgDef = msgDefs[i];

                    // skip those already used
                    if (_edited.Msgs.find(msgDef.Name) != _edited.Msgs.end())
                    {
                        continue;
                    }

                    if (ImGui::Selectable(msgDef.Name.c_str()))
                    {
                        _selectingNewMsg = false;
                        NewMsgSpec(msgDef.Name.c_str());
                        changed = true;
                    }
                }
                ImGui::EndCombo();
            }

        }

        if (!toRemove.empty())
        {
            changed = true;

            // remove from settings
            for (auto& name : toRemove)
            {
                _edited.Msgs.erase(name);
            }
        }

        if (changed) // everyone can change settings at runtime (last one wins)
        {
            //printf("SharedData changed\n");
            OnChanged();
        }

    }

}