#pragma once

class FVertex {
public:
	FVertex();
	FVertex(float x, float y);
	~FVertex();

	//! \todo Math operators (need at least scaling)

	float x, y;
};

