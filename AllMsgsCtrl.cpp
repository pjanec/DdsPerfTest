#include "stdafx.h"
#include <stdio.h>
#include <chrono>
#include <thread>
#include "AllMsgsCtrl.h"
#include "App.h"
#include "imgui.h"
#include "dds/dds.h"
#include "MsgController.h"
#include "TopicRW.h"

namespace DdsPerfTest
{

    // implement app here
    AllMsgsCtrl::AllMsgsCtrl( App* app )
    {
        _app = app;
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
        // pass new settings to each msgclass-specific controller
        // create new msgclass-specific controllers if necessary

        std::vector<std::string> newCtrlNames;

        for( auto& msgKV : sample.Msgs )
        {
            auto& msg = msgKV.second;
            //printf("%d,%s,%d,%d,%d,%d,%d,%d\n", msg.Disabled, msg.Name.c_str(), msg.Rate, msg.Size, msg.PublCnt, msg.PublAppMask, msg.SubsCnt, msg.SubsAppMask);

            newCtrlNames.push_back(msg.Name);

            auto it = _msgControllers.find(msg.Name);
            if (it == _msgControllers.end())
			{
				// create new controller
				auto controller = std::make_shared<MsgController>( _app, msg.Name );
                controller->UpdateSettings( msg, sample.Disabled );
				_msgControllers[msg.Name] = controller;
			}
			else
			{
				// update existing controller
				it->second->UpdateSettings(msg, sample.Disabled);
			}
        }

        // remove controllers no longer part of the settings
        // (thise still present in our cached controller but no longer part of the settings)
        for( auto it = _msgControllers.begin(); it != _msgControllers.end(); )
		{
			auto& msgName = it->first;
			auto& controller = it->second;
            if( std::find(newCtrlNames.begin(), newCtrlNames.end(), msgName) == newCtrlNames.end() )
			{
				// controller no longer part of the settings
				// remove it
				it = _msgControllers.erase(it);
			}
			else
			{
				++it;
			}
		}

    }




};