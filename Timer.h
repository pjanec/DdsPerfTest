#pragma once

#include <functional>
#include <chrono>

namespace DdsPerfTest
{

// calls given callback every given period
class Timer
{
	std::function<void()> _callback;
	long long _periodUsecs;
	std::chrono::time_point<std::chrono::high_resolution_clock> _start;
public:
	Timer(std::function<void()> callback, int periodUsecs)
	{
		_callback = callback;
		_periodUsecs = periodUsecs;
		Reset();
	}

	void Tick()
	{
		// if period is negative, do nothing
		if (_periodUsecs < 0)
			return;

		// if period is zero, call callback every tick
		if( _periodUsecs == 0 )
		{
			_callback();
			return;
		}

		// if elapsed time is greater than period, call callback
		// and reset timer
		if (GetElapsedMicroseconds() > _periodUsecs)
		{
			_callback();
			Reset();
		}
	}

	long long GetElapsedMicroseconds()
	{
		auto now = std::chrono::high_resolution_clock::now();
		auto elapsed = now - _start;
		return std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

	}

	void Reset()
	{
		_start = std::chrono::high_resolution_clock::now();
	}

	void SetPeriod( int periodUsecs )
	{
		_periodUsecs = periodUsecs;
	}

};

}
