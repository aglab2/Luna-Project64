#include "stdafx.h"
#include "InputDelayer.h"

#include <algorithm>

#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/Plugins/ControllerPlugin.h>

#define MINIMAL_DURATION_MS 5
#define MINIMAL_ERROR_MS 3

bool InputDelayer::SampleInputLessThan::operator() (const SampledInput& left, const SampledInput& right)
{
	return left.time < right.time;
}
bool InputDelayer::SampleInputLessThan::operator() (const SampledInput& left, InputDelayer::TimePoint right)
{
	return left.time < right;
}
bool InputDelayer::SampleInputLessThan::operator() (InputDelayer::TimePoint left, const SampledInput& right)
{
	return left < right.time;
}

InputDelayer::InputDelayer()
{
	std::lock_guard<std::mutex> lck1(m_StartStopMutex);
	std::lock_guard<std::mutex> lck2(m_Mutex);
	if (m_Running)
		return;

	m_SampledInputs = {};
	m_DelayedKeysLastRequestTime = Clock::now();
	m_DelayedKeysRequestDurations = {};
	m_Running = true;
	m_ExpectedWakeUpTime = m_ExpectedPollingTime = m_DelayedKeysLastRequestTime + estimateFrameTime();
	m_Worker = std::thread{ &InputDelayer::work, this };
}

InputDelayer::~InputDelayer()
{
	std::lock_guard<std::mutex> lck1(m_StartStopMutex);
	if (!m_Worker.joinable())
		return;

	{
		std::lock_guard<std::mutex> lck2(m_Mutex);
		m_Running = false;
	}
	m_CV.notify_one();
	m_Worker.join();
}

void InputDelayer::work()
{
	std::unique_lock<std::mutex> lck(m_Mutex);

	// I have no clue what this does or if it is needed but I am sure it is of critical importance
	auto ctl = g_Plugins->Control();
	auto controls = ctl->PluginControllers();

	if (controls->Present && controls->RawData)
	{
		if (auto fn = ctl->ControllerCommand)
		{
			BYTE cmd[] = { 1, 3, 0, 255, 255, 255, 255, 255 };
			fn(0, cmd);
			fn(-1, nullptr);
		}
		if (auto fn = ctl->ReadController)
		{
			BYTE cmd[] = { 1, 3, 0, 255, 255, 255, 255, 255 };
			fn(0, cmd);
			fn(-1, nullptr);
		}
		if (auto fn = ctl->ControllerCommand)
		{
			BYTE cmd[] = { 1, 4, 1, 255, 255, 255, 255, 255 };
			fn(0, cmd);
			fn(-1, nullptr);
		}
		if (auto fn = ctl->ReadController)
		{
			BYTE cmd[] = { 1, 4, 1, 255, 255, 255, 255, 255 };
			fn(0, cmd);
			fn(-1, nullptr);
		}
	}

	while (m_Running)
	{
		auto res = m_CV.wait_until(lck, m_ExpectedWakeUpTime);
		if (res == std::cv_status::no_timeout)
		{
			// woken up
			if (m_ExpectedPollingTime < m_ExpectedWakeUpTime)
			{
				auto diff = m_ExpectedWakeUpTime - m_ExpectedPollingTime;
				if (diff > std::chrono::milliseconds(MINIMAL_ERROR_MS))
				{
					m_ExpectedWakeUpTime = m_ExpectedPollingTime;
					continue;
				}
			}
		}

		auto now = Clock::now();
		BUTTONS keys{};
		// can technically unlock mutex for GetKeys but should not matter much

		if (controls->Present && controls->RawData)
		{
			if (auto fn = ctl->ReadController)
			{
				BYTE pifRam[] = { 1, 4, 1, 0, 0, 255, 0, 255 };
				fn(0, pifRam);
				fn(-1, nullptr);
				keys.Value = *(DWORD*)&pifRam[3];
			}
		}
		else
		{
			if (auto fn = ctl->GetKeys) fn(0, &keys);
		}

		m_SampledInputs.push({ now, keys.Value });

		if (m_ExpectedWakeUpTime < now + std::chrono::milliseconds(MINIMAL_ERROR_MS))
		{
			m_ExpectedWakeUpTime = m_ExpectedPollingTime = now + estimateFrameTime();
		}
		else
		{
			m_ExpectedWakeUpTime = m_ExpectedPollingTime;
		}
	}
}

InputDelayer::Duration InputDelayer::absDiff(TimePoint t0, TimePoint t1)
{
	if (t0 > t1)
	{
		return t0 - t1;
	}
	else
	{
		return t1 - t0;
	}
}

DWORD InputDelayer::findBestKeys(TimePoint now)
{
	if (m_SampledInputs.empty())
		return 0;

	auto it1 = std::lower_bound(m_SampledInputs.begin(), m_SampledInputs.end(), now, SampleInputLessThan{});
	if (it1 == m_SampledInputs.begin())
	{
#ifdef DEBUG_RECORD_ERROR_DURATIONS
		recordedErrorDurations_.push(absDiff(now, it1->time));
#endif
		return it1->key;
	}

	auto it2 = it1 - 1;
	if (it1 == m_SampledInputs.end())
	{
#ifdef DEBUG_RECORD_ERROR_DURATIONS
		recordedErrorDurations_.push(absDiff(now, it2->time));
#endif
		return it2->key;
	}

	auto d1 = absDiff(it1->time, now);
	auto d2 = absDiff(it2->time, now);
	if (d2 > d1)
	{
#ifdef DEBUG_RECORD_ERROR_DURATIONS
		recordedErrorDurations_.push(d1);
#endif
		return it1->key;
	}
	else
	{
#ifdef DEBUG_RECORD_ERROR_DURATIONS
		recordedErrorDurations_.push(d2);
#endif
		return it2->key;
	}
}

InputDelayer::Duration InputDelayer::estimateFrameTime()
{
	if (m_DelayedKeysRequestDurations.empty())
	{
		return std::chrono::duration_cast<Duration>(std::chrono::milliseconds(1000 / 30));
	}

	// copy
	std::vector<Duration> buf;
	if (m_DelayedKeysRequestDurations.full())
	{
		buf = m_DelayedKeysRequestDurations.buffer();
	}
	else
	{
		const Duration* data = m_DelayedKeysRequestDurations.buffer().data();
		buf = { data, data + m_DelayedKeysRequestDurations.head() };
	}

	auto m = buf.begin() + buf.size() / 2;
	std::nth_element(buf.begin(), m, buf.end());
	return buf[buf.size() / 2];
}

DWORD InputDelayer::getDelayedKeys()
{
	std::lock_guard<std::mutex> lck(m_Mutex);
	auto now = Clock::now();
	int inputDelay = g_System->InputDelay();
	if (inputDelay < MINIMAL_ERROR_MS)
		inputDelay = MINIMAL_ERROR_MS;

	auto keys = findBestKeys(now - std::chrono::milliseconds(inputDelay));
	auto duration = now - m_DelayedKeysLastRequestTime;
	if (duration < std::chrono::milliseconds(MINIMAL_DURATION_MS))
		return keys;

	m_DelayedKeysRequestDurations.push(duration);
	m_DelayedKeysLastRequestTime = now;

	Duration frameTime = estimateFrameTime();
	Duration delay = std::chrono::duration_cast<Duration>(std::chrono::milliseconds(inputDelay));
	// we want to estimate when next controller input needs to be polled - it is likely to be polled within a 'frameTime'
	// from now which means 'delay' should not be larger than 'frameTime'
	// Such approach may still add jittering but CircularBuffer polling should help us out here...
	delay %= frameTime;

	m_ExpectedPollingTime = delay + now;
	if (m_ExpectedPollingTime < m_ExpectedWakeUpTime)
	{
		auto diff = m_ExpectedWakeUpTime - m_ExpectedPollingTime;
		if (diff > std::chrono::milliseconds(MINIMAL_ERROR_MS))
		{
			m_CV.notify_one();
		}
	}
	return keys;
}
