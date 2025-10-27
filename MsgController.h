#pragma once
#include "dds/dds.h"
#include "SharedData.h"
#include <string>
#include <memory>


namespace DdsPerfTest
{

class App;
class Publisher;
class Subscriber;

// Manages local publishers and subscribers according to the current settings for given message class.
// If more publishers are requested for this machine, each is created with different participant to simulate multiple apps on machine.
//   for publisher #0 a participant #0 is used
//   for publisher #1 a participant #1 is used
//   etc.
//   for sunscriber #0 a participant #0 is used
//   for subscriber #1 a participant #1 is used
//   etc.
class MsgController
{
protected:
	App* _app;
	std::string _msgClass;
	int _domainId;  // DDS domain ID for this message class
	MsgSettings _spec;
	std::vector<std::shared_ptr<Publisher>> _publishers;
	std::vector<std::shared_ptr<Subscriber>> _subscribers;

public:
	MsgController( App* app, std::string msgClass, int domainId);
	~MsgController();
	const MsgSettings& GetSpec() const { return _spec; }

	void Tick();
	void DrawUI();
	void UpdateSettings( const MsgSettings &msgSpec, bool allDisabled );

	int GetCntForThisApp( int total, int appMask ) const;
};

}