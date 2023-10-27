/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include <QmitkDataNodeM2olieSaveAction.h>

// mitk core
#include <mitkDataNode.h>
#include <mitkImage.h>
#include <mitkRenderingManager.h>

// mitk gui common plugin
#include <mitkDataNodeSelection.h>
#include <mitkIOUtil.h>
#include <berryISelectionService.h>

// qt
#include <QHBoxLayout>
#include <QLabel>
#include <itksys/SystemTools.hxx>

// QmitkDataNodeM2olieSaveAction::QmitkDataNodeM2olieSaveAction(QWidget* parent, berry::IWorkbenchPartSite::Pointer workbenchpartSite)
//   : QmitkFileSaveAction(parent)
//   , QmitkAbstractDataNodeAction(workbenchpartSite)
// {
//   InitializeAction();
// }

// QmitkDataNodeM2olieSaveAction::QmitkDataNodeM2olieSaveAction(QWidget* parent, berry::IWorkbenchPartSite* workbenchpartSite)
//   : QmitkFileSaveAction(parent)
//   , QmitkAbstractDataNodeAction(berry::IWorkbenchPartSite::Pointer(workbenchpartSite))
// {
//   InitializeAction();
// }


QmitkDataNodeM2olieSaveAction::QmitkDataNodeM2olieSaveAction(berry::IWorkbenchWindow::Pointer window)
  : QmitkFileSaveAction(window)
{
  InitializeAction();
  m_Window = window;
}

QmitkDataNodeM2olieSaveAction::QmitkDataNodeM2olieSaveAction(const QIcon& icon, berry::IWorkbenchWindow::Pointer window)
  : QmitkFileSaveAction(icon, window)
{
  InitializeAction();
  m_Window = window;
}

void QmitkDataNodeM2olieSaveAction::InitializeAction()
{
    setText("M2OLIE: Save Registration");
    disconnect(this);
    connect(this, &QmitkDataNodeM2olieSaveAction::triggered, this, &QmitkDataNodeM2olieSaveAction::OnActionTriggered);
}


void QmitkDataNodeM2olieSaveAction::OnActionTriggered()
{
  std::string workDir, batchDir, altDir, savePath, fileSavePath, name;
  name = "file.nrrd";
  
  itksys::SystemTools::GetEnv("WORKFLOW_DIR", workDir);
  itksys::SystemTools::GetEnv("BATCH_NAME", batchDir);
  itksys::SystemTools::GetEnv("M2OLIE_OUTPUT_DIR", altDir);

  const char * targetName = "manually_registered";
   

  if(workDir.empty() || batchDir.empty()){
    MITK_WARN << "The environment variables 'WORKFLOW_DIR'/'BATCH_NAME' is not set.";
    
    if(altDir.empty()){
        MITK_WARN << "The environment variables 'M2OLIE_OUTPUT_DIR' is not set.";
        MITK_INFO << "Search manually for the M2olie output dir.";
        QmitkFileSaveAction::Run();
    }else{
      savePath = itksys::SystemTools::JoinPath({altDir, "/", targetName});
      itksys::SystemTools::MakeDirectory(savePath);
      MITK_WARN << "The environment variables 'M2OLIE_OUTPUT_DIR' is used.";
    }

  }else{
    // join paths and make dir
    savePath = itksys::SystemTools::JoinPath({workDir,"/", batchDir, "/", targetName});
    itksys::SystemTools::MakeDirectory(savePath);
  }
  
  // create final file path
  fileSavePath = itksys::SystemTools::JoinPath({savePath, "/", name});
  fileSavePath = itksys::SystemTools::ConvertToOutputPath(fileSavePath);
  MITK_INFO << "M2olie process data is saved in: " << fileSavePath;
  
  // get the selection and save the file
  mitk::DataNodeSelection::ConstPointer selection = m_Window.Lock()->GetSelectionService()->GetSelection().Cast<const mitk::DataNodeSelection>();
  auto nodes = selection->GetSelectedDataNodes();
  if(nodes.size() == 1){
    mitk::IOUtil::Save(nodes.front()->GetData(), fileSavePath.c_str());
  }
}
