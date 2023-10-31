/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef QmitkDataNodeM2olieSaveAction_h
#define QmitkDataNodeM2olieSaveAction_h

#include <org_mitk_gui_qt_application_Export.h>

// qt widgets ext module
#include <QmitkNumberPropertySlider.h>
#include "QmitkAbstractDataNodeAction.h"

// qt
#include <QmitkFileSaveAction.h>

// mitk
#include <mitkDataStorage.h>

class MITK_QT_APP QmitkDataNodeM2olieSaveAction : public QmitkFileSaveAction
{
  Q_OBJECT

public:

  QmitkDataNodeM2olieSaveAction(berry::IWorkbenchWindow::Pointer window);
  QmitkDataNodeM2olieSaveAction(const QIcon& icon, berry::IWorkbenchWindow::Pointer window);
  void SetDataStorage(mitk::WeakPointer<mitk::DataStorage>& ds){m_DataStorage = ds;}

  // QmitkDataNodeM2olieSaveAction(QWidget* parent, berry::IWorkbenchPartSite::Pointer workbenchPartSite);
  // QmitkDataNodeM2olieSaveAction(QWidget* parent, berry::IWorkbenchPartSite* workbenchPartSite);

private Q_SLOTS:

  void OnActionTriggered();

protected:

  void InitializeAction();
  berry::IWorkbenchWindow::WeakPtr m_Window;

private:

  QmitkNumberPropertySlider* m_ComponentSlider;
  mitk::WeakPointer<mitk::DataStorage> m_DataStorage;

};

#endif
