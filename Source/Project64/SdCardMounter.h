#pragma once

namespace SdCardMounter
{
	enum State
	{
		ISO,
		VHD,
	};

	void switchStates();
	State getState();
}
