#pragma once

#include "../../Precompiled.h"

class COverlay {
public:
	void Watermark();
	void SpectatorsList();
	void KeybindList();
	void Indicators();
};

const auto g_Overlay = std::make_unique<COverlay>();