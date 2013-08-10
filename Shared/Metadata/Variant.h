//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for describing the contents of a sum type instance (tagged variant)
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include <vector>
#include <set>


class VariantDefinition
{
// Construction
public:
	VariantDefinition()
		: DataSize(0)
	{ }

// Base type configuration
public:
	void AddBaseType(Metadata::EpochTypeID type, size_t typesize)
	{
		BaseTypes.insert(type);
		DataSize = std::max(DataSize, typesize);
	}

// Accessors
public:
	size_t GetMaxSize() const
	{ return DataSize + sizeof(Metadata::EpochTypeID); }

	const std::set<Metadata::EpochTypeID>& GetBaseTypes() const
	{ return BaseTypes; }

// Internal tracking
private:
	size_t DataSize;
	std::set<Metadata::EpochTypeID> BaseTypes;
};
