#include "stdafx.h"
#include "MsgController.h"
#include "imgui.h"
#include "App.h"
#include "AllMsgsCtrl.h"
#include <string>
#include "Publisher.h"
#include "Subscriber.h"

namespace DdsPerfTest
{

MsgController::MsgController( App* app, std::string msgClass, int domainId )
{
    _app = app;
    _msgClass = msgClass;
    _domainId = domainId;
    printf("MsgController::MsgController(%s on domain %d)\n", msgClass.c_str(), domainId);
}

MsgController::~MsgController()
{
	printf("MsgController::~MsgController(%s)\n", _msgClass.c_str());
}

void MsgController::Tick()
{
	// tick publishers
	for( auto& publ : _publishers )
	{
		publ->Tick();
	}

	// tick subscribers
	for( auto& subs : _subscribers )
	{
		subs->Tick();
	}
}

void MsgController::DrawUI()
{
    ImGui::Begin("MsgClassCtrl");
    ImGui::Button("Hi!");
    ImGui::End();

}

void MsgController::UpdateSettings( const MsgSettings& msgSpec, bool allDisabled)
{
	int appIndex = _app->GetAppIndex();
	if( appIndex < 0 ) return; // no apps found yet

	// adjust number of publishers
	int numPublThisApp = appIndex < (int)msgSpec.PublCnt.size() ? msgSpec.PublCnt[appIndex] : 0;
	if( allDisabled || msgSpec.Disabled || msgSpec.AllPublDisabled )
	{
		numPublThisApp = 0;
	}

	if(numPublThisApp > (int)_publishers.size() )
	{
        for( int i = (int)_publishers.size(); i < numPublThisApp; i++ )
	    {
		    _publishers.push_back(std::make_shared<Publisher>(_app, _msgClass, i, _domainId, msgSpec.PartitionName));
	    }
    }
    else if(numPublThisApp < (int)_publishers.size() )
    {
		// destroy publishers
		for( int i = (int)_publishers.size() - 1; i >= numPublThisApp; i-- )
	    {
		    _publishers.pop_back();
	    }
	}

	// update the settings of existing publishers
	for( int i = 0; i < numPublThisApp; i++ )
	{
		_publishers[i]->UpdateSettings(msgSpec);
	}

    // adjust number of subscribers
	int numSubsThisApp = appIndex < (int)msgSpec.SubsCnt.size() ? msgSpec.SubsCnt[appIndex] : 0;
	if (allDisabled || msgSpec.Disabled || msgSpec.AllSubsDisabled)
	{
		numSubsThisApp = 0;
	}

    if(numSubsThisApp > (int)_subscribers.size() )
    {
		for( int i = (int)_subscribers.size(); i < numSubsThisApp; i++ )
	    {
		    _subscribers.push_back(std::make_shared<Subscriber>(_app, _msgClass, i, _domainId, msgSpec.PartitionName));
	    }
	}
	else if(numSubsThisApp < (int)_subscribers.size() )
	{
		// destroy subscribers
		for( int i = (int)_subscribers.size() - 1; i >= numSubsThisApp; i-- )
	    {
		    _subscribers.pop_back();
	    }
	}
    
	// update the settings of existing subscribers
	for( int i = 0; i < numSubsThisApp; i++ )
	{
		_subscribers[i]->UpdateSettings(msgSpec);;
	}

    // remember last settings
    _spec = msgSpec;
}





}