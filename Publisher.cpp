#include "stdafx.h"
#include "Publisher.h"
#include "imgui.h"
#include "App.h"
#include "Timer.h"
#include "TopicRW.h"
#include "MsgDef.h"
#include "NetworkDefs.h"

namespace DdsPerfTest
{

    Publisher::Publisher( App* app, std::string msgClass, int index )
    {
        _app = app;
		_msgClass = msgClass;
		_index = index;
		printf("Publisher::Publisher(%s, %d)\n", msgClass.c_str(), index);

        _participant = _app->GetParticipant( index );

        auto& msgDefs = _app->GetMsgDefs();

        // find msg by name
        auto msgDefIt = std::find_if(msgDefs.begin(), msgDefs.end(), [msgClass](const MsgDef& msgDef) { return msgDef.Name == msgClass; });
        if (msgDefIt == msgDefs.end())
			DDS_FATAL("msgDef %s not found\n", msgClass.c_str());
        auto& msgDef = *msgDefIt;

		// create topic
        _topicRW = std::make_shared<TopicRW>(
			_participant,
			msgClass.c_str(),
			&Net_TestMsg_desc,
            msgDef.Reliability,
            msgDef.Durability,
            msgDef.History,
            msgDef.HistoryDepth,
			false,
			true);


        _sendTimer = std::make_shared<Timer>([this]() -> void { this->SendMsg(); }, -1);
    }

    Publisher::~Publisher()
    {
        printf("Publisher::~Publisher(%s, %d)\n", _msgClass.c_str(), _index);
    }

    void Publisher::Tick()
    {
        // send msg of given size at given rate
        _sendTimer->Tick();
    }

    void Publisher::DrawUI()
    {
        ImGui::Begin("Publisher");
        ImGui::Button("Hi!");
        ImGui::End();

    }

    void Publisher::UpdateSettings(const MsgSettings& spec)
    {
        if( spec.Rate <= 0 )
			_sendTimer->SetPeriod( -1 ); // no sending
		else
            _sendTimer->SetPeriod( 1000000 / spec.Rate );

        _msgSpec = spec;
    }

    void Publisher::SendMsg()
    {
        // create msg
		Net_TestMsg msg = {0};
		msg.AppId = _app->GetAppIndex();
        msg.InAppIndex = _index;
        msg.SeqNum = _seqNum++;
        if( _msgSpec.Size > 0 )
		{
			msg.Payload._length = _msgSpec.Size;
			msg.Payload._maximum = _msgSpec.Size;
            msg.Payload._buffer = (uint8_t*) dds_alloc( _msgSpec.Size );
            msg.Payload._release = false;
			for( int i = 0; i < _msgSpec.Size; ++i )
			{
				msg.Payload._buffer[i] = i % 256;
			}
		}

		// send msg
        auto rc = dds_write( _topicRW->GetWriter(), &msg);
        if( rc < 0 )
			printf("dds_write(%s, %d) failed: %d\n", _msgSpec.Name.c_str(), _msgSpec.Size, rc);

        dds_free(msg.Payload._buffer);
    }
}