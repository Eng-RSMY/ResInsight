/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017     Statoil ASA
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

#include "RigFlowDiagSolverInterface.h"

#include <QWidget>

class QDockWidget;
class QButtonGroup;
class QCheckBox;
class QwtPlot;


//==================================================================================================
//
//
//
//==================================================================================================
class RiuRelativePermeabilityPlotPanel : public QWidget
{
    Q_OBJECT

public:
    RiuRelativePermeabilityPlotPanel(QDockWidget* parent);
    virtual ~RiuRelativePermeabilityPlotPanel();

    void setPlotData(const std::vector<RigFlowDiagSolverInterface::RelPermCurve>& relPermCurves, double swat, double sgas, QString cellReferenceText);
    void clearPlot();

private:
    void            plotUiSelectedCurves();
    static void     setPlotDefaults(QwtPlot* plot);
    static void     plotCurvesInQwt(const std::vector<RigFlowDiagSolverInterface::RelPermCurve>& curveArr, double swat, double sgas, QString cellReferenceText, QwtPlot* plot);
    static QColor   curveColorFromIdent(RigFlowDiagSolverInterface::RelPermCurve::Ident ident);
    static QString  determineXAxisTitleFromCurveCollection(const std::vector<RigFlowDiagSolverInterface::RelPermCurve>& curveArr);
    static void     addVerticalSaturationMarkerLine(double saturationValue, QString label, QColor color, QwtPlot* plot);
    static void     addCurveConstSaturationIntersectionMarker(const RigFlowDiagSolverInterface::RelPermCurve& curve, double saturationValue, QColor markerColor, bool plotCurveOnRightAxis, QwtPlot* plot);
    static double   interpolatedCurveYValue(const std::vector<double>& xVals, const std::vector<double>& yVals, double x);

private slots:
    void            slotButtonInButtonGroupClicked(int);
    void            slotUnscaledCheckBoxStateChanged(int);

private:
    std::vector<RigFlowDiagSolverInterface::RelPermCurve>   m_allCurvesArr;
    double                                                  m_swat;
    double                                                  m_sgas;
    QString                                                 m_cellReferenceText;
    QwtPlot*                                                m_qwtPlot;
    QButtonGroup*                                           m_selectedCurvesButtonGroup;
    QCheckBox*                                              m_showUnscaledCheckBox;
};

