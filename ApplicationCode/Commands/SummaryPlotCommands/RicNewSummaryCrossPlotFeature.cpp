/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2016-     Statoil ASA
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

#include "RicNewSummaryCrossPlotFeature.h"

#include "RiaApplication.h"
#include "RiaPreferences.h"

#include "RicEditSummaryPlotFeature.h"
#include "RicSummaryCurveCreator.h"
#include "RicSummaryCurveCreatorDialog.h"

#include "RimSummaryCurveFilter.h"
#include "RimSummaryCrossPlot.h"
#include "RimSummaryCrossPlotCollection.h"

#include "RiuMainPlotWindow.h"

#include "cvfAssert.h"

#include <QAction>


CAF_CMD_SOURCE_INIT(RicNewSummaryCrossPlotFeature, "RicNewSummaryCrossPlotFeature");

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RicNewSummaryCrossPlotFeature::isCommandEnabled()
{
    return true;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicNewSummaryCrossPlotFeature::onActionTriggered(bool isChecked)
{
    RimProject* project = RiaApplication::instance()->project();
    CVF_ASSERT(project);

    auto dialog = RicEditSummaryPlotFeature::curveCreatorDialog();

    if (!dialog->isVisible())
    {
        dialog->show();
    }
    else
    {
        dialog->raise();
    }

    dialog->updateFromSummaryPlot(nullptr);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicNewSummaryCrossPlotFeature::setupActionLook(QAction* actionToSetup)
{
    actionToSetup->setText("New Summary Plot");
    actionToSetup->setIcon(QIcon(":/SummaryPlot16x16.png"));
}

//--------------------------------------------------------------------------------------------------
/// This method is not called from within this class, only by other classes
//--------------------------------------------------------------------------------------------------
RimSummaryCrossPlot* RicNewSummaryCrossPlotFeature::createNewSummaryPlot(RimSummaryCrossPlotCollection* summaryPlotColl, RimSummaryCase* summaryCase)
{
    RimSummaryCrossPlot* plot = new RimSummaryCrossPlot();
    summaryPlotColl->summaryCrossPlots().push_back(plot);

    plot->setDescription(QString("Summary Cross Plot %1").arg(summaryPlotColl->summaryCrossPlots.size()));

    summaryPlotColl->updateConnectedEditors();
    plot->loadDataAndUpdate();

    return plot;
}
