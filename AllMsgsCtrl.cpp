#include "stdafx.h"
#include <stdio.h>
#include <chrono>
#include <thread>
#include <set>
#include "AllMsgsCtrl.h"
#include "App.h"
#include "imgui.h"
#include "dds/dds.h"
#include "MsgController.h"
#include "TopicRW.h"
#include "ParticMgr.h"

namespace DdsPerfTest
{

    AllMsgsCtrl::AllMsgsCtrl( App* app, std::shared_ptr<ParticMgr> particMgr )
    {
        _app = app;
        _particMgr = particMgr;
    }

    AllMsgsCtrl::~AllMsgsCtrl()
    {
    }

    void AllMsgsCtrl::Tick()
    {

        // tick msg controllers
        for( auto& kv : _msgControllers )
        {
            kv.second->Tick();
        }
    }

    void AllMsgsCtrl::DrawUI()
    {
        DrawLocalSettings();
    }

    void AllMsgsCtrl::DrawLocalSettings()
    {
        // table of settings for this app
        if( ImGui::BeginTable("Local Settings", 6) )
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Enabled");
            ImGui::TableSetupColumn("Rate");
            ImGui::TableSetupColumn("Size");
            ImGui::TableSetupColumn("PublCnt");
            ImGui::TableSetupColumn("SubsCnt");
            ImGui::TableHeadersRow();
            for( auto& kv : _msgControllers )
		    {
			    auto& msgCtrl = kv.second;

                auto& spec = msgCtrl->GetSpec();

                ImGui::TableNextColumn();
                ImGui::Text(spec.Name.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%d", spec.Disabled ? 0:1);

                ImGui::TableNextColumn();
                ImGui::Text( "%d", spec.Rate );

                ImGui::TableNextColumn();
                ImGui::Text("%d", spec.Size);

                ImGui::TableNextColumn();
                int publCnt = 0;
                for(int i=0; i<(int)_app->GetApps().size(); i++)
				{ // sum per each app
                    if( i < (int)spec.PublCnt.size() )
					    publCnt += spec.PublCnt[i];
				}
                ImGui::Text("%d", publCnt);

                ImGui::TableNextColumn();
                int subsCnt = 0;
                for (int i = 0; i < (int)_app->GetApps().size(); i++)
                { // sum per each app
                    if (i < (int)spec.SubsCnt.size())
                           subsCnt += spec.SubsCnt[i];
                }

                ImGui::Text("%d", subsCnt);
		    }
            ImGui::EndTable();
		}
    }

    void AllMsgsCtrl::Set( const SharedData& sample )
    {
        auto& msgDefs = _app->GetMsgDefs();

        std::set<std::string> newMsgNames;
        std::vector<std::string> unknownMessages;

        for (const auto& msgKV : sample.Msgs)
        {
            newMsgNames.insert(msgKV.first);
        }

        for (const auto& msgKV : sample.Msgs)
        {
            const auto& msg = msgKV.second;
            
            auto msgDefIt = std::find_if(msgDefs.begin(), msgDefs.end(), 
                [&msg](const MsgDef& msgDef) { return msgDef.Name == msg.Name; });
            
            if (msgDefIt == msgDefs.end())
            {
                unknownMessages.push_back(msg.Name);
                continue;
            }

            auto it = _msgControllers.find(msg.Name);
            if (it == _msgControllers.end())
            {
                printf("Creating new MsgController for '%s' on Domain %d\n", msg.Name.c_str(), msg.DomainId);
                auto controller = std::make_shared<MsgController>(_app, msg.Name, msg.DomainId);
                controller->UpdateSettings(msg, sample.Disabled);
                _msgControllers[msg.Name] = controller;
            }
            else
            {
                if (it->second->GetSpec() != msg)
                {
                    printf("Recreating MsgController for '%s' due to settings change.\n", msg.Name.c_str());
                    it->second.reset();
                    auto newController = std::make_shared<MsgController>(_app, msg.Name, msg.DomainId);
                    newController->UpdateSettings(msg, sample.Disabled);
                    _msgControllers[msg.Name] = newController;
                }
                else
                {
                    it->second->UpdateSettings(msg, sample.Disabled);
                }
            }
        }

        for (auto it = _msgControllers.begin(); it != _msgControllers.end(); )
        {
            if (newMsgNames.find(it->first) == newMsgNames.end())
            {
                printf("Removing obsolete MsgController for '%s'\n", it->first.c_str());
                it = _msgControllers.erase(it);
            }
            else
            {
                ++it;
            }
        }

        std::set<dds_domainid_t> activeDomains;
        for (const auto& kv : _msgControllers)
        {
            activeDomains.insert(kv.second->GetSpec().DomainId);
        }

        if (_particMgr)
        {
            _particMgr->CleanupUnusedDomains(activeDomains);
        }
        
        if (!unknownMessages.empty())
        {
            for (const auto& msgName : unknownMessages)
            {
                fprintf(stderr, "WARNING: Unknown message type '%s' - skipped\n", msgName.c_str());
            }
        }
    }




};