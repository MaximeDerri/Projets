#pragma once

#include "tree.h"

//generic declaration
class job {
public:
	virtual void run () = 0;
	virtual ~job() {};
};