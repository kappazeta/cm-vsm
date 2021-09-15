//! @file
//! @brief Geometry classes for vector layers
//
// Copyright 2020 KappaZeta Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once


/**
 * A floating-point vertex class
 */
class FVertex {
public:
	/**
	 * Initialize a vertex \f$(0, 0)\f$.
	 */
	FVertex();

	/**
	 * Initialize a vertex with the given coordinates.
	 */
	FVertex(float x, float y);

	/**
	 * De-initialize the vertex.
	 */
	~FVertex();

	//! \todo Math operators (need at least scaling)

	float x, y;	///< Coordinates.
};

