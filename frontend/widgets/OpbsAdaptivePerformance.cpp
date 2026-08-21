/******************************************************************************
    Copyright (C) 2026

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
******************************************************************************/

#include "OpbsAdaptivePerformance.hpp"

#include <util/platform.h>

#include <algorithm>
#include <cmath>

OpbsAdaptiveHostProfile OpbsAdaptivePerformanceController::DetectHostProfile()
{
	OpbsAdaptiveHostProfile profile;
	profile.totalMemoryBytes = os_get_sys_total_size();
	profile.logicalCores = std::max(1, os_get_logical_cores());
	const uint64_t gib = profile.totalMemoryBytes / (1024ULL * 1024ULL * 1024ULL);

	if (gib <= 4 || profile.logicalCores <= 2) {
		profile.tier = OpbsAdaptiveHostProfile::Tier::VeryLow;
		profile.outputWidth = 854;
		profile.outputHeight = 480;
		profile.fps = 30;
		profile.recommendedBitrate = 1800;
		profile.thumbnailConcurrency = 1;
		profile.idleUiIntervalMs = 500;
	} else if (gib <= 8 || profile.logicalCores <= 4) {
		profile.tier = OpbsAdaptiveHostProfile::Tier::Low;
		profile.outputWidth = 1280;
		profile.outputHeight = 720;
		profile.fps = 30;
		profile.recommendedBitrate = 4000;
		profile.thumbnailConcurrency = 1;
		profile.idleUiIntervalMs = 350;
	} else if (gib < 16 || profile.logicalCores < 8) {
		profile.tier = OpbsAdaptiveHostProfile::Tier::Balanced;
		profile.outputWidth = 1920;
		profile.outputHeight = 1080;
		profile.fps = 30;
		profile.recommendedBitrate = 6000;
		profile.thumbnailConcurrency = 2;
		profile.idleUiIntervalMs = 250;
	} else {
		profile.tier = OpbsAdaptiveHostProfile::Tier::High;
		profile.outputWidth = 1920;
		profile.outputHeight = 1080;
		profile.fps = 60;
		profile.recommendedBitrate = 6000;
		profile.thumbnailConcurrency = 4;
		profile.idleUiIntervalMs = 200;
	}

	return profile;
}

const char *OpbsAdaptivePerformanceController::TierName(OpbsAdaptiveHostProfile::Tier tier)
{
	switch (tier) {
	case OpbsAdaptiveHostProfile::Tier::VeryLow:
		return "very-low";
	case OpbsAdaptiveHostProfile::Tier::Low:
		return "low";
	case OpbsAdaptiveHostProfile::Tier::Balanced:
		return "balanced";
	case OpbsAdaptiveHostProfile::Tier::High:
		return "high";
	}
	return "balanced";
}

void OpbsAdaptivePerformanceController::Reset(int maximum, int minimum)
{
	maximumBitrate = std::max(1000, maximum);
	minimumBitrate = std::clamp(minimum, 1000, maximumBitrate);
	currentBitrate = maximumBitrate;
	networkPressureSamples = 0;
	networkHealthySamples = 0;
	hostPressureSamples = 0;
	hostHealthySamples = 0;
	bitrateCooldownSamples = 0;
}

OpbsAdaptivePerformanceController::Decision OpbsAdaptivePerformanceController::Update(const Sample &sample)
{
	Decision decision;
	decision.bitrate = currentBitrate;

	const bool hostPressure = sample.cpuPercent >= 85.0 || sample.memoryPressure >= 0.92 ||
				  sample.renderUtilization >= 0.90 || sample.renderLagRatio >= 0.02 ||
				  sample.encodingLagRatio >= 0.02;
	if (hostPressure) {
		++hostPressureSamples;
		hostHealthySamples = 0;
	} else {
		hostPressureSamples = 0;
		++hostHealthySamples;
	}
	if (!constrainedMode && hostPressureSamples >= 3) {
		constrainedMode = true;
		decision.constrainedModeChanged = true;
	} else if (constrainedMode && hostHealthySamples >= 15) {
		constrainedMode = false;
		decision.constrainedModeChanged = true;
	}
	decision.constrainedMode = constrainedMode;

	if (!sample.streaming) {
		networkPressureSamples = 0;
		networkHealthySamples = 0;
		return decision;
	}

	if (bitrateCooldownSamples > 0)
		--bitrateCooldownSamples;
	const bool severe = sample.worstCongestion >= 0.35 || sample.worstDroppedRatio >= 0.02;
	const bool networkPressure = severe || sample.worstCongestion >= 0.15 || sample.worstDroppedRatio >= 0.005;
	const bool networkHealthy = sample.worstCongestion < 0.05 && sample.worstDroppedRatio < 0.001;
	decision.severeNetworkPressure = severe;

	if (networkPressure) {
		++networkPressureSamples;
		networkHealthySamples = 0;
	} else if (networkHealthy) {
		networkPressureSamples = 0;
		++networkHealthySamples;
	} else {
		networkPressureSamples = std::max(0, networkPressureSamples - 1);
		networkHealthySamples = 0;
	}

	const int pressureThreshold = severe ? 2 : 3;
	if (bitrateCooldownSamples == 0 && networkPressureSamples >= pressureThreshold &&
	    currentBitrate > minimumBitrate) {
		const double factor = severe ? 0.78 : 0.85;
		const int next = std::max(minimumBitrate, int(std::floor(currentBitrate * factor / 50.0)) * 50);
		if (next < currentBitrate) {
			currentBitrate = next;
			decision.bitrateChanged = true;
			decision.bitrate = currentBitrate;
			bitrateCooldownSamples = 8;
		}
		networkPressureSamples = 0;
	} else if (bitrateCooldownSamples == 0 && networkHealthySamples >= 25 &&
		   currentBitrate < maximumBitrate) {
		const int next = std::min(maximumBitrate, int(std::ceil(currentBitrate * 1.06 / 50.0)) * 50);
		if (next > currentBitrate) {
			currentBitrate = next;
			decision.bitrateChanged = true;
			decision.bitrate = currentBitrate;
			bitrateCooldownSamples = 10;
		}
		networkHealthySamples = 0;
	}

	return decision;
}
