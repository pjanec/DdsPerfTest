#include "stdafx.h"
#include "MsgEdit.h"
#include "App.h"
#include "imgui.h"

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
        
        // Use fixed ID based on message name to prevent header from closing when totals change
        ImGui::PushID(("PubSubTable_" + msgSpec.Name).c_str());
        
        // Header with totals - use ## to hide the ID part from display
        std::string headerText = "Publishers & Subscribers (Pubs: " + std::to_string(totalPubs) + 
                               ", Subs: " + std::to_string(totalSubs) + ")##" + msgSpec.Name;
        
        if (ImGui::CollapsingHeader(headerText.c_str())) {
            ImGui::Indent();
            
            // Quick actions with input fields for custom values
            ImGui::Text("Quick Actions:");
            
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
                
                // Table rows
                for (int i = 0; i < numApps; i++) {
                    bool haveApp = i < (int)_edited.Apps.size();
                    
                    ImGui::TableNextRow();
                    ImGui::PushID(i);
                    
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
        
        ImGui::PopID(); // Pop the fixed ID for this message type

        return changed;
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