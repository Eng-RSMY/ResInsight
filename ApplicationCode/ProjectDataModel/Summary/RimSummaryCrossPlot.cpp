/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2016 Statoil ASA
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

#include "RimSummaryCrossPlot.h"

#include "RiaApplication.h"

#include "RimAsciiDataCurve.h"
#include "RimGridTimeHistoryCurve.h"
#include "RimSummaryCase.h"
#include "RimSummaryCurve.h"
#include "RimSummaryCurveCollection.h"
#include "RimSummaryCurveFilter.h"
#include "RimSummaryCurvesCalculator.h"
#include "RimSummaryPlotCollection.h"
#include "RimSummaryTimeAxisProperties.h"
#include "RimSummaryYAxisProperties.h"

#include "RiuMainPlotWindow.h"
#include "RiuSummaryQwtCrossPlot.h"

#include "cvfBase.h"
#include "cvfColor3.h"

#include "cafPdmUiTreeOrdering.h"
#include "cafPdmUiCheckBoxEditor.h"

#include <QDateTime>
#include <QString>
#include <QRectF>

#include "qwt_abstract_legend.h"
#include "qwt_legend.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_renderer.h"

#include "vector"


CAF_PDM_SOURCE_INIT(RimSummaryCrossPlot, "RimSummaryCrossPlot");

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimSummaryCrossPlot::RimSummaryCrossPlot()
{
    CAF_PDM_InitObject("Summary Plot", ":/SummaryPlotLight16x16.png", "", "");

    CAF_PDM_InitField(&m_userName, "PlotDescription", QString("Summary Plot"), "Name", "", "", "");
    CAF_PDM_InitField(&m_showPlotTitle, "ShowPlotTitle", true, "Plot Title", "", "", "");
    m_showPlotTitle.uiCapability()->setUiLabelPosition(caf::PdmUiItemInfo::HIDDEN);
    CAF_PDM_InitField(&m_showLegend, "ShowLegend", true, "Legend", "", "", "");
    m_showLegend.uiCapability()->setUiLabelPosition(caf::PdmUiItemInfo::HIDDEN);

    CAF_PDM_InitFieldNoDefault(&m_curveFilters_OBSOLETE, "SummaryCurveFilters", "", "", "", "");
    m_curveFilters_OBSOLETE.uiCapability()->setUiTreeHidden(true);

	CAF_PDM_InitFieldNoDefault(&m_summaryCurveCollection, "SummaryCurveCollection", "", "", "", "");
    m_summaryCurveCollection.uiCapability()->setUiTreeHidden(true);

	CAF_PDM_InitFieldNoDefault(&m_summaryCurves_OBSOLETE, "SummaryCurves", "", "", "", "");
    m_summaryCurves_OBSOLETE.uiCapability()->setUiTreeHidden(true);

    CAF_PDM_InitFieldNoDefault(&m_gridTimeHistoryCurves, "GridTimeHistoryCurves", "", "", "", "");
    m_gridTimeHistoryCurves.uiCapability()->setUiTreeHidden(true);

    CAF_PDM_InitFieldNoDefault(&m_asciiDataCurves, "AsciiDataCurves", "", "", "", "");
    m_asciiDataCurves.uiCapability()->setUiTreeHidden(true);

    CAF_PDM_InitFieldNoDefault(&m_leftYAxisProperties, "LeftYAxisProperties", "Left Y Axis", "", "", "");
    m_leftYAxisProperties.uiCapability()->setUiTreeHidden(true);

    CAF_PDM_InitFieldNoDefault(&m_rightYAxisProperties, "RightYAxisProperties", "Right Y Axis", "", "", "");
    m_rightYAxisProperties.uiCapability()->setUiTreeHidden(true);

    CAF_PDM_InitFieldNoDefault(&m_timeAxisProperties, "TimeAxisProperties", "Time Axis", "", "", "");
    m_timeAxisProperties.uiCapability()->setUiTreeHidden(true);

    CAF_PDM_InitField(&m_isAutoZoom, "AutoZoom", true, "Auto Zoom", "", "", "");
    m_isAutoZoom.uiCapability()->setUiHidden(true);

    m_summaryCurveCollection = new RimSummaryCurveCollection;

    m_leftYAxisProperties = new RimSummaryYAxisProperties;
    m_leftYAxisProperties->setNameAndAxis("Left Y-Axis", QwtPlot::yLeft);

    m_rightYAxisProperties = new RimSummaryYAxisProperties;
    m_rightYAxisProperties->setNameAndAxis("Right Y-Axis", QwtPlot::yRight);

    m_timeAxisProperties = new RimSummaryTimeAxisProperties;

    setAsPlotMdiWindow();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimSummaryCrossPlot::~RimSummaryCrossPlot()
{
    removeMdiWindowFromMdiArea();

    deleteViewWidget();

    m_summaryCurves_OBSOLETE.deleteAllChildObjects();
    m_curveFilters_OBSOLETE.deleteAllChildObjects();
    delete m_summaryCurveCollection;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::updateAxes()
{
    updateAxis(RiaDefines::PLOT_AXIS_LEFT);
    updateAxis(RiaDefines::PLOT_AXIS_RIGHT);

    updateZoomInQwt();

    updateTimeAxis();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimSummaryCrossPlot::isLogarithmicScaleEnabled(RiaDefines::PlotAxis plotAxis) const
{
    return yAxisPropertiesForAxis(plotAxis)->isLogarithmicScaleEnabled();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimSummaryTimeAxisProperties* RimSummaryCrossPlot::timeAxisProperties()
{
    return m_timeAxisProperties();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::selectAxisInPropertyEditor(int axis)
{
    RiuMainPlotWindow* plotwindow = RiaApplication::instance()->getOrCreateAndShowMainPlotWindow();
    if (axis == QwtPlot::yLeft)
    {
        plotwindow->selectAsCurrentItem(m_leftYAxisProperties);
    }
    else if (axis == QwtPlot::yRight)
    {
        plotwindow->selectAsCurrentItem(m_rightYAxisProperties);
    }
    else if (axis == QwtPlot::xBottom)
    {
        plotwindow->selectAsCurrentItem(m_timeAxisProperties);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
time_t RimSummaryCrossPlot::firstTimeStepOfFirstCurve()
{
    RimSummaryCurve * firstCurve = nullptr;

    for (RimSummaryCurveFilter* curveFilter : m_curveFilters_OBSOLETE )
    {
        if (curveFilter)
        {
            std::vector<RimSummaryCurve *> curves = curveFilter->curves();
            size_t i = 0;
            while (firstCurve == nullptr && i < curves.size())
            {
                firstCurve = curves[i];
                i++;
            }

            if (firstCurve) break;
        }
    }

    if (m_summaryCurveCollection)
    {
        std::vector<RimSummaryCurve*> curves = m_summaryCurveCollection->curves();
        size_t i = 0;
        while (firstCurve == nullptr && i < curves.size())
        {
            firstCurve = curves[i];
            ++i;
        }
    }

    size_t i = 0;
    while (firstCurve == nullptr && i < m_summaryCurves_OBSOLETE.size())
    {
        firstCurve = m_summaryCurves_OBSOLETE[i];
        ++i;
    }

    if (firstCurve && firstCurve->timeSteps().size() > 0)
    {
        return firstCurve->timeSteps()[0];
    }
    else return time_t(0);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QWidget* RimSummaryCrossPlot::viewWidget()
{
    return m_qwtPlot;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimSummaryCrossPlot::asciiDataForPlotExport() const
{
    QString out;

    out += description();

    {
        std::vector<RimSummaryCurve*> curves;
        this->descendantsIncludingThisOfType(curves);

        std::vector<QString> caseNames;
        std::vector<std::vector<time_t> > timeSteps;

        std::vector<std::vector<std::vector<double> > > allCurveData;
        std::vector<std::vector<QString > > allCurveNames;
        //Vectors containing cases - curves - data points/curve name

        for (RimSummaryCurve* curve : curves)
        {
            if (!curve->isCurveVisible()) continue;
            QString curveCaseName = curve->summaryCase()->caseName();

            size_t casePosInList = cvf::UNDEFINED_SIZE_T;
            for (size_t i = 0; i < caseNames.size(); i++)
            {
                if (curveCaseName == caseNames[i]) casePosInList = i;
            }

            if (casePosInList == cvf::UNDEFINED_SIZE_T)
            {
                caseNames.push_back(curveCaseName);
            
                std::vector<time_t> curveTimeSteps = curve->timeSteps();
                timeSteps.push_back(curveTimeSteps);

                std::vector<std::vector<double> > curveDataForCase;
                std::vector<double> curveYData = curve->yValues();
                curveDataForCase.push_back(curveYData);
                allCurveData.push_back(curveDataForCase);

                std::vector<QString> curveNamesForCase;
                curveNamesForCase.push_back(curve->curveName());
                allCurveNames.push_back(curveNamesForCase);
            }
            else
            {
                std::vector<double> curveYData = curve->yValues();
                allCurveData[casePosInList].push_back(curveYData);

                QString curveName = curve->curveName();
                allCurveNames[casePosInList].push_back(curveName);
            }
        }

        for (size_t i = 0; i < timeSteps.size(); i++) //cases
        {
            out += "\n\n";
            out += "Case: " + caseNames[i];
            out += "\n";

            for (size_t j = 0; j < timeSteps[i].size(); j++) //time steps & data points
            {
                if (j == 0)
                {
                    out += "Date and time";
                    for (size_t k = 0; k < allCurveNames[i].size(); k++) // curves
                    {
                        out += "\t" + (allCurveNames[i][k]);
                    }
                }
                out += "\n";
                out += QDateTime::fromTime_t(timeSteps[i][j]).toUTC().toString("yyyy-MM-dd hh:mm:ss ");

                for (size_t k = 0; k < allCurveData[i].size(); k++) // curves
                {
                    out += "\t" + QString::number(allCurveData[i][k][j], 'g', 6);
                }
            }
        }
    }


    {
        std::vector<QString> caseNames;
        std::vector<std::vector<time_t> > timeSteps;

        std::vector<std::vector<std::vector<double> > > allCurveData;
        std::vector<std::vector<QString > > allCurveNames;
        //Vectors containing cases - curves - data points/curve name

        for (RimGridTimeHistoryCurve* curve : m_gridTimeHistoryCurves)
        {
            if (!curve->isCurveVisible()) continue;
            QString curveCaseName = curve->caseName();

            size_t casePosInList = cvf::UNDEFINED_SIZE_T;
            for (size_t i = 0; i < caseNames.size(); i++)
            {
                if (curveCaseName == caseNames[i]) casePosInList = i;
            }

            if (casePosInList == cvf::UNDEFINED_SIZE_T)
            {
                caseNames.push_back(curveCaseName);

                std::vector<time_t> curveTimeSteps = curve->timeStepValues();
                timeSteps.push_back(curveTimeSteps);

                std::vector<std::vector<double> > curveDataForCase;
                std::vector<double> curveYData = curve->yValues();
                curveDataForCase.push_back(curveYData);
                allCurveData.push_back(curveDataForCase);

                std::vector<QString> curveNamesForCase;
                curveNamesForCase.push_back(curve->curveName());
                allCurveNames.push_back(curveNamesForCase);
            }
            else
            {
                std::vector<double> curveYData = curve->yValues();
                allCurveData[casePosInList].push_back(curveYData);

                QString curveName = curve->curveName();
                allCurveNames[casePosInList].push_back(curveName);
            }
        }

        for (size_t i = 0; i < timeSteps.size(); i++) //cases
        {
            out += "\n\n";
            out += "Case: " + caseNames[i];
            out += "\n";

            for (size_t j = 0; j < timeSteps[i].size(); j++) //time steps & data points
            {
                if (j == 0)
                {
                    out += "Date and time";
                    for (size_t k = 0; k < allCurveNames[i].size(); k++) // curves
                    {
                        out += "\t" + (allCurveNames[i][k]);
                    }
                }
                out += "\n";
                out += QDateTime::fromTime_t(timeSteps[i][j]).toUTC().toString("yyyy-MM-dd hh:mm:ss ");

                for (size_t k = 0; k < allCurveData[i].size(); k++) // curves
                {
                    out += "\t" + QString::number(allCurveData[i][k][j], 'g', 6);
                }
            }
        }
    }

    {
        std::vector<std::vector<time_t> > timeSteps;

        std::vector<std::vector<std::vector<double> > > allCurveData;
        std::vector<std::vector<QString > > allCurveNames;
        //Vectors containing cases - curves - data points/curve name

        for (RimAsciiDataCurve* curve : m_asciiDataCurves)
        {
            if (!curve->isCurveVisible()) continue;

            size_t casePosInList = cvf::UNDEFINED_SIZE_T;

            if (casePosInList == cvf::UNDEFINED_SIZE_T)
            {
                std::vector<time_t> curveTimeSteps = curve->timeSteps();
                timeSteps.push_back(curveTimeSteps);

                std::vector<std::vector<double> > curveDataForCase;
                std::vector<double> curveYData = curve->yValues();
                curveDataForCase.push_back(curveYData);
                allCurveData.push_back(curveDataForCase);

                std::vector<QString> curveNamesForCase;
                curveNamesForCase.push_back(curve->curveName());
                allCurveNames.push_back(curveNamesForCase);
            }
            else
            {
                std::vector<double> curveYData = curve->yValues();
                allCurveData[casePosInList].push_back(curveYData);

                QString curveName = curve->curveName();
                allCurveNames[casePosInList].push_back(curveName);
            }
        }

        for (size_t i = 0; i < timeSteps.size(); i++) //cases
        {
            out += "\n\n";

            for (size_t j = 0; j < timeSteps[i].size(); j++) //time steps & data points
            {
                if (j == 0)
                {
                    out += "Date and time";
                    for (size_t k = 0; k < allCurveNames[i].size(); k++) // curves
                    {
                        out += "\t" + (allCurveNames[i][k]);
                    }
                }
                out += "\n";
                out += QDateTime::fromTime_t(timeSteps[i][j]).toUTC().toString("yyyy-MM-dd hh:mm:ss ");

                for (size_t k = 0; k < allCurveData[i].size(); k++) // curves
                {
                    out += "\t" + QString::number(allCurveData[i][k][j], 'g', 6);
                }
            }
        }
    }

    return out;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RimSummaryCurve*> RimSummaryCrossPlot::summaryCurves() const
{
    return m_summaryCurveCollection->curves();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::deleteAllSummaryCurves()
{
    m_summaryCurveCollection->deleteAllCurves();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimSummaryCurveCollection* RimSummaryCrossPlot::summaryCurveCollection() const
{
    return m_summaryCurveCollection(); 
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RiuSummaryQwtCrossPlot* RimSummaryCrossPlot::qwtPlot() const
{
    return m_qwtPlot;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::updateAxis(RiaDefines::PlotAxis plotAxis)
{
    if (!m_qwtPlot) return;

    QwtPlot::Axis qwtAxis = QwtPlot::yLeft;
    if (plotAxis == RiaDefines::PLOT_AXIS_LEFT)
    {
        qwtAxis = QwtPlot::yLeft;
    }
    else
    {
        qwtAxis = QwtPlot::yRight;
    }

    RimSummaryYAxisProperties* yAxisProperties = yAxisPropertiesForAxis(plotAxis);
    if (yAxisProperties->isActive() && hasVisibleCurvesForAxis(plotAxis))
    {
        m_qwtPlot->enableAxis(qwtAxis, true);

        std::set<QString> timeHistoryQuantities;

        for (auto c : visibleTimeHistoryCurvesForAxis(plotAxis))
        {
            timeHistoryQuantities.insert(c->quantityName());
        }

        RimSummaryPlotYAxisFormatter calc(yAxisProperties,
                                          visibleSummaryCurvesForAxis(plotAxis),
                                          visibleAsciiDataCurvesForAxis(plotAxis),
                                          timeHistoryQuantities);
        //calc.applyYAxisPropertiesToPlot(m_qwtPlot);
    }
    else
    {
        m_qwtPlot->enableAxis(qwtAxis, false);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::updateZoomForAxis(RiaDefines::PlotAxis plotAxis)
{
    RimSummaryYAxisProperties* yAxisProps = yAxisPropertiesForAxis(plotAxis);

    if (yAxisProps->isLogarithmicScaleEnabled)
    {
        std::vector<double> yValues;
        std::vector<QwtPlotCurve*> plotCurves;

        for (RimSummaryCurve* c : visibleSummaryCurvesForAxis(plotAxis))
        {
            std::vector<double> curveValues = c->yValues();
            yValues.insert(yValues.end(), curveValues.begin(), curveValues.end());
            plotCurves.push_back(c->qwtPlotCurve());
        }

        for (RimGridTimeHistoryCurve* c : visibleTimeHistoryCurvesForAxis(plotAxis))
        {
            std::vector<double> curveValues = c->yValues();
            yValues.insert(yValues.end(), curveValues.begin(), curveValues.end());
            plotCurves.push_back(c->qwtPlotCurve());
        }

        for (RimAsciiDataCurve* c : visibleAsciiDataCurvesForAxis(plotAxis))
        {
            std::vector<double> curveValues = c->yValues();
            yValues.insert(yValues.end(), curveValues.begin(), curveValues.end());
            plotCurves.push_back(c->qwtPlotCurve());
        }

        double min, max;
        RimSummaryPlotYAxisRangeCalculator calc(plotCurves, yValues);
        calc.computeYRange(&min, &max);

        m_qwtPlot->setAxisScale(yAxisProps->qwtPlotAxisType(), min, max);
    }
    else
    {
        m_qwtPlot->setAxisAutoScale(yAxisProps->qwtPlotAxisType(), true);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RimSummaryCurve*> RimSummaryCrossPlot::visibleSummaryCurvesForAxis(RiaDefines::PlotAxis plotAxis) const
{
    std::vector<RimSummaryCurve*> curves;

    for (RimSummaryCurve* curve : m_summaryCurves_OBSOLETE)
    {
        if (curve->isCurveVisible() && curve->yAxis() == plotAxis)
        {
            curves.push_back(curve);
        }
    }

    for (RimSummaryCurveFilter * curveFilter : m_curveFilters_OBSOLETE)
    {
        if (curveFilter->isCurvesVisible())
        {
            for (RimSummaryCurve* curve : curveFilter->curves())
            {
                if (curve->isCurveVisible() && curve->yAxis() == plotAxis)
                {
                    curves.push_back(curve);
                }
            }
        }
    }

    if (m_summaryCurveCollection && m_summaryCurveCollection->isCurvesVisible())
    {
        for (RimSummaryCurve* curve : m_summaryCurveCollection->curves())
        {
            if (curve->isCurveVisible() && curve->yAxis() == plotAxis)
            {
                curves.push_back(curve);
            }
        }
    }

    return curves;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimSummaryCrossPlot::hasVisibleCurvesForAxis(RiaDefines::PlotAxis plotAxis) const
{
    if (visibleSummaryCurvesForAxis(plotAxis).size() > 0)
    {
        return true;
    }

    if (visibleTimeHistoryCurvesForAxis(plotAxis).size() > 0)
    {
        return true;
    }

    if (visibleAsciiDataCurvesForAxis(plotAxis).size() > 0)
    {
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimSummaryYAxisProperties* RimSummaryCrossPlot::yAxisPropertiesForAxis(RiaDefines::PlotAxis plotAxis) const
{
    RimSummaryYAxisProperties* yAxisProps = nullptr;

    if (plotAxis == RiaDefines::PLOT_AXIS_LEFT)
    {
        yAxisProps = m_leftYAxisProperties();
    }
    else
    {
        yAxisProps = m_rightYAxisProperties();
    }

    CVF_ASSERT(yAxisProps);

    return yAxisProps;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RimGridTimeHistoryCurve*> RimSummaryCrossPlot::visibleTimeHistoryCurvesForAxis(RiaDefines::PlotAxis plotAxis) const
{
    std::vector<RimGridTimeHistoryCurve*> curves;

    for (auto c : m_gridTimeHistoryCurves)
    {
        if (c->isCurveVisible() && c->yAxis() == plotAxis)
        {
            curves.push_back(c);
        }
    }

    return curves;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RimAsciiDataCurve*> RimSummaryCrossPlot::visibleAsciiDataCurvesForAxis(RiaDefines::PlotAxis plotAxis) const
{
    std::vector<RimAsciiDataCurve*> curves;

    for (auto c : m_asciiDataCurves)
    {
        if (c->isCurveVisible() && c->yAxis() == plotAxis)
        {
            curves.push_back(c);
        }
    }

    return curves;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::updateTimeAxis()
{
    if (!m_qwtPlot) return;

    if (!m_timeAxisProperties->isActive())
    {
        m_qwtPlot->enableAxis(QwtPlot::xBottom, false);

        return;
    }

    if (m_timeAxisProperties->timeMode() == RimSummaryTimeAxisProperties::DATE)
    {
        m_qwtPlot->useDateBasedTimeAxis();
    }
    else 
    {
        m_qwtPlot->useTimeBasedTimeAxis();
    }   

    m_qwtPlot->enableAxis(QwtPlot::xBottom, true);

    {
        QString axisTitle;
        if (m_timeAxisProperties->showTitle) axisTitle = m_timeAxisProperties->title();

        QwtText timeAxisTitle = m_qwtPlot->axisTitle(QwtPlot::xBottom);

        QFont font = timeAxisTitle.font();
        font.setBold(true);
        font.setPixelSize(m_timeAxisProperties->fontSize);
        timeAxisTitle.setFont(font);

        timeAxisTitle.setText(axisTitle);

        switch ( m_timeAxisProperties->titlePositionEnum() )
        {
            case RimSummaryTimeAxisProperties::AXIS_TITLE_CENTER:
            timeAxisTitle.setRenderFlags(Qt::AlignCenter);
            break;
            case RimSummaryTimeAxisProperties::AXIS_TITLE_END:
            timeAxisTitle.setRenderFlags(Qt::AlignRight);
            break;
        }

        m_qwtPlot->setAxisTitle(QwtPlot::xBottom, timeAxisTitle);
    }

    {
        QFont timeAxisFont = m_qwtPlot->axisFont(QwtPlot::xBottom);
        timeAxisFont.setBold(false);
        timeAxisFont.setPixelSize(m_timeAxisProperties->fontSize);
        m_qwtPlot->setAxisFont(QwtPlot::xBottom, timeAxisFont);
    }

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::updateCaseNameHasChanged()
{
    for (RimSummaryCurve* curve : m_summaryCurves_OBSOLETE)
    {
        curve->updateCurveNameAndUpdatePlotLegend();
        curve->updateConnectedEditors();
    }

    for (RimSummaryCurveFilter* curveFilter : m_curveFilters_OBSOLETE)
    {
        curveFilter->updateCaseNameHasChanged();
    }

    if (m_summaryCurveCollection)
    {
        m_summaryCurveCollection->updateCaseNameHasChanged();
    }

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::setZoomWindow(const QwtInterval& leftAxis, const QwtInterval& rightAxis, const QwtInterval& timeAxis)
{
    m_leftYAxisProperties->visibleRangeMax = leftAxis.maxValue();
    m_leftYAxisProperties->visibleRangeMin = leftAxis.minValue();
    m_leftYAxisProperties->updateConnectedEditors();

    m_rightYAxisProperties->visibleRangeMax = rightAxis.maxValue();
    m_rightYAxisProperties->visibleRangeMin = rightAxis.minValue();
    m_rightYAxisProperties->updateConnectedEditors();

    m_timeAxisProperties->setVisibleRangeMin(timeAxis.minValue());
    m_timeAxisProperties->setVisibleRangeMax(timeAxis.maxValue());
    m_timeAxisProperties->updateConnectedEditors();

    disableAutoZoom();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::zoomAll()
{
    if (m_qwtPlot)
    {
        m_qwtPlot->setAxisAutoScale(QwtPlot::xBottom, true);

        updateZoomForAxis(RiaDefines::PLOT_AXIS_LEFT);
        updateZoomForAxis(RiaDefines::PLOT_AXIS_RIGHT);

        m_qwtPlot->replot();
    }

    updateZoomWindowFromQwt();

    m_isAutoZoom = true;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::addCurveAndUpdate(RimSummaryCurve* curve)
{
    if (curve)
    {
        m_summaryCurveCollection->addCurve(curve);

        if (m_qwtPlot)
        {
            curve->setParentQwtPlotAndReplot(m_qwtPlot);
            this->updateAxes();
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::addCurveNoUpdate(RimSummaryCurve* curve)
{
    if (curve)
    {
        m_summaryCurveCollection->addCurve(curve);

        if (m_qwtPlot)
        {
            curve->setParentQwtPlotNoReplot(m_qwtPlot);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::deleteCurve(RimSummaryCurve* curve)
{
    if (curve)
    {
        if (m_summaryCurveCollection)
        {
            m_summaryCurveCollection->deleteCurve(curve);
        }
        else
        {
            m_summaryCurves_OBSOLETE.removeChildObject(curve);
            delete curve;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::deleteCurvesAssosiatedWithCase(RimSummaryCase* summaryCase)
{
    for (RimSummaryCurveFilter* summaryCurveFilter : m_curveFilters_OBSOLETE)
    {
        if (!summaryCurveFilter) continue;
        summaryCurveFilter->removeCurvesAssosiatedWithCase(summaryCase);
    }

    if (m_summaryCurveCollection)
    {
        m_summaryCurveCollection->deleteCurvesAssosiatedWithCase(summaryCase);
    }

    std::vector<RimSummaryCurve*> summaryCurvesToDelete;

    for (RimSummaryCurve* summaryCurve : m_summaryCurves_OBSOLETE)
    {
        if (!summaryCurve) continue;
        if (!summaryCurve->summaryCase()) continue;

        if (summaryCurve->summaryCase() == summaryCase)
        {
            summaryCurvesToDelete.push_back(summaryCurve);
        }
    }
    for (RimSummaryCurve* summaryCurve : summaryCurvesToDelete)
    {
        deleteCurve(summaryCurve);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::deleteAllTopLevelCurves()
{
    m_summaryCurves_OBSOLETE.deleteAllChildObjects();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::addCurveFilter(RimSummaryCurveFilter* curveFilter)
{
    if(curveFilter)
    {
        m_curveFilters_OBSOLETE.push_back(curveFilter);
        if(m_qwtPlot)
        {
            curveFilter->setParentQwtPlotAndReplot(m_qwtPlot);
            this->updateAxes();
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::setCurveCollection(RimSummaryCurveCollection* curveCollection)
{
    if (curveCollection)
    {
        m_summaryCurveCollection = curveCollection;
        if (m_qwtPlot)
        {
            m_summaryCurveCollection->setParentQwtPlotAndReplot(m_qwtPlot);
            this->updateAxes();
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::addGridTimeHistoryCurve(RimGridTimeHistoryCurve* curve)
{
    CVF_ASSERT(curve);

    m_gridTimeHistoryCurves.push_back(curve);
    if (m_qwtPlot)
    {
        curve->setParentQwtPlotAndReplot(m_qwtPlot);
        this->updateAxes();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::addAsciiDataCruve(RimAsciiDataCurve* curve)
{
    CVF_ASSERT(curve);

    m_asciiDataCurves.push_back(curve);
    if (m_qwtPlot)
    {
        curve->setParentQwtPlotAndReplot(m_qwtPlot);
        this->updateAxes();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue)
{
    RimViewWindow::fieldChangedByUi(changedField, oldValue, newValue);

    if (changedField == &m_userName || 
        changedField == &m_showPlotTitle ||
        changedField == &m_showLegend)
    {
        updateMdiWindowTitle();
    }
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QImage RimSummaryCrossPlot::snapshotWindowContent()
{
    QImage image;

    if (m_qwtPlot)
    {
        image = QImage(m_qwtPlot->size(), QImage::Format_ARGB32);
        image.fill(QColor(Qt::white).rgb());

        QPainter painter(&image);
        QRectF rect(0, 0, m_qwtPlot->size().width(), m_qwtPlot->size().height());

        QwtPlotRenderer plotRenderer;
        plotRenderer.render(m_qwtPlot, &painter, rect);
    }

    return image;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::defineUiTreeOrdering(caf::PdmUiTreeOrdering& uiTreeOrdering, QString uiConfigName /*= ""*/)
{
    caf::PdmUiTreeOrdering* axisFolder = uiTreeOrdering.add("Axes", ":/Axes16x16.png");
    axisFolder->add(&m_timeAxisProperties);
    axisFolder->add(&m_leftYAxisProperties);
    axisFolder->add(&m_rightYAxisProperties);

    uiTreeOrdering.add(&m_curveFilters_OBSOLETE);
    uiTreeOrdering.add(&m_summaryCurveCollection);
    uiTreeOrdering.add(&m_summaryCurves_OBSOLETE);
    uiTreeOrdering.add(&m_gridTimeHistoryCurves);
    uiTreeOrdering.add(&m_asciiDataCurves);

    uiTreeOrdering.skipRemainingChildren(true);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::onLoadDataAndUpdate()
{
   updateMdiWindowVisibility();    

    for (RimSummaryCurveFilter* curveFilter: m_curveFilters_OBSOLETE)
    {
        curveFilter->loadDataAndUpdate();
    }

    if (m_summaryCurveCollection)
    {
        m_summaryCurveCollection->loadDataAndUpdate(false);
    }

    for (RimSummaryCurve* curve : m_summaryCurves_OBSOLETE)
    {
        curve->loadDataAndUpdate(true);
    }
 
    for (RimGridTimeHistoryCurve* curve : m_gridTimeHistoryCurves)
    {
        curve->loadDataAndUpdate(true);
    }

    for (RimAsciiDataCurve* curve : m_asciiDataCurves)
    {
        curve->loadDataAndUpdate(true);
    }

    if (m_qwtPlot) m_qwtPlot->updateLegend();
    this->updateAxes();
    updateZoomInQwt();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::updateZoomInQwt()
{
    if (!m_qwtPlot) return;
    
    if (m_isAutoZoom)
    {
        zoomAll();
    }
    else
    {
        setZoomIntervalsInQwtPlot();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::setZoomIntervalsInQwtPlot()
{
    QwtInterval left, right, time;

    left.setMinValue(m_leftYAxisProperties->visibleRangeMin());
    left.setMaxValue(m_leftYAxisProperties->visibleRangeMax());
    right.setMinValue(m_rightYAxisProperties->visibleRangeMin());
    right.setMaxValue(m_rightYAxisProperties->visibleRangeMax());
    time.setMinValue(m_timeAxisProperties->visibleRangeMin());
    time.setMaxValue(m_timeAxisProperties->visibleRangeMax());

    m_qwtPlot->setZoomWindow(left, right, time);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::updateZoomWindowFromQwt()
{
    if (!m_qwtPlot) return;

    QwtInterval left, right, time;
    m_qwtPlot->currentVisibleWindow(&left, &right, &time);

    setZoomWindow(left, right, time);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::disableAutoZoom()
{
    m_isAutoZoom = false;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::setDescription(const QString& description)
{
    m_userName = description;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimSummaryCrossPlot::description() const
{
    return m_userName();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::setShowDescription(bool showDescription)
{
    m_showPlotTitle = showDescription;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QWidget* RimSummaryCrossPlot::createViewWidget(QWidget* mainWindowParent)
{
    if (!m_qwtPlot)
    {
        m_qwtPlot = new RiuSummaryQwtCrossPlot(this, mainWindowParent);

        for(RimSummaryCurveFilter* curveFilter: m_curveFilters_OBSOLETE)
        {
            curveFilter->setParentQwtPlotAndReplot(m_qwtPlot);
        }
        
        for (RimSummaryCurve* curve : m_summaryCurves_OBSOLETE)
        {
            curve->setParentQwtPlotAndReplot(m_qwtPlot);
        }

        for ( RimGridTimeHistoryCurve* curve : m_gridTimeHistoryCurves )
        {
            curve->setParentQwtPlotNoReplot(m_qwtPlot);
        }

        for (RimAsciiDataCurve* curve : m_asciiDataCurves)
        {
            curve->setParentQwtPlotNoReplot(m_qwtPlot);
        }

        if ( m_summaryCurveCollection )
        {
        	m_summaryCurveCollection->setParentQwtPlotAndReplot(m_qwtPlot);
        }
   }

    return m_qwtPlot;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::deleteViewWidget()
{
    detachAllCurves();

    if (m_qwtPlot)
    {
        m_qwtPlot->deleteLater();
        m_qwtPlot = nullptr;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::initAfterRead()
{
    // Move summary curves from obsolete storage to the new curve collection
    std::vector<RimSummaryCurve*> curvesToMove;

    for (auto& curveFilter : m_curveFilters_OBSOLETE)
    {
        const auto& tmpCurves = curveFilter->curves();
        curvesToMove.insert(curvesToMove.end(), tmpCurves.begin(), tmpCurves.end());
        curveFilter->clearCurvesWithoutDelete();
    }
    m_curveFilters_OBSOLETE.clear();

    curvesToMove.insert(curvesToMove.end(), m_summaryCurves_OBSOLETE.begin(), m_summaryCurves_OBSOLETE.end());
    m_summaryCurves_OBSOLETE.clear();

    for (const auto& curve : curvesToMove)
    {
        m_summaryCurveCollection->addCurve(curve);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::updateMdiWindowTitle()
{
    if (m_qwtPlot)
    {
        m_qwtPlot->setWindowTitle(m_userName);

        if (m_showPlotTitle)
        {
            m_qwtPlot->setTitle(m_userName);
        }
        else
        {
            m_qwtPlot->setTitle("");
        }


        if (m_showLegend)
        {
            // Will be released in plot destructor or when a new legend is set
            QwtLegend* legend = new QwtLegend(m_qwtPlot);
            m_qwtPlot->insertLegend(legend, QwtPlot::BottomLegend);
        }
        else
        {
            m_qwtPlot->insertLegend(nullptr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::detachAllCurves()
{
    for(RimSummaryCurveFilter* curveFilter: m_curveFilters_OBSOLETE)
    {
        curveFilter->detachQwtCurves();
    }

    if (m_summaryCurveCollection)
    {
        m_summaryCurveCollection->detachQwtCurves();
    }

    for(RimSummaryCurve* curve : m_summaryCurves_OBSOLETE)
    {
        curve->detachQwtCurve();
    }

    for (RimGridTimeHistoryCurve* curve : m_gridTimeHistoryCurves)
    {
        curve->detachQwtCurve();
    }

    for (RimAsciiDataCurve* curve : m_asciiDataCurves)
    {
        curve->detachQwtCurve();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
caf::PdmObject* RimSummaryCrossPlot::findRimCurveFromQwtCurve(const QwtPlotCurve* qwtCurve) const
{
    for(RimSummaryCurve* curve : m_summaryCurves_OBSOLETE)
    {
        if(curve->qwtPlotCurve() == qwtCurve)
        {
            return curve;
        }
    }

    for (RimGridTimeHistoryCurve* curve : m_gridTimeHistoryCurves)
    {
        if (curve->qwtPlotCurve() == qwtCurve)
        {
            return curve;
        }
    }

    for (RimAsciiDataCurve* curve : m_asciiDataCurves)
    {
        if (curve->qwtPlotCurve() == qwtCurve)
        {
            return curve;
        }
    }

    for (RimSummaryCurveFilter* curveFilter: m_curveFilters_OBSOLETE)
    {
        RimSummaryCurve* foundCurve = curveFilter->findRimCurveFromQwtCurve(qwtCurve);
        if (foundCurve) return foundCurve;
    }

    if (m_summaryCurveCollection)
    {
        RimSummaryCurve* foundCurve = m_summaryCurveCollection->findRimCurveFromQwtCurve(qwtCurve);
        
        if (foundCurve)
        {
            m_summaryCurveCollection->setCurrentSummaryCurve(foundCurve);

            return foundCurve;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
size_t RimSummaryCrossPlot::curveCount() const
{
    return m_summaryCurves_OBSOLETE.size() + m_gridTimeHistoryCurves.size() + m_asciiDataCurves.size();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimSummaryCrossPlot::defineEditorAttribute(const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute * attribute)
{
    if (field == &m_showLegend || field == &m_showPlotTitle)
    {
        caf::PdmUiCheckBoxEditorAttribute* myAttr = dynamic_cast<caf::PdmUiCheckBoxEditorAttribute*>(attribute);
        if (myAttr)
        {
            myAttr->m_useNativeCheckBoxLabel = true;
        }
    }
}
