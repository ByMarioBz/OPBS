/******************************************************************************
    Copyright (C) 2026

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
******************************************************************************/

#pragma once

#include <cstdint>

struct OpbsAdaptiveHostProfile {
	enum class Tier { VeryLow, Low, Balanced, High };

	Tier tier = Tier::Balanced;
	uint32_t outputWidth = 1920;
	uint32_t outputHeight = 1080;
	uint32_t fps = 30;
	int recommendedBitrate = 6000;
	int thumbnailConcurrency = 2;
	int idleUiIntervalMs = 250;
	uint64_t totalMemoryBytes = 0;
	int logicalCores = 0;
};

class OpbsAdaptivePerformanceController {
public:
	struct Sample {
		bool streaming = false;
		double cpuPercent = 0.0;
		double memoryPressure = 0.0;
		double renderUtilization = 0.0;
		double renderLagRatio = 0.0;
		double encodingLagRatio = 0.0;
		double worstCongestion = 0.0;
		double worstDroppedRatio = 0.0;
	};

	struct Decision {
		bool constrainedModeChanged = false;
		bool constrainedMode = false;
		bool bitrateChanged = false;
		int bitrate = 0;
		bool severeNetworkPressure = false;
	};

	static OpbsAdaptiveHostProfile DetectHostProfile();
	static const char *TierName(OpbsAdaptiveHostProfile::Tier tier);

	void Reset(int maximumBitrate, int minimumBitrate);
	Decision Update(const Sample &sample);
	int CurrentBitrate() const { return currentBitrate; }
	bool ConstrainedMode() const { return constrainedMode; }

private:
	int maximumBitrate = 6000;
	int minimumBitrate = 1800;
	int currentBitrate = 6000;
	int networkPressureSamples = 0;
	int networkHealthySamples = 0;
	int hostPressureSamples = 0;
	int hostHealthySamples = 0;
	int bitrateCooldownSamples = 0;
	bool constrainedMode = false;
};
