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
#include <berryISelectionService.h>
#include <mitkDataNodeSelection.h>
#include <mitkIOUtil.h>

// qt
#include <QHBoxLayout>
#include <QLabel>
#include <itksys/SystemTools.hxx>

// QmitkDataNodeM2olieSaveAction::QmitkDataNodeM2olieSaveAction(QWidget* parent, berry::IWorkbenchPartSite::Pointer
// workbenchpartSite)
//   : QmitkFileSaveAction(parent)
//   , QmitkAbstractDataNodeAction(workbenchpartSite)
// {
//   InitializeAction();
// }

// QmitkDataNodeM2olieSaveAction::QmitkDataNodeM2olieSaveAction(QWidget* parent, berry::IWorkbenchPartSite*
// workbenchpartSite)
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

QmitkDataNodeM2olieSaveAction::QmitkDataNodeM2olieSaveAction(const QIcon &icon, berry::IWorkbenchWindow::Pointer window)
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
  std::string workDir = "/tmp/", batchDir = "/m2olie_test/", outputDir = "", savePath = "", fileSavePath = "";
  std::string name = "file.nrrd";
  std::string targetName = "manually_registered";

  itksys::SystemTools::GetEnv("WORKFLOW_DIR", workDir);
  itksys::SystemTools::GetEnv("BATCH_NAME", batchDir);
  itksys::SystemTools::GetEnv("UID_OUTPUT_DIR", outputDir);

  // auto any = [](mitk::BaseProperty * a, mitk::BaseProperty *b, mitk::BaseProperty *c = nullptr) -> mitk::BaseProperty *{
  //   if(a) return a;
  //   if(b) return b;
  //   if(c) return c;
  //   return nullptr;
  // };

  auto ds = m_DataStorage.Lock();
  if (outputDir.empty() && ds.IsNotNull())
  {
    MITK_INFO << "Search UID";
    if (auto allNodes = ds->GetAll())
      for (auto node : *allNodes)
      {
        if (node->GetName() == "fixed_image"){

          if (auto propUid = node->GetProperty("DICOM.0020.000E"))
          {
            outputDir = propUid->GetValueAsString();
            MITK_INFO << "Node with uid: " << outputDir;
          }
          else
          {
            MITK_ERROR << "Tag Not found";
          }

        }
      }
  }

  if (workDir.empty() || batchDir.empty() || outputDir.empty())
  {
    MITK_WARN << "The environment variables 'WORKFLOW_DIR'/'BATCH_NAME' is not set.";
    MITK_INFO << "Search manually for the M2olie output dir.";
    QmitkFileSaveAction::Run();
  }
  else
  {
    // join paths and make dir
    savePath = itksys::SystemTools::JoinPath({workDir, "/", batchDir, "/", outputDir, "/", targetName});
    itksys::SystemTools::MakeDirectory(savePath);
  }

  // create final file path
  fileSavePath = itksys::SystemTools::JoinPath({savePath, "/", name});
  fileSavePath = itksys::SystemTools::ConvertToOutputPath(fileSavePath);
  MITK_INFO << "M2olie process data is saved in: " << fileSavePath;

  // get the selection and save the file
  mitk::DataNodeSelection::ConstPointer selection =
    m_Window.Lock()->GetSelectionService()->GetSelection().Cast<const mitk::DataNodeSelection>();
  auto nodes = selection->GetSelectedDataNodes();
  if (nodes.size() == 1)
  {
    mitk::IOUtil::Save(nodes.front()->GetData(), fileSavePath.c_str());
    nodes.front()->SetName(nodes.front()->GetName() + " (saved)");
  }
}
