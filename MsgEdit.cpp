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

            changed |= DrawPerAppCnt("Pubs##Publ", msgSpec.PublCnt, msgSpec.AllPublDisabled);

            changed |= DrawPerAppCnt("Subs##Subscr", msgSpec.SubsCnt, msgSpec.AllSubsDisabled);

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
        bool changed = false;

        if (perAppCnts.size() < _edited.Apps.size()) // adjust size to app count
            perAppCnts.resize(_edited.Apps.size(), 0);

        ImGui::PushID(name);

        // individual app checkboxes
        ImGui::Indent();
        for (int i = 0; i < (int)perAppCnts.size(); i++)
        {
            bool haveApp = i < (int)_edited.Apps.size();

            ImGui::PushID(i);
            // number per app
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 20);
            char buf[100]; sprintf(buf, "%d", perAppCnts[i]);
            if (!haveApp)
            {
                // set style to gray
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5, 0.5, 0.5, 1));
            }

            if (ImGui::InputText("", buf, 100))
            {
                int val = atoi(buf);
                if (val < 0) val = 0;
                perAppCnts[i] = val;
                changed = true;
            }

            if (!haveApp)
            {
                ImGui::PopStyleColor();
            }

            // tooltip over the app-specific widget
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                if (haveApp)
                    ImGui::Text("%s %d", _edited.Apps[i].ComputerName.c_str(), _edited.Apps[i].ProcessId);
                else
                    ImGui::Text("[not enough apps running]");
                ImGui::EndTooltip();
            }

            ImGui::PopID();

            if (i < (int)perAppCnts.size() - 1)
                ImGui::SameLine();
        }

        // title of the group
        ImGui::SameLine();

        std::string strippedName = name;
        strippedName = strippedName.substr(0, strippedName.find("##"));
        ImGui::Text(strippedName.c_str());

        // checkbox that toggles all at once
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);

        if (ImGui::Checkbox("Disable", &allDisabled))
        {
            changed = true;
        }


        ImGui::Unindent();

        ImGui::PopID();

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