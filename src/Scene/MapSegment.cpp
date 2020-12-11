#include "MapSegment.hpp"

void MapSegment::SetProperties(const std::map<std::string, std::string>& mapProperties)
{
    EntityDictionaryBuilder builder;
    builder.AddMap(mapProperties);
    _mapProperties = builder.BuildList();

    properties = EntityDictionary(_mapProperties.data(), _mapProperties.size());
}
