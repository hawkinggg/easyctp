#pragma once
#include <thread>
#include <string>
#include <ctime>

void getNowString()
{
	std::time_t raw_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	tm tm1 = *std::localtime(&raw_time);
}

