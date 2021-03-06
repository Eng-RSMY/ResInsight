/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017-     Statoil ASA
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

#include "RifWellPathFormationReader.h"

#include <QFile>
#include <QStringList>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::map<QString, std::vector<std::pair<double, QString>>> RifWellPathFormationReader::readWellFormations(const QString& filePath)
{
    std::map<QString, std::vector<std::pair<double, QString>>> formations;

    readFileIntoMap(filePath, &formations);

    return formations;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::map<QString, cvf::ref<RigWellPathFormations>> RifWellPathFormationReader::readWellFormationsToGeometry(const QString& filePath)
{
    std::map<QString, cvf::ref<RigWellPathFormations>> result;

    std::map<QString, std::vector<std::pair<double, QString>>> formations;

    readFileIntoMap(filePath, &formations);

    std::map<QString, std::vector<std::pair<double, QString>>>::iterator it;

    for (it = formations.begin(); it != formations.end(); it++)
    {
        cvf::ref<RigWellPathFormations> wellPathFormations = new RigWellPathFormations(it->second);
        result[it->first] = wellPathFormations;
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RifWellPathFormationReader::readFileIntoMap(const QString&                                              filePath,
                                                 std::map<QString, std::vector<std::pair<double, QString>>>* formations)
{
    QFile data(filePath);

    if (!data.open(QFile::ReadOnly))
    {
        return;
    }
    QStringList header;

    while (header.size() < 3)
    {
        QString line              = data.readLine().toLower();
        QString lineWithoutSpaces = line.remove(' ');

        header = lineWithoutSpaces.split(';', QString::KeepEmptyParts);
    }

    const QString wellNameText      = "wellname";
    const QString surfaceNameText   = "surfacename";
    const QString measuredDepthText = "md";

    int wellNameIndex      = header.indexOf(wellNameText);
    int surfaceNameIndex   = header.indexOf(surfaceNameText);
    int measuredDepthIndex = header.indexOf(measuredDepthText);

    if (wellNameIndex == -1 || surfaceNameIndex == -1 || measuredDepthIndex == -1)
    {
        return;
    }

    do
    {
        QString line = data.readLine();

        QStringList dataLine = line.split(';', QString::KeepEmptyParts);
        if (dataLine.size() != header.size())
            continue;

        QString wellName      = dataLine[wellNameIndex];
        QString surfaceName   = dataLine[surfaceNameIndex];
        double  measuredDepth = dataLine[measuredDepthIndex].toDouble();

        (*formations)[wellName].push_back(std::make_pair(measuredDepth, surfaceName));

    } while (!data.atEnd());
}
