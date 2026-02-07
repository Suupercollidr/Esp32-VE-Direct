/**
 * @file maputils.h
 * @brief mergeMaps(map1, map2) merges two maps with the same structure
 *        also define structs and such for map mania
 * @date 2026-01-08
 *
 */

#pragma once
#include <map>

/*
// function to any type of maps, as long as both are same type 
template <typename BOSSE, typename KURT>
void mergeMaps(const std::map<BOSSE, KURT> &source, std::map<BOSSE, KURT> &destination)
{
  for (const auto &pair : source)
  destination.insert(pair);
}
*/

using CodeMap = std::map<String, std::map<int, String>>;