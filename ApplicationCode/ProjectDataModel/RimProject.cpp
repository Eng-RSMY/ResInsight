/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2011-     Statoil ASA
//  Copyright (C) 2013-     Ceetron Solutions AS
//  Copyright (C) 2011-2012 Ceetron AS
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

#include "RimProject.h"

#include "RiaApplication.h"
#include "RiaVersionInfo.h"

#include "RigEclipseCaseData.h"
#include "RigGridBase.h"
#include "RigSimulationWellCenterLineCalculator.h"
#include "RigSimulationWellCoordsAndMD.h"
#include "RigWellPath.h"

#include "RimCalcScript.h"
#include "RimSummaryCalculationCollection.h"
#include "RimCase.h"
#include "RimCaseCollection.h"
#include "RimCommandObject.h"
#include "RimContextCommandBuilder.h"
#include "RimDialogData.h"
#include "RimEclipseCase.h"
#include "RimEclipseCaseCollection.h"
#include "RimFlowPlotCollection.h"
#include "RimFormationNamesCollection.h"

#ifdef USE_PROTOTYPE_FEATURE_FRACTURES
#include "RimFractureTemplateCollection.h"
#endif // USE_PROTOTYPE_FEATURE_FRACTURES

#include "RimGeoMechCase.h"
#include "RimGeoMechModels.h"
#include "RimGridSummaryCase.h"
#include "RimIdenticalGridCaseGroup.h"
#include "RimMainPlotCollection.h"
#include "RimMultiSnapshotDefinition.h"
#include "RimObservedDataCollection.h"
#include "RimOilField.h"
#include "RimScriptCollection.h"
#include "RimSummaryCaseMainCollection.h"
#include "RimSummaryCrossPlotCollection.h"
#include "RimSummaryPlotCollection.h"
#include "RimView.h"
#include "RimViewLinker.h"
#include "RimViewLinkerCollection.h"
#include "RimWellLogPlotCollection.h"
#include "RimRftPlotCollection.h"
#include "RimPltPlotCollection.h"
#include "RimWellPathCollection.h"
#include "RimWellPathImport.h"
#include "RimWellPath.h"

#include "RiuMainWindow.h"
#include "RiuMainPlotWindow.h"

#include "OctaveScriptCommands/RicExecuteScriptForCasesFeature.h"

#include "cafCmdFeature.h"
#include "cafCmdFeatureManager.h"
#include "cafPdmUiTreeOrdering.h"
#include "cafCmdFeatureMenuBuilder.h"
#include "cvfBoundingBox.h"

#include <QDir>
#include <QMenu>


CAF_PDM_SOURCE_INIT(RimProject, "ResInsightProject");
//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimProject::RimProject(void)
{
    CAF_PDM_InitFieldNoDefault(&m_projectFileVersionString, "ProjectFileVersionString", "", "", "", "");
    m_projectFileVersionString.uiCapability()->setUiHidden(true);

    CAF_PDM_InitField(&nextValidCaseId, "NextValidCaseId", 0, "Next Valid Case ID", "", "" ,"");
    nextValidCaseId.uiCapability()->setUiHidden(true);

    CAF_PDM_InitField(&nextValidCaseGroupId, "NextValidCaseGroupId", 0, "Next Valid Case Group ID", "", "" ,"");
    nextValidCaseGroupId.uiCapability()->setUiHidden(true);

    CAF_PDM_InitFieldNoDefault(&oilFields, "OilFields", "Oil Fields",  "", "", "");
    oilFields.uiCapability()->setUiHidden(true);

    CAF_PDM_InitFieldNoDefault(&scriptCollection, "ScriptCollection", "Scripts", ":/Default.png", "", "");
    scriptCollection.uiCapability()->setUiHidden(true);

    CAF_PDM_InitFieldNoDefault(&wellPathImport, "WellPathImport", "WellPathImport", "", "", "");
    wellPathImport = new RimWellPathImport();
    wellPathImport.uiCapability()->setUiHidden(true);
    wellPathImport.uiCapability()->setUiTreeChildrenHidden(true);

    CAF_PDM_InitFieldNoDefault(&mainPlotCollection, "MainPlotCollection", "Plots", "", "", "");
    mainPlotCollection.uiCapability()->setUiHidden(true);

    CAF_PDM_InitFieldNoDefault(&viewLinkerCollection, "LinkedViews", "Linked Views (field in RimProject", ":/chain.png", "", "");
    viewLinkerCollection.uiCapability()->setUiHidden(true);
    viewLinkerCollection = new RimViewLinkerCollection;

    CAF_PDM_InitFieldNoDefault(&calculationCollection, "CalculationCollection", "Calculation Collection", "", "", "");
    calculationCollection = new RimSummaryCalculationCollection;

    CAF_PDM_InitFieldNoDefault(&commandObjects, "CommandObjects", "CommandObjects", "", "", "");
    //wellPathImport.uiCapability()->setUiHidden(true);

    CAF_PDM_InitFieldNoDefault(&multiSnapshotDefinitions, "MultiSnapshotDefinitions", "MultiSnapshotDefinitions", "", "", "");

    CAF_PDM_InitFieldNoDefault(&mainWindowTreeViewState, "TreeViewState", "",  "", "", "");
    mainWindowTreeViewState.uiCapability()->setUiHidden(true);
    CAF_PDM_InitFieldNoDefault(&mainWindowCurrentModelIndexPath, "TreeViewCurrentModelIndexPath", "",  "", "", "");
    mainWindowCurrentModelIndexPath.uiCapability()->setUiHidden(true);

    CAF_PDM_InitFieldNoDefault(&plotWindowTreeViewState, "PlotWindowTreeViewState", "", "", "", "");
    plotWindowTreeViewState.uiCapability()->setUiHidden(true);
    CAF_PDM_InitFieldNoDefault(&plotWindowCurrentModelIndexPath, "PlotWindowTreeViewCurrentModelIndexPath", "", "", "", "");
    plotWindowCurrentModelIndexPath.uiCapability()->setUiHidden(true);

    CAF_PDM_InitField(&m_show3DWindow, "show3DWindow", true, "Show 3D Window", "", "", "");
    m_show3DWindow.uiCapability()->setUiHidden(true);

    CAF_PDM_InitField(&m_showPlotWindow, "showPlotWindow", false, "Show Plot Window", "", "", "");
    m_showPlotWindow.uiCapability()->setUiHidden(true);

    CAF_PDM_InitFieldNoDefault(&m_dialogData, "DialogData", "DialogData", "", "", "");
    m_dialogData = new RimDialogData();
    m_dialogData.uiCapability()->setUiHidden(true);
    m_dialogData.uiCapability()->setUiTreeChildrenHidden(true);

    // Obsolete fields. The content is moved to OilFields and friends
    CAF_PDM_InitFieldNoDefault(&casesObsolete, "Reservoirs", "",  "", "", "");
    casesObsolete.uiCapability()->setUiHidden(true);
    casesObsolete.xmlCapability()->setIOWritable(false); // read but not write, they will be moved into RimAnalysisGroups
    CAF_PDM_InitFieldNoDefault(&caseGroupsObsolete, "CaseGroups", "",  "", "", "");
    caseGroupsObsolete.uiCapability()->setUiHidden(true);
    caseGroupsObsolete.xmlCapability()->setIOWritable(false); // read but not write, they will be moved into RimAnalysisGroups

    // Initialization

    scriptCollection = new RimScriptCollection();
    scriptCollection->directory.uiCapability()->setUiHidden(true);
    scriptCollection->uiCapability()->setUiName("Scripts");
    scriptCollection->uiCapability()->setUiIcon(QIcon(":/Default.png"));

    mainPlotCollection = new RimMainPlotCollection();

    // For now, create a default first oilfield that contains the rest of the project
    oilFields.push_back(new RimOilField);

    initScriptDirectories();

    this->setUiHidden(true);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimProject::~RimProject(void)
{
    close();

    oilFields.deleteAllChildObjects();
    if (scriptCollection()) delete scriptCollection();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::close()
{
    if (mainPlotCollection()) 
    {
        mainPlotCollection()->deleteAllContainedObjects();
    }

    oilFields.deleteAllChildObjects();
    oilFields.push_back(new RimOilField);

    casesObsolete.deleteAllChildObjects();
    caseGroupsObsolete.deleteAllChildObjects();

    wellPathImport->regions().deleteAllChildObjects();

    commandObjects.deleteAllChildObjects();

    multiSnapshotDefinitions.deleteAllChildObjects();

    calculationCollection->deleteAllContainedObjects();

    delete viewLinkerCollection->viewLinker();
    viewLinkerCollection->viewLinker = NULL;

    fileName = "";

    nextValidCaseId = 0;
    nextValidCaseGroupId = 0;
    mainWindowCurrentModelIndexPath = "";
    mainWindowTreeViewState = "";
    plotWindowCurrentModelIndexPath = "";
    plotWindowTreeViewState = "";
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::initScriptDirectories()
{
    //
    // TODO : Must store content of scripts in project file and notify user if stored content is different from disk on execute and edit
    // 
    RiaApplication* app = RiaApplication::instance();
    QString scriptDirectories = app->scriptDirectories();

    this->setScriptDirectories(scriptDirectories);

    // Find largest used caseId read from file and make sure all cases have a valid caseId
    {
        int largestId = -1;

        std::vector<RimCase*> cases;
        allCases(cases);
    
        for (size_t i = 0; i < cases.size(); i++)
        {
            if (cases[i]->caseId > largestId)
            {
                largestId = cases[i]->caseId;
            }
        }

        if (largestId > this->nextValidCaseId)
        {
            this->nextValidCaseId = largestId + 1;
        }

        // Assign case Id to cases with an invalid case Id
        for (size_t i = 0; i < cases.size(); i++)
        {
            if (cases[i]->caseId < 0)
            {
                assignCaseIdToCase(cases[i]);
            }
        }
    }

    // Find largest used groupId read from file and make sure all groups have a valid groupId
    RimEclipseCaseCollection* analysisModels = activeOilField() ? activeOilField()->analysisModels() : NULL;
    if (analysisModels)
    {
        int largestGroupId = -1;
        
        for (size_t i = 0; i < analysisModels->caseGroups().size(); i++)
        {
            RimIdenticalGridCaseGroup* cg = analysisModels->caseGroups()[i];

            if (cg->groupId > largestGroupId)
            {
                largestGroupId = cg->groupId;
            }
        }

        if (largestGroupId > this->nextValidCaseGroupId)
        {
            this->nextValidCaseGroupId = largestGroupId + 1;
        }

        // Assign group Id to groups with an invalid Id
        for (size_t i = 0; i < analysisModels->caseGroups().size(); i++)
        {
            RimIdenticalGridCaseGroup* cg = analysisModels->caseGroups()[i];

            if (cg->groupId < 0)
            {
                assignIdToCaseGroup(cg);
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::initAfterRead()
{
    initScriptDirectories();

    // Create an empty oil field in case the project did not contain one
    if (oilFields.size() < 1)
    {
        oilFields.push_back(new RimOilField);
    }

    // Handle old project files with obsolete structure.
    // Move caseGroupsObsolete and casesObsolete to oilFields()[idx]->analysisModels()
    RimEclipseCaseCollection* analysisModels = activeOilField() ? activeOilField()->analysisModels() : NULL;
    bool movedOneRimIdenticalGridCaseGroup = false;
    for (size_t cgIdx = 0; cgIdx < caseGroupsObsolete.size(); ++cgIdx)
    {
        RimIdenticalGridCaseGroup* sourceCaseGroup = caseGroupsObsolete[cgIdx];
        if (analysisModels)
        {
            analysisModels->caseGroups.push_back(sourceCaseGroup);
            //printf("Moved m_project->caseGroupsObsolete[%i] to first oil fields analysis models\n", cgIdx);
            movedOneRimIdenticalGridCaseGroup = true; // moved at least one so assume the others will be moved too...
        }
    }

    if (movedOneRimIdenticalGridCaseGroup)
    {
        caseGroupsObsolete.clear();
    }

    bool movedOneRimCase = false;
    for (size_t cIdx = 0; cIdx < casesObsolete().size(); ++cIdx)
    {
        if (analysisModels)
        {
            RimEclipseCase* sourceCase = casesObsolete[cIdx];
            casesObsolete.set(cIdx, NULL);
            analysisModels->cases.push_back(sourceCase);
            //printf("Moved m_project->casesObsolete[%i] to first oil fields analysis models\n", cIdx);
            movedOneRimCase = true; // moved at least one so assume the others will be moved too...
        }
    }

    if (movedOneRimCase)
    {
        casesObsolete.clear();
    }
    
    if (casesObsolete().size() > 0 || caseGroupsObsolete.size() > 0)
    {
        //printf("RimProject::initAfterRead: Was not able to move all cases (%i left) or caseGroups (%i left) from Project to analysisModels", 
          //  casesObsolete().size(), caseGroupsObsolete.size());
    }

    // Set project pointer to each well path
    for (size_t oilFieldIdx = 0; oilFieldIdx < oilFields().size(); oilFieldIdx++)
    {
        RimOilField* oilField = oilFields[oilFieldIdx];
        if (oilField == NULL || oilField->wellPathCollection == NULL) continue;
    }
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::setupBeforeSave()
{
    m_show3DWindow = RiuMainWindow::instance()->isVisible();

    if (RiaApplication::instance()->mainPlotWindow() &&
        RiaApplication::instance()->mainPlotWindow()->isVisible())
    {
        m_showPlotWindow = true;
    }
    else
    {
        m_showPlotWindow = false;
    }

    m_projectFileVersionString = STRPRODUCTVER;
}

//--------------------------------------------------------------------------------------------------
/// Support list of multiple script paths divided by ';'
//--------------------------------------------------------------------------------------------------
void RimProject::setScriptDirectories(const QString& scriptDirectories)
{
    scriptCollection->calcScripts().deleteAllChildObjects();
    scriptCollection->subDirectories().deleteAllChildObjects();

    QStringList pathList = scriptDirectories.split(';');
    foreach(QString path, pathList)
    {
        QDir dir(path);
        if (!path.isEmpty() && dir.exists() && dir.isReadable())
        {
            RimScriptCollection* sharedScriptLocation = new RimScriptCollection;
            sharedScriptLocation->directory = path;
            sharedScriptLocation->setUiName(dir.dirName());

            sharedScriptLocation->readContentFromDisc();

            scriptCollection->subDirectories.push_back(sharedScriptLocation);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimProject::projectFileVersionString() const
{
    return m_projectFileVersionString;
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::setProjectFileNameAndUpdateDependencies(const QString& fileName)
{
    // Extract the filename of the project file when it was saved 
    QString oldProjectFileName =  this->fileName;
    // Replace with the new actual filename
    this->fileName = fileName;

    QFileInfo fileInfo(fileName);
    QString newProjectPath = fileInfo.path();

    QFileInfo fileInfoOld(oldProjectFileName);
    QString oldProjectPath = fileInfoOld.path();

    // Loop over all cases and update file path

    std::vector<RimCase*> cases;
    allCases(cases);
    for (size_t i = 0; i < cases.size(); i++)
    {
        cases[i]->updateFilePathsFromProjectPath(newProjectPath, oldProjectPath);
    }

    // Update path to well path file cache
    for(RimOilField* oilField: oilFields)
    {
        if (oilField == NULL) continue;
        if (oilField->wellPathCollection() != NULL)
        {
            oilField->wellPathCollection()->updateFilePathsFromProjectPath(newProjectPath, oldProjectPath);
        }
        if (oilField->formationNamesCollection() != NULL)
        {
            oilField->formationNamesCollection()->updateFilePathsFromProjectPath(newProjectPath, oldProjectPath);
        }
        if (oilField->summaryCaseMainCollection() != NULL) {
            oilField->summaryCaseMainCollection()->updateFilePathsFromProjectPath(newProjectPath, oldProjectPath);
        }

#ifdef USE_PROTOTYPE_FEATURE_FRACTURES
        CVF_ASSERT(oilField->fractureDefinitionCollection());
        oilField->fractureDefinitionCollection()->updateFilePathsFromProjectPath(newProjectPath, oldProjectPath);
#endif // USE_PROTOTYPE_FEATURE_FRACTURES
    }


    wellPathImport->updateFilePaths();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::assignCaseIdToCase(RimCase* reservoirCase)
{
    if (reservoirCase)
    {
        reservoirCase->caseId = nextValidCaseId;

        nextValidCaseId = nextValidCaseId + 1;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::assignIdToCaseGroup(RimIdenticalGridCaseGroup* caseGroup)
{
    if (caseGroup)
    {
        caseGroup->groupId = nextValidCaseGroupId;

        nextValidCaseGroupId = nextValidCaseGroupId + 1;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::allCases(std::vector<RimCase*>& cases)
{
    for (size_t oilFieldIdx = 0; oilFieldIdx < oilFields().size(); oilFieldIdx++)
    {
        RimOilField* oilField = oilFields[oilFieldIdx];
        if (!oilField) continue;

        RimEclipseCaseCollection* analysisModels = oilField->analysisModels();
        if (analysisModels ) 
        {
            for (size_t caseIdx = 0; caseIdx < analysisModels->cases.size(); caseIdx++)
            {
                cases.push_back(analysisModels->cases[caseIdx]);
            }
            for (size_t cgIdx = 0; cgIdx < analysisModels->caseGroups.size(); cgIdx++)
            {
                // Load the Main case of each IdenticalGridCaseGroup
                RimIdenticalGridCaseGroup* cg = analysisModels->caseGroups[cgIdx];
                if (cg == NULL) continue;

                if (cg->statisticsCaseCollection())
                {
                    for (size_t caseIdx = 0; caseIdx < cg->statisticsCaseCollection()->reservoirs.size(); caseIdx++)
                    {
                        cases.push_back(cg->statisticsCaseCollection()->reservoirs[caseIdx]);
                    }
                }
                if (cg->caseCollection())
                {
                    for (size_t caseIdx = 0; caseIdx < cg->caseCollection()->reservoirs.size(); caseIdx++)
                    {
                        cases.push_back(cg->caseCollection()->reservoirs[caseIdx]);
                    }
                }
            }
        }

        RimGeoMechModels* geomModels = oilField->geoMechModels();
        if (geomModels)
        {
            for (size_t caseIdx = 0; caseIdx < geomModels->cases.size(); caseIdx++)
            {
                cases.push_back(geomModels->cases[caseIdx]);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::allSummaryCases(std::vector<RimSummaryCase*>& sumCases)
{
    sumCases.clear();
    for (RimOilField* oilField: oilFields)
    {
        if(!oilField) continue;
        RimSummaryCaseMainCollection* sumCaseMainColl = oilField->summaryCaseMainCollection();
        if(sumCaseMainColl)
        {
            std::vector<RimSummaryCase*> allSummaryCases = sumCaseMainColl->allSummaryCases();
            sumCases.insert(sumCases.end(), allSummaryCases.begin(), allSummaryCases.end());
        }

        auto observedDataColl = oilField->observedDataCollection();
        if (observedDataColl != nullptr && observedDataColl->allObservedData().size() > 0)
        {
            auto observedData = observedDataColl->allObservedData();
            sumCases.insert(sumCases.end(), observedData.begin(), observedData.end());
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::allObservedData(std::vector<RimSummaryCase*>& observedData)
{
    for (RimOilField* oilField : oilFields)
    {
        if (!oilField) continue;
        RimObservedDataCollection* observedDataCollection = oilField->observedDataCollection();
        if (observedDataCollection)
        {
            observedData.clear();
            std::vector<RimSummaryCase*> allObservedData = observedDataCollection->allObservedData();
            observedData.insert(observedData.end(), allObservedData.begin(), allObservedData.end());
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::allNotLinkedViews(std::vector<RimView*>& views)
{
    std::vector<RimCase*> cases;
    allCases(cases);

    std::vector<RimView*> alreadyLinkedViews;
    if (viewLinkerCollection->viewLinker())
    {
        viewLinkerCollection->viewLinker()->allViews(alreadyLinkedViews);
    }

    for (size_t caseIdx = 0; caseIdx < cases.size(); caseIdx++)
    {
        RimCase* rimCase = cases[caseIdx];
        if (!rimCase) continue;

        std::vector<RimView*> caseViews = rimCase->views();
        for (size_t viewIdx = 0; viewIdx < caseViews.size(); viewIdx++)
        {
            bool isLinked = false;
            for (size_t lnIdx = 0; lnIdx < alreadyLinkedViews.size(); lnIdx++)
            {
                if (caseViews[viewIdx] == alreadyLinkedViews[lnIdx])
                {
                    isLinked = true;
                }
            }
            if (!isLinked)
            {
                views.push_back(caseViews[viewIdx]);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::allVisibleViews(std::vector<RimView*>& views)
{
    std::vector<RimCase*> cases;
    allCases(cases);
    
    for (size_t caseIdx = 0; caseIdx < cases.size(); caseIdx++)
    {
        RimCase* rimCase = cases[caseIdx];
        if (!rimCase) continue;

        std::vector<RimView*> caseViews = rimCase->views();
        for (size_t viewIdx = 0; viewIdx < caseViews.size(); viewIdx++)
        {
            if (caseViews[viewIdx] && caseViews[viewIdx]->viewer())
            {
                views.push_back(caseViews[viewIdx]);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::createDisplayModelAndRedrawAllViews()
{
    std::vector<RimCase*> cases;
    allCases(cases);
    for (size_t caseIdx = 0; caseIdx < cases.size(); caseIdx++)
    {
        RimCase* rimCase = cases[caseIdx];
        if (rimCase == NULL) continue;
        std::vector<RimView*> views = rimCase->views();

        for (size_t viewIdx = 0; viewIdx < views.size(); viewIdx++)
        {
            views[viewIdx]->scheduleCreateDisplayModelAndRedraw();
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::allOilFields(std::vector<RimOilField*>& oilFields)
{
    oilFields.clear();
    for (const auto& oilField : this->oilFields)
    {
        oilFields.push_back(oilField);
    }
}

//--------------------------------------------------------------------------------------------------
/// Currently there will be only one oil field in Resinsight, so return hardcoded first oil field
/// from the RimOilField collection.
//--------------------------------------------------------------------------------------------------
RimOilField* RimProject::activeOilField()
{
    CVF_ASSERT(oilFields.size() == 1);
  
    return oilFields[0];
}

//--------------------------------------------------------------------------------------------------
/// Currently there will be only one oil field in Resinsight, so return hardcoded first oil field
/// from the RimOilField collection.
//--------------------------------------------------------------------------------------------------
const RimOilField * RimProject::activeOilField() const
{
    CVF_ASSERT(oilFields.size() == 1);
  
    return oilFields[0];
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::computeUtmAreaOfInterest()
{
    std::vector<RimCase*> cases;
    allCases(cases);

    cvf::BoundingBox projectBB;

    for (size_t i = 0; i < cases.size(); i++)
    {
        RimEclipseCase* rimCase = dynamic_cast<RimEclipseCase*>(cases[i]);

        if (rimCase && rimCase->eclipseCaseData())
        {
            for (size_t gridIdx = 0; gridIdx < rimCase->eclipseCaseData()->gridCount(); gridIdx++ )
            {
                RigGridBase* rigGrid = rimCase->eclipseCaseData()->grid(gridIdx);

                projectBB.add(rigGrid->boundingBox());
            }
        }
        else
        {
            // Todo : calculate BBox of GeoMechCase
        }
    }

    if (projectBB.isValid())
    {
        double north, south, east, west;

        north = projectBB.max().y();
        south = projectBB.min().y();

        west = projectBB.min().x();
        east = projectBB.max().x();

        wellPathImport->north = north;
        wellPathImport->south = south;
        wellPathImport->east = east;
        wellPathImport->west = west;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::actionsBasedOnSelection(QMenu& contextMenu)
{
    caf::CmdFeatureMenuBuilder menuBuilder = RimContextCommandBuilder::commandsFromSelection();

    menuBuilder.appendToMenu(&contextMenu);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimProject::show3DWindow() const
{
    return m_show3DWindow;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimProject::showPlotWindow() const
{
    return m_showPlotWindow;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::reloadCompletionTypeResultsInAllViews()
{
    createDisplayModelAndRedrawAllViews();
    RiaApplication::instance()->scheduleRecalculateCompletionTypeAndRedrawAllViews();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimDialogData* RimProject::dialogData() const
{
    return m_dialogData;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RimEclipseCase*> RimProject::eclipseCases() const
{
    std::vector<RimEclipseCase*> allCases;
    for (const auto& oilField : oilFields)
    {
        const auto& cases = oilField->analysisModels->cases;
        allCases.insert(allCases.end(), cases.begin(), cases.end());
    }
    return allCases;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<QString> RimProject::simulationWellNames() const
{
    std::set<QString> wellNames;

    for (RimOilField* oilField : oilFields)
    {
        auto analysisCaseColl = oilField->analysisModels();
        for (RimEclipseCase* eclCase : analysisCaseColl->cases())
        {
            const auto& eclData = eclCase->eclipseCaseData();
            if (eclData == nullptr) continue;

            const auto names = eclData->simulationWellNames();
            wellNames.insert(names.begin(), names.end());
        }
    }
    return std::vector<QString>(wellNames.begin(), wellNames.end());
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<const RigWellPath*> RimProject::simulationWellBranches(const QString& simWellName)
{
    // Find first case containing the specified simulation well
    auto simCases = eclipseCases();
    auto caseItr = std::find_if(simCases.begin(), simCases.end(), [&simWellName](const RimEclipseCase* eclCase) {
        const auto& eclData = eclCase->eclipseCaseData();
        return eclData != nullptr && eclData->hasSimulationWell(simWellName);
    });
    RimEclipseCase* eclipseCase = caseItr != simCases.end() ? *caseItr : nullptr;
    RigEclipseCaseData* eclCaseData = eclipseCase != nullptr ? eclipseCase->eclipseCaseData() : nullptr;
    return eclCaseData != nullptr ?
        eclCaseData->simulationWellBranches(simWellName) :
        std::vector<const RigWellPath*>();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellPath* RimProject::wellPathFromSimWellName(const QString& simWellName, int branchIndex)
{
    std::vector<RimWellPath*> paths;
    for (RimWellPath* const path : allWellPaths())
    {
        if (QString::compare(path->associatedSimulationWellName(), simWellName) == 0 &&
            (branchIndex < 0 || path->associatedSimulationWellBranch() == branchIndex))
        {
            return path;
        }
    }
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellPath* RimProject::wellPathByName(const QString& wellPathName) const
{
    for (RimWellPath* const path : allWellPaths())
    {
        if (path->name() == wellPathName) return path;
    }
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RimWellPath*> RimProject::allWellPaths() const
{
    std::vector<RimWellPath*> paths;
    for (const auto& oilField : oilFields())
    {
        auto wellPathColl = oilField->wellPathCollection();
        for (const auto& path : wellPathColl->wellPaths)
        {
            paths.push_back(path);
        }
    }
    return paths;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RimGeoMechCase*> RimProject::geoMechCases() const
{
    std::vector<RimGeoMechCase*> cases;

    for (size_t oilFieldIdx = 0; oilFieldIdx < oilFields().size(); oilFieldIdx++)
    {
        RimOilField* oilField = oilFields[oilFieldIdx];
        if (!oilField) continue;

        RimGeoMechModels* geomModels = oilField->geoMechModels();
        if (geomModels)
        {
            for (size_t caseIdx = 0; caseIdx < geomModels->cases.size(); caseIdx++)
            {
                cases.push_back(geomModels->cases[caseIdx]);
            }
        }
    }
    return cases;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::reloadCompletionTypeResultsForEclipseCase(RimEclipseCase* eclipseCase)
{
    std::vector<RimView*> views = eclipseCase->views();

    for (size_t viewIdx = 0; viewIdx < views.size(); viewIdx++)
    {
        views[viewIdx]->scheduleCreateDisplayModelAndRedraw();
    }

    RiaApplication::instance()->scheduleRecalculateCompletionTypeAndRedrawEclipseCase(eclipseCase);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimProject::defineUiTreeOrdering(caf::PdmUiTreeOrdering& uiTreeOrdering, QString uiConfigName /*= ""*/)
{
    if (uiConfigName == "PlotWindow")
    {
        RimOilField* oilField = activeOilField();
        if (oilField)
        {
            if (oilField->summaryCaseMainCollection())
            {
                uiTreeOrdering.add( oilField->summaryCaseMainCollection() );
            }
            if (oilField->observedDataCollection())
            {
                uiTreeOrdering.add( oilField->observedDataCollection() );
            }
        }

        if (mainPlotCollection)
        {
           if (mainPlotCollection->summaryPlotCollection())
            {
                uiTreeOrdering.add(mainPlotCollection->summaryPlotCollection());
            }

           if (mainPlotCollection->summaryCrossPlotCollection())
           {
               uiTreeOrdering.add(mainPlotCollection->summaryCrossPlotCollection());
           }

           if (mainPlotCollection->wellLogPlotCollection())
            {
                uiTreeOrdering.add(mainPlotCollection->wellLogPlotCollection());
            }

           if (mainPlotCollection->rftPlotCollection())
           {
               uiTreeOrdering.add(mainPlotCollection->rftPlotCollection());
           }
           
           if (mainPlotCollection->pltPlotCollection())
           {
               uiTreeOrdering.add(mainPlotCollection->pltPlotCollection());
           }

           if (mainPlotCollection->flowPlotCollection())
            {
                uiTreeOrdering.add(mainPlotCollection->flowPlotCollection());
            }
        }
    }
    else
    {
        if (viewLinkerCollection()->viewLinker())
        {
            // Use object instead of field to avoid duplicate entries in the tree view
            uiTreeOrdering.add(viewLinkerCollection());
        }

        RimOilField* oilField = activeOilField();
        if (oilField)
        {
            if (oilField->analysisModels())                 uiTreeOrdering.add(oilField->analysisModels());
            if (oilField->geoMechModels())                  uiTreeOrdering.add(oilField->geoMechModels());
            if (oilField->wellPathCollection())             uiTreeOrdering.add(oilField->wellPathCollection());

#ifdef USE_PROTOTYPE_FEATURE_FRACTURES
            if (oilField->fractureDefinitionCollection())   uiTreeOrdering.add(oilField->fractureDefinitionCollection());
#endif // USE_PROTOTYPE_FEATURE_FRACTURES

            if (oilField->formationNamesCollection())       uiTreeOrdering.add(oilField->formationNamesCollection());
        }

        uiTreeOrdering.add(scriptCollection());
    }
        
    uiTreeOrdering.skipRemainingChildren(true);
}

