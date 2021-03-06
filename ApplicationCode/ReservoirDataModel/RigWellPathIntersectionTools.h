/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017 Statoil ASA
// 
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
// 
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
// 
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "RigCell.h"

#include "RigHexIntersectionTools.h"

#include "cvfVector3.h"

#include <array>

class RigWellPath;
class RigMainGrid;
class RigEclipseCaseData;

//==================================================================================================
/// 
//==================================================================================================
struct WellPathCellIntersectionInfo 
{
    WellPathCellIntersectionInfo(size_t globReservoirCellIndex, 
                                 cvf::Vec3d startPoint, 
                                 cvf::Vec3d endPoint, 
                                 cvf::Vec3d internalCellLengths)
        : globCellIndex(globReservoirCellIndex),
          startPoint(startPoint),
          endPoint(endPoint),
          internalCellLengths(internalCellLengths)
    {}

    size_t                        globCellIndex;
    cvf::Vec3d                    startPoint;
    cvf::Vec3d                    endPoint;
    cvf::Vec3d                    internalCellLengths; // intersectionLengthsInCellCS
};

//==================================================================================================
/// 
//==================================================================================================
class RigWellPathIntersectionTools
{
public:
    static std::vector<WellPathCellIntersectionInfo>   findCellsIntersectedByPath(const RigEclipseCaseData* caseData, 
                                                                                  const std::vector<cvf::Vec3d>& pathCoords,
                                                                                  const std::vector<double>& pathMds);

    static std::vector<HexIntersectionInfo>            getIntersectedCells(const RigMainGrid* grid, const std::vector<cvf::Vec3d>& coords);

    static cvf::Vec3d                                  calculateLengthInCell(const std::array<cvf::Vec3d, 8>& hexCorners, const cvf::Vec3d& startPoint, const cvf::Vec3d& endPoint);
    static cvf::Vec3d                                  calculateLengthInCell(const RigMainGrid* grid, size_t cellIndex, const cvf::Vec3d& startPoint, const cvf::Vec3d& endPoint);

    static std::vector<size_t>                         findCloseCells(const RigMainGrid* grid, const cvf::BoundingBox& bb);
    static size_t                                      findCellFromCoords(const RigMainGrid* caseData, const cvf::Vec3d& coords, bool* foundCell);

private:
    static void                                        removeEnteringIntersections(std::vector<HexIntersectionInfo>* intersections);
};
