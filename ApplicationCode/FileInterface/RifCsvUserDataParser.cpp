/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017-  Statoil ASA
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

#include "RifCsvUserDataParser.h"

#include "RifEclipseUserDataKeywordTools.h"
#include "RifEclipseUserDataParserTools.h"

#include "RiaDateStringParser.h"
#include "RiaLogging.h"
#include "RiaStdStringTools.h"
#include "RiaQDateTimeTools.h"

#include "../Commands/SummaryPlotCommands/RicPasteAsciiDataToSummaryPlotFeatureUi.h"

#include "cvfAssert.h"

#include <QString>
#include <QTextStream>
#include <QFile>

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RifCsvUserDataParser::RifCsvUserDataParser(QString* errorText) :
    m_errorText(errorText)
{
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RifCsvUserDataParser::~RifCsvUserDataParser()
{
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RifCsvUserDataParser::parse(const AsciiDataParseOptions& parseOptions)
{
    return parseData(parseOptions);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
const TableData& RifCsvUserDataParser::tableData() const
{
    return m_tableData;
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
const ColumnInfo* RifCsvUserDataParser::columnInfo(size_t columnIndex) const
{
    if (columnIndex >= m_tableData.columnInfos().size()) return nullptr;

    return &(m_tableData.columnInfos()[columnIndex]);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RifCsvUserDataParser::parseColumnInfo(const QString& cellSeparator)
{
    QTextStream* dataStream = openDataStream();
    std::vector<ColumnInfo> columnInfoList;
    bool result = parseColumnInfo(dataStream, cellSeparator, &columnInfoList);

    if (result)
    {
        m_tableData = TableData("", "", columnInfoList);
    }
    closeDataStream();
    return result;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RifCsvUserDataParser::previewText()
{
    QTextStream *stream = openDataStream();

    if (!stream) return "";

    QString preview;
    QTextStream outStream(&preview);
    int iLine = 0;

    while (iLine < 30 + 1 && !stream->atEnd())
    {
        QString line = stream->readLine();

        if (line.isEmpty()) continue;

        outStream << line;
        outStream << "\n";
        iLine++;
    }
    closeDataStream();
    return columnifyText(preview);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RifCsvUserDataParser::parseColumnInfo(QTextStream* dataStream, const QString& cellSeparator, std::vector<ColumnInfo>* columnInfoList)
{
    bool headerFound = false;

    if (!columnInfoList) return false;

    columnInfoList->clear();
    while (!headerFound)
    {
        QString line = dataStream->readLine();
        if (line.trimmed().isEmpty()) continue;

        QStringList lineColumns = splitLineAndTrim(line, cellSeparator);

        int colCount = lineColumns.size();

        for (int iCol = 0; iCol < colCount; iCol++)
        {
            QString colName = lineColumns[iCol];
            RifEclipseSummaryAddress addr = RifEclipseSummaryAddress::importedAddress(colName.toStdString());
            ColumnInfo col = ColumnInfo::createColumnInfoFromCsvData(addr, "");

            columnInfoList->push_back(col);
        }
        headerFound = true;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RifCsvUserDataParser::parseData(const AsciiDataParseOptions& parseOptions)
{
    bool errors = false;
    enum { FIRST_DATA_ROW, DATA_ROW } parseState = FIRST_DATA_ROW;
    int colCount;
    std::vector<ColumnInfo> columnInfoList;

    QTextStream* dataStream = openDataStream();

    // Parse header
    if (!parseColumnInfo(dataStream, parseOptions.cellSeparator, &columnInfoList))
    {
        m_errorText->append("Failed to parse column headers");
        return false;
    }

    colCount = (int)columnInfoList.size();
    while (!dataStream->atEnd() && !errors)
    {
        QString line = dataStream->readLine();
        if(line.trimmed().isEmpty()) continue;

        QStringList lineColumns = splitLineAndTrim(line, parseOptions.cellSeparator);

        if(lineColumns.size() != colCount)
        {
            m_errorText->append("CSV file has invalid content (Column count mismatch)");
            errors = true;
            break;
        }
        else if(parseState == FIRST_DATA_ROW)
        {
            for (int iCol = 0; iCol < colCount; iCol++)
            {
                std::string colData = lineColumns[iCol].toStdString();
                ColumnInfo& col = columnInfoList[iCol];

                // Determine column data type
                if (col.dataType == ColumnInfo::NONE)
                {
                    if (QString::fromStdString(col.summaryAddress.quantityName()) == parseOptions.timeSeriesColumnName)
                    {
                        col.dataType = ColumnInfo::DATETIME;
                    }
                    else
                    {
                        if (RiaStdStringTools::isNumber(colData, parseOptions.locale.decimalPoint().toAscii()))
                        {
                            col.dataType = ColumnInfo::NUMERIC;
                        }
                        else
                        {
                            col.dataType = ColumnInfo::TEXT;
                        }
                    }
                }
            }
            
            parseState = DATA_ROW;
        }
                
        if (parseState == DATA_ROW)
        {
            for (int iCol = 0; iCol < colCount; iCol++)
            {
                QString& colData = lineColumns[iCol];
                ColumnInfo& col = columnInfoList[iCol];

                try
                {
                    if (col.dataType == ColumnInfo::NUMERIC)
                    {
                        col.values.push_back(parseOptions.locale.toDouble(colData));
                    }
                    else if (col.dataType == ColumnInfo::TEXT)
                    {
                        col.textValues.push_back(colData.toStdString());
                    }
                    else if (col.dataType == ColumnInfo::DATETIME)
                    {
                        QDateTime dt;
                        dt = tryParseDateTime(colData.toStdString(), parseOptions.dateTimeFormat);

                        if (!dt.isValid() && !parseOptions.useCustomDateTimeFormat)
                        {
                            // Try to match date format only
                            dt = tryParseDateTime(colData.toStdString(), parseOptions.dateFormat);
                        }

                        if (!dt.isValid()) throw 0;
                        col.dateTimeValues.push_back(dt);
                    }
                }
                catch (...)
                {
                    m_errorText->append("CSV file has invalid content (Column type mismatch)");
                    errors = true;
                    break;
                }
            }
        }
    }

    closeDataStream();

    if (!errors)
    {
        TableData td("", "", columnInfoList);
        m_tableData = td;
    }
    return !errors;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RifCsvUserDataParser::columnifyText(const QString& text)
{
    QString pretty = text;

    QString cellSep = tryDetermineCellSeparator();
    if (!cellSep.isEmpty())
    {
        if (cellSep == ";" || cellSep == ",")
        {
            pretty = pretty.replace(cellSep, QString("\t") + cellSep);
        }
    }

    return pretty;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QStringList RifCsvUserDataParser::splitLineAndTrim(const QString& line, const QString& separator)
{
    QStringList cols = line.split(separator);
    for (QString& col : cols)
    {
        col = col.trimmed();
    }
    return cols;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QDateTime RifCsvUserDataParser::tryParseDateTime(const std::string& colData, const QString& format)
{
    return RiaQDateTimeTools::fromString(QString::fromStdString(colData), format);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RifCsvUserDataParser::tryDetermineCellSeparator()
{
    QTextStream* dataStream = openDataStream();
    std::vector<QString> lines;
    int iLine = 0;

    while(iLine < 10 && !dataStream->atEnd())
    {
        QString line = dataStream->readLine();
        if(line.isEmpty()) continue;

        lines.push_back(line);
        iLine++;
    }
    closeDataStream();

    // Try different cell separators
    int totColumnCountTab = 0;
    int totColumnCountSemicolon = 0;
    int totColumnCountComma = 0;

    for (const QString& line : lines)
    {
        totColumnCountTab       += splitLineAndTrim(line, "\t").size();
        totColumnCountSemicolon += splitLineAndTrim(line, ";").size();
        totColumnCountComma     += splitLineAndTrim(line, ",").size();
    }

    double avgColumnCountTab        = (double)totColumnCountTab / lines.size();
    double avgColumnCountSemicolon  = (double)totColumnCountSemicolon / lines.size();
    double avgColumnCountComma      = (double)totColumnCountComma / lines.size();

    // Select the one having highest average
    double maxAvg = std::max(std::max(avgColumnCountTab, avgColumnCountSemicolon), avgColumnCountComma);

    if (maxAvg == avgColumnCountTab)         return "\t";
    if (maxAvg == avgColumnCountSemicolon)   return ";";
    if (maxAvg == avgColumnCountComma)       return ",";
    return "";
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RifCsvUserDataFileParser::RifCsvUserDataFileParser(const QString& fileName, QString* errorText) :
    RifCsvUserDataParser(errorText)
{
    m_fileName = fileName;
    m_file = nullptr;
    m_textStream = nullptr;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RifCsvUserDataFileParser::~RifCsvUserDataFileParser()
{
    if (m_textStream)
    {
        delete m_textStream;
    }
    closeFile();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QTextStream* RifCsvUserDataFileParser::openDataStream()
{
    if (!openFile()) return nullptr;

    m_textStream = new QTextStream(m_file);
    return m_textStream;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RifCsvUserDataFileParser::closeDataStream()
{
    if (m_textStream)
    {
        delete m_textStream;
        m_textStream = nullptr;
    }
    closeFile();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RifCsvUserDataFileParser::openFile()
{
    if (!m_file)
    {
        m_file = new QFile(m_fileName);
        if (!m_file->open(QIODevice::ReadOnly | QIODevice::Text))
        {
            RiaLogging::error(QString("Failed to open %1").arg(m_fileName));

            delete m_file;
            return false;
        }
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RifCsvUserDataFileParser::closeFile()
{
    if (m_file)
    {
        m_file->close();
        delete m_file;
        m_file = nullptr;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RifCsvUserDataPastedTextParser::RifCsvUserDataPastedTextParser(const QString& text, QString* errorText):
    RifCsvUserDataParser(errorText)
{
    m_text = text;
    m_textStream = nullptr;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RifCsvUserDataPastedTextParser::~RifCsvUserDataPastedTextParser()
{
    if (m_textStream)
    {
        delete m_textStream;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QTextStream* RifCsvUserDataPastedTextParser::openDataStream()
{
    if (m_textStream)
    {
        delete m_textStream;
    }
    m_textStream = new QTextStream(&m_text);
    return m_textStream;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RifCsvUserDataPastedTextParser::closeDataStream()
{
    if (m_textStream)
    {
        delete m_textStream;
        m_textStream = nullptr;
    }
}