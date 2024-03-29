﻿#pragma once
#include "Platform.h"

class Mutex
{
public:
	Mutex();
	~Mutex();

	Mutex(const Mutex&)		= delete;
	Mutex(Mutex&&) noexcept = delete;
	Mutex& operator=(const Mutex&) = delete;
	Mutex& operator=(Mutex&&) noexcept = delete;

	void Enter();

	void Leave();

	bool TryEnter();

private:
	Kaguya::Windows::CRITICAL_SECTION Handle;
};
