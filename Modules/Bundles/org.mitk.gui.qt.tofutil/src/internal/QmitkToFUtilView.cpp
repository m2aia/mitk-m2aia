/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Language:  C++
Date:      $Date: 2010-03-31 16:40:27 +0200 (Mi, 31 Mrz 2010) $
Version:   $Revision: 21975 $ 

Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

// Qmitk
#include "QmitkToFUtilView.h"
#include <QmitkStdMultiWidget.h>
#include <QmitkTextOverlay.h>

// Qt
#include <QMessageBox>
#include <QString>

// MITK
#include <mitkBaseRenderer.h>
#include <mitkGlobalInteraction.h>
#include <mitkVtkRepresentationProperty.h>

#include <mitkLookupTableProperty.h>
#include <mitkToFDistanceImageToPointSetFilter.h>

// VTK
#include <vtkCamera.h>

// ITK
#include <itkCommand.h>

const std::string QmitkToFUtilView::VIEW_ID = "org.mitk.views.tofutil";

QmitkToFUtilView::QmitkToFUtilView()
: QmitkFunctionality()
, m_Controls(NULL), m_MultiWidget( NULL )
, m_MitkDistanceImage(NULL), m_MitkAmplitudeImage(NULL), m_MitkIntensityImage(NULL), m_Surface(NULL)
, m_ToFImageGrabber(NULL)
, m_TransferFunctionInitialized(false), m_ImageSequence(0), m_VideoEnabled(false)
, m_DistanceImageNode(NULL), m_AmplitudeImageNode(NULL), m_IntensityImageNode(NULL), m_SurfaceNode(NULL) // TODO necessary?
, m_QmitkToFImageBackground1(NULL), m_QmitkToFImageBackground2(NULL), m_QmitkToFImageBackground3(NULL) // TODO necessary?
{
  this->m_Frametimer = new QTimer(this);

  this->m_ToFDistanceImageToSurfaceFilter = mitk::ToFDistanceImageToSurfaceFilter::New();
  this->m_ToFCompositeFilter = mitk::ToFCompositeFilter::New();
  this->m_ToFVisualizationFilter = mitk::ToFVisualizationFilter::New();
  this->m_ToFImageRecorder = mitk::ToFImageRecorder::New();
  this->m_ToFSurfaceVtkMapper3D = mitk::ToFSurfaceVtkMapper3D::New();
}

QmitkToFUtilView::~QmitkToFUtilView()
{
}

void QmitkToFUtilView::CreateQtPartControl( QWidget *parent )
{
  // build up qt view, unless already done
  if ( !m_Controls )
  {
    // create GUI widgets from the Qt Designer's .ui file
    m_Controls = new Ui::QmitkToFUtilViewControls;
    m_Controls->setupUi( parent );

    connect(m_Frametimer, SIGNAL(timeout()), this, SLOT(OnUpdateCamera()));
    connect( (QObject*)(m_Controls->m_ToFConnectionWidget), SIGNAL(ToFCameraConnected()), this, SLOT(OnToFCameraConnected()) );
    connect( (QObject*)(m_Controls->m_ToFConnectionWidget), SIGNAL(ToFCameraDisconnected()), this, SLOT(OnToFCameraDisconnected()) );
    connect( (QObject*)(m_Controls->m_ToFConnectionWidget), SIGNAL(ToFCameraSelected(const QString)), this, SLOT(OnToFCameraSelected(const QString)) );
    connect( (QObject*)(m_Controls->m_ToFRecorderWidget), SIGNAL(ToFCameraStarted()), this, SLOT(OnToFCameraStarted()) );
    connect( (QObject*)(m_Controls->m_ToFRecorderWidget), SIGNAL(ToFCameraStopped()), this, SLOT(OnToFCameraStopped()) );
    connect( (QObject*)(m_Controls->m_TextureCheckBox), SIGNAL(toggled(bool)), this, SLOT(OnTextureCheckBoxChecked(bool)) );
    connect( (QObject*)(m_Controls->m_VideoTextureCheckBox), SIGNAL(toggled(bool)), this, SLOT(OnVideoTextureCheckBoxChecked(bool)) );
  }
}


void QmitkToFUtilView::StdMultiWidgetAvailable (QmitkStdMultiWidget &stdMultiWidget)
{
  m_MultiWidget = &stdMultiWidget;
}


void QmitkToFUtilView::StdMultiWidgetNotAvailable()
{
  m_MultiWidget = NULL;
}

void QmitkToFUtilView::Activated()
{
  QmitkFunctionality::Activated();
  // configure views
  m_MultiWidget->SetWidgetPlanesVisibility(false);
  m_MultiWidget->mitkWidget1->GetSliceNavigationController()->SetDefaultViewDirection(mitk::SliceNavigationController::Transversal);
  m_MultiWidget->mitkWidget1->GetSliceNavigationController()->SliceLockedOn();
  m_MultiWidget->mitkWidget2->GetSliceNavigationController()->SetDefaultViewDirection(mitk::SliceNavigationController::Transversal);
  m_MultiWidget->mitkWidget2->GetSliceNavigationController()->SliceLockedOn();
  m_MultiWidget->mitkWidget3->GetSliceNavigationController()->SetDefaultViewDirection(mitk::SliceNavigationController::Transversal);
  m_MultiWidget->mitkWidget3->GetSliceNavigationController()->SliceLockedOn();

  //m_Controls->m_ToFImageConverterWidget->Initialize(this->GetDefaultDataStorage(), m_MultiWidget);
  m_Controls->m_ToFVisualisationSettingsWidget->Initialize(this->GetDefaultDataStorage(), m_MultiWidget);
  m_Controls->m_ToFVisualisationSettingsWidget->SetParameter(this->m_ToFVisualizationFilter);

  m_Controls->m_ToFCompositeFilterWidget->SetToFCompositeFilter(this->m_ToFCompositeFilter);

  if (this->m_ToFImageGrabber.IsNull())
  {
    m_Controls->m_ToFRecorderWidget->setEnabled(false);
    m_Controls->m_ToFVisualisationSettingsWidget->setEnabled(false);
  }
}

void QmitkToFUtilView::Deactivated()
{
  m_MultiWidget->SetWidgetPlanesVisibility(true);
  m_MultiWidget->mitkWidget1->GetSliceNavigationController()->SetDefaultViewDirection(mitk::SliceNavigationController::Transversal);
  m_MultiWidget->mitkWidget1->GetSliceNavigationController()->SliceLockedOff();
  m_MultiWidget->mitkWidget2->GetSliceNavigationController()->SetDefaultViewDirection(mitk::SliceNavigationController::Sagittal);
  m_MultiWidget->mitkWidget2->GetSliceNavigationController()->SliceLockedOff();
  m_MultiWidget->mitkWidget3->GetSliceNavigationController()->SetDefaultViewDirection(mitk::SliceNavigationController::Frontal);
  m_MultiWidget->mitkWidget3->GetSliceNavigationController()->SliceLockedOff();

  mitk::RenderingManager::GetInstance()->InitializeViews();
  mitk::RenderingManager::GetInstance()->ForceImmediateUpdateAll();
  QmitkFunctionality::Deactivated();
}

void QmitkToFUtilView::OnToFCameraConnected()
{
  this->m_TransferFunctionInitialized = false;
  this->m_ImageSequence = 0;

  this->m_ToFImageGrabber = m_Controls->m_ToFConnectionWidget->GetToFImageGrabber();
  this->m_ToFCaptureWidth = this->m_ToFImageGrabber->GetCaptureWidth();
  this->m_ToFCaptureHeight = this->m_ToFImageGrabber->GetCaptureHeight();

  this->m_ToFImageRecorder->SetCameraDevice(this->m_ToFImageGrabber->GetCameraDevice());
  m_Controls->m_ToFRecorderWidget->SetParameter(this->m_ToFImageGrabber, this->m_ToFImageRecorder);
  m_Controls->m_ToFRecorderWidget->setEnabled(true);
  m_Controls->m_ToFRecorderWidget->ResetGUIToInitial();
  m_Controls->m_ToFVisualisationSettingsWidget->setEnabled(true);

  this->m_SurfaceDisplayCount = 0;
  this->m_2DDisplayCount = 0;

  //TODO
  this->m_RealTimeClock = mitk::RealTimeClock::New();
  this->m_StepsForFramerate = 100; 
  this->m_2DTimeBefore = this->m_RealTimeClock->GetCurrentStamp();

  try
  {
    this->m_VideoSource = mitk::OpenCVVideoSource::New();

    this->m_VideoSource->SetVideoCameraInput(0, false);
    this->m_VideoSource->StartCapturing();
    if(!this->m_VideoSource->IsCapturingEnabled())
    {
      MITK_INFO << "unable to initialize video grabbing/playback";
      this->m_VideoEnabled = false;
      m_Controls->m_VideoTextureCheckBox->setEnabled(false);
    }
    else
    {
      this->m_VideoEnabled = true;
      m_Controls->m_VideoTextureCheckBox->setEnabled(true);
    }

    if (this->m_VideoEnabled)
    {

      this->m_VideoSource->FetchFrame();
      this->m_VideoCaptureHeight = this->m_VideoSource->GetImageHeight();
      this->m_VideoCaptureWidth = this->m_VideoSource->GetImageWidth();
      int videoTexSize = this->m_VideoCaptureWidth * this->m_VideoCaptureHeight * 3; // for each pixel three values for rgb are needed!!
      this->m_VideoTexture = this->m_VideoSource->GetVideoTexture();

      unsigned int dimensions[2];
      dimensions[0] = this->m_VideoCaptureWidth;
      dimensions[1] = this->m_VideoCaptureHeight;       

      this->m_ToFDistanceImageToSurfaceFilter->SetTextureImageWidth(this->m_VideoCaptureWidth);
      this->m_ToFDistanceImageToSurfaceFilter->SetTextureImageHeight(this->m_VideoCaptureHeight);


      this->m_ToFSurfaceVtkMapper3D->SetTextureWidth(this->m_VideoCaptureWidth);
      this->m_ToFSurfaceVtkMapper3D->SetTextureHeight(this->m_VideoCaptureHeight);
    }
    m_MultiWidget->DisableGradientBackground();
  }
  catch (std::logic_error& e)
  {
    QMessageBox::warning(NULL, "Warning", QString(e.what()));
    MITK_ERROR << e.what();
    return;
  }

  mitk::RenderingManager::GetInstance()->ForceImmediateUpdateAll();

}

void QmitkToFUtilView::OnToFCameraDisconnected()
{
  m_Controls->m_ToFRecorderWidget->OnStop();
  m_Controls->m_ToFRecorderWidget->setEnabled(false);
  m_Controls->m_ToFVisualisationSettingsWidget->setEnabled(false);
  if(this->m_VideoSource)
  {
    this->m_VideoSource->StopCapturing();
    this->m_VideoSource = NULL;
  }
  mitk::RenderingManager::GetInstance()->ForceImmediateUpdateAll();
}

void QmitkToFUtilView::OnToFCameraStarted()
{
  if (m_ToFImageGrabber.IsNotNull())
  {
    // initial update of image grabber
    this->m_ToFImageGrabber->Update();

    this->m_ToFCompositeFilter->SetInput(0,this->m_ToFImageGrabber->GetOutput(0));
    this->m_ToFCompositeFilter->SetInput(1,this->m_ToFImageGrabber->GetOutput(1));
    this->m_ToFCompositeFilter->SetInput(2,this->m_ToFImageGrabber->GetOutput(2));
    // initial update of composite filter
    this->m_ToFCompositeFilter->Update();
    this->m_ToFVisualizationFilter->SetInput(0,this->m_ToFCompositeFilter->GetOutput(0));
    this->m_ToFVisualizationFilter->SetInput(1,this->m_ToFCompositeFilter->GetOutput(1));
    this->m_ToFVisualizationFilter->SetInput(2,this->m_ToFCompositeFilter->GetOutput(2));
    // initial update of visualization filter
    this->m_ToFVisualizationFilter->Update();
    //this->m_MitkDistanceImage = m_ToFVisualizationFilter->GetOutput(0);
    //this->m_MitkAmplitudeImage = m_ToFVisualizationFilter->GetOutput(1);
    //this->m_MitkIntensityImage = m_ToFVisualizationFilter->GetOutput(2);
    this->m_MitkDistanceImage = m_ToFCompositeFilter->GetOutput(0);
    this->m_DistanceImageNode = ReplaceNodeData("Distance image",m_MitkDistanceImage);
    this->m_DistanceImageNode->SetProperty( "visible" , mitk::BoolProperty::New( true ));
    this->m_DistanceImageNode->SetVisibility( false, mitk::BaseRenderer::GetInstance(GetActiveStdMultiWidget()->mitkWidget2->GetRenderWindow() ) );
    this->m_DistanceImageNode->SetVisibility( false, mitk::BaseRenderer::GetInstance(GetActiveStdMultiWidget()->mitkWidget3->GetRenderWindow() ) );
    this->m_DistanceImageNode->SetVisibility( false, mitk::BaseRenderer::GetInstance(GetActiveStdMultiWidget()->mitkWidget4->GetRenderWindow() ) );
//    m_DistanceImageNode->SetBoolProperty( "use color", false );
//    // add a default rainbow lookup table for color mapping
//    mitk::LookupTable::Pointer mitkLut = mitk::LookupTable::New();
//    vtkLookupTable* vtkLut = mitkLut->GetVtkLookupTable();
//    vtkLut->SetHueRange(0.6667, 0.0);
//    vtkLut->SetTableRange(0.0, 1.0);
//    vtkLut->Build();
//    mitk::LookupTableProperty::Pointer mitkLutProp = mitk::LookupTableProperty::New();
//    mitkLutProp->SetLookupTable(mitkLut);
//    m_DistanceImageNode->SetProperty( "LookupTable", mitkLutProp );

//    float a = 255.0;
//    mitk::TransferFunction::Pointer tf = mitk::TransferFunction::New();
//    vtkColorTransferFunction *ctf = tf->GetColorTransferFunction();
//    ctf->RemoveAllPoints();
//    //ctf->AddRGBPoint(-1000, 0.0, 0.0, 0.0);
//    ctf->AddRGBPoint(1, 203/a, 104/a, 102/a);
//    ctf->AddRGBPoint(0, 255/a, 0/a, 0/a);
//    ctf->ClampingOn();
//    ctf->Modified();
//    mitk::TransferFunctionProperty::Pointer tfp = mitk::TransferFunctionProperty::New();
//    tfp->SetValue(tf);
//    m_DistanceImageNode->SetProperty("TransferFunction", tfp);

//    vtkColorTransferFunction* ctf = m_Controls->m_ToFVisualisationSettingsWidget->GetColorTransferFunctionByImageType("Distance");
//    this->PrepareImageForBackground(colorTransferFunction,);


    this->m_MitkAmplitudeImage = m_ToFCompositeFilter->GetOutput(1);
    this->m_AmplitudeImageNode = ReplaceNodeData("Amplitude image",m_MitkAmplitudeImage);
    this->m_AmplitudeImageNode->SetProperty( "visible" , mitk::BoolProperty::New( true ));
    this->m_AmplitudeImageNode->SetVisibility( false, mitk::BaseRenderer::GetInstance(GetActiveStdMultiWidget()->mitkWidget1->GetRenderWindow() ) );
    this->m_AmplitudeImageNode->SetVisibility( false, mitk::BaseRenderer::GetInstance(GetActiveStdMultiWidget()->mitkWidget3->GetRenderWindow() ) );
    this->m_AmplitudeImageNode->SetVisibility( false, mitk::BaseRenderer::GetInstance(GetActiveStdMultiWidget()->mitkWidget4->GetRenderWindow() ) );

    this->m_MitkIntensityImage = m_ToFCompositeFilter->GetOutput(2);
    this->m_IntensityImageNode = ReplaceNodeData("Intensity image",m_MitkIntensityImage);
    this->m_IntensityImageNode->SetProperty( "visible" , mitk::BoolProperty::New( true ));
    this->m_IntensityImageNode->SetVisibility( false, mitk::BaseRenderer::GetInstance(GetActiveStdMultiWidget()->mitkWidget1->GetRenderWindow() ) );
    this->m_IntensityImageNode->SetVisibility( false, mitk::BaseRenderer::GetInstance(GetActiveStdMultiWidget()->mitkWidget2->GetRenderWindow() ) );
    this->m_IntensityImageNode->SetVisibility( false, mitk::BaseRenderer::GetInstance(GetActiveStdMultiWidget()->mitkWidget4->GetRenderWindow() ) );

    this->m_ToFDistanceImageToSurfaceFilter->SetInput(0,m_MitkDistanceImage);
    this->m_ToFDistanceImageToSurfaceFilter->SetInput(1,m_MitkAmplitudeImage);
    this->m_ToFDistanceImageToSurfaceFilter->SetInput(2,m_MitkIntensityImage);
    this->m_Surface = this->m_ToFDistanceImageToSurfaceFilter->GetOutput(0);
    this->m_SurfaceNode = ReplaceNodeData("Surface",m_Surface);

    mitk::RenderingManager::GetInstance()->InitializeViews(
      this->m_MitkDistanceImage->GetTimeSlicedGeometry(), mitk::RenderingManager::REQUEST_UPDATE_ALL, true);

    this->m_Frametimer->start(0);

    m_Controls->m_ToFCompositeFilterWidget->UpdateFilterParameter();

    if (m_Controls->m_TextureCheckBox->isChecked())
    {
      OnTextureCheckBoxChecked(true);
    }
    if (m_Controls->m_VideoTextureCheckBox->isChecked())
    {
      OnVideoTextureCheckBoxChecked(true);
    }
  }
  // initialize point set measurement
  m_Controls->tofMeasurementWidget->InitializeWidget(m_MultiWidget,this->GetDefaultDataStorage(),m_MitkDistanceImage);
}

void QmitkToFUtilView::OnToFCameraStopped()
{
  this->m_Frametimer->stop();
}

void QmitkToFUtilView::OnToFCameraSelected(const QString selected)
{
  if ((selected=="PMD CamBoard")||(selected=="PMD O3D"))
  {
    MITK_INFO<<"Surface representation currently not available for CamBoard and O3. Intrinsic parameters missing.";
    this->m_Controls->m_SurfaceCheckBox->setEnabled(false);
    this->m_Controls->m_TextureCheckBox->setEnabled(false);
    this->m_Controls->m_VideoTextureCheckBox->setEnabled(false);
    this->m_Controls->m_SurfaceCheckBox->setChecked(false);
    this->m_Controls->m_TextureCheckBox->setChecked(false);
    this->m_Controls->m_VideoTextureCheckBox->setChecked(false);
  }
  else
  {
    this->m_Controls->m_SurfaceCheckBox->setEnabled(true);
    this->m_Controls->m_TextureCheckBox->setEnabled(true); // TODO enable when bug 8106 is solved
    this->m_Controls->m_VideoTextureCheckBox->setEnabled(true);
  }
}

void QmitkToFUtilView::InitTexture(unsigned char* &image, int width, int height)
{
  int texSize = width * height * 3;
  image = new unsigned char[ texSize ];
  for(int i=0; i<texSize; i++)
  {
    image[i] = 0;
  }
}

void* QmitkToFUtilView::GetDataFromImage(std::string imageType)
{
  void* data;
  if (imageType.compare("Distance")==0)
  {
    data = this->m_MitkDistanceImage->GetData();
  }
  else if (imageType.compare("Amplitude")==0)
  {
    data = this->m_MitkAmplitudeImage->GetData();
  }
  if (imageType.compare("Intensity")==0)
  {
    data = this->m_MitkIntensityImage->GetData();
  }
  return data;
}

void QmitkToFUtilView::OnUpdateCamera()
{
  int currentImageSequence = 0;


  if (!this->m_TransferFunctionInitialized)
  {
    m_Controls->m_ToFVisualisationSettingsWidget->InitializeTransferFunction(this->m_MitkDistanceImage, this->m_MitkAmplitudeImage, this->m_MitkIntensityImage);
    this->m_TransferFunctionInitialized = true;
  }

  if (m_Controls->m_VideoTextureCheckBox->isChecked() && this->m_VideoEnabled && this->m_VideoSource)
  {
    this->m_VideoTexture = this->m_VideoSource->GetVideoTexture();
    ProcessVideoTransform();
  }

  vtkColorTransferFunction* colorTransferFunction;
//  std::string imageType;

  colorTransferFunction = m_Controls->m_ToFVisualisationSettingsWidget->GetWidget1ColorTransferFunction();
  mitk::TransferFunction::Pointer tf = mitk::TransferFunction::New();
  tf->SetColorTransferFunction( colorTransferFunction );
//  mitk::TransferFunctionProperty::Pointer

      m_DistanceImageNode->SetProperty("imageRendering.tranferFunction",mitk::TransferFunctionProperty::New(tf));
//  imageType = m_Controls->m_ToFVisualisationSettingsWidget->GetWidget1ImageType();
//  RenderWidget(m_MultiWidget->mitkWidget1, this->m_QmitkToFImageBackground1, this->m_Widget1ImageType, imageType,
//    colorTransferFunction, this->m_VideoTexture, this->m_Widget1Texture );

//  colorTransferFunction = m_Controls->m_ToFVisualisationSettingsWidget->GetWidget2ColorTransferFunction();
//  imageType = m_Controls->m_ToFVisualisationSettingsWidget->GetWidget2ImageType();
//  RenderWidget(m_MultiWidget->mitkWidget2, this->m_QmitkToFImageBackground2, this->m_Widget2ImageType, imageType,
//    colorTransferFunction, this->m_VideoTexture, this->m_Widget2Texture );

//  colorTransferFunction = m_Controls->m_ToFVisualisationSettingsWidget->GetWidget3ColorTransferFunction();
//  imageType = m_Controls->m_ToFVisualisationSettingsWidget->GetWidget3ImageType();
//  RenderWidget(m_MultiWidget->mitkWidget3, this->m_QmitkToFImageBackground3, this->m_Widget3ImageType, imageType,
//    colorTransferFunction, this->m_VideoTexture, this->m_Widget3Texture );

  if (m_Controls->m_SurfaceCheckBox->isChecked())
  {
    // update surface
    this->m_Surface->Update();

    colorTransferFunction = m_Controls->m_ToFVisualisationSettingsWidget->GetColorTransferFunctionByImageType("Distance");
 
    this->m_ToFSurfaceVtkMapper3D->SetVtkScalarsToColors(colorTransferFunction);

    if (this->m_SurfaceDisplayCount < 2)
    {
      this->m_SurfaceNode->SetData(this->m_Surface);
      this->m_SurfaceNode->SetMapper(mitk::BaseRenderer::Standard3D, m_ToFSurfaceVtkMapper3D);

      mitk::RenderingManager::GetInstance()->InitializeViews(
        this->m_Surface->GetTimeSlicedGeometry(), mitk::RenderingManager::REQUEST_UPDATE_3DWINDOWS, true);

      mitk::Point3D surfaceCenter= this->m_Surface->GetGeometry()->GetCenter();
      m_MultiWidget->mitkWidget4->GetRenderer()->GetVtkRenderer()->GetActiveCamera()->SetPosition(0,0,-50);
      m_MultiWidget->mitkWidget4->GetRenderer()->GetVtkRenderer()->GetActiveCamera()->SetViewUp(0,-1,0);
      m_MultiWidget->mitkWidget4->GetRenderer()->GetVtkRenderer()->GetActiveCamera()->SetFocalPoint(0,0,surfaceCenter[2]);
      m_MultiWidget->mitkWidget4->GetRenderer()->GetVtkRenderer()->GetActiveCamera()->SetViewAngle(40);
      m_MultiWidget->mitkWidget4->GetRenderer()->GetVtkRenderer()->GetActiveCamera()->SetClippingRange(1, 10000);
    }
    this->m_SurfaceDisplayCount++;

  }
  else
  {
    // update pipeline
    this->m_MitkDistanceImage->Update();
  //  // update distance image node
  //  this->m_DistanceImageNode->SetData(this->m_MitkDistanceImage);
  }

  mitk::RenderingManager::GetInstance()->ForceImmediateUpdateAll();

  this->m_2DDisplayCount++;
  if ((this->m_2DDisplayCount % this->m_StepsForFramerate) == 0)
  {
    this->m_2DTimeAfter = this->m_RealTimeClock->GetCurrentStamp() - this->m_2DTimeBefore;
    MITK_INFO << " 2D-Display-framerate (fps): " << this->m_StepsForFramerate / (this->m_2DTimeAfter/1000) << " Sequence: " << this->m_ImageSequence;
    this->m_2DTimeBefore = this->m_RealTimeClock->GetCurrentStamp();
  }
}

void QmitkToFUtilView::ProcessVideoTransform()
{
  IplImage *src, *dst;
  src = cvCreateImageHeader(cvSize(this->m_VideoCaptureWidth, this->m_VideoCaptureHeight), IPL_DEPTH_8U, 3);
  src->imageData = (char*)this->m_VideoTexture;

  CvPoint2D32f srcTri[3], dstTri[3];
	CvMat* rot_mat = cvCreateMat(2,3,CV_32FC1);
	CvMat* warp_mat = cvCreateMat(2,3,CV_32FC1);
  dst = cvCloneImage(src);
	dst->origin = src->origin;
	cvZero( dst );

	// Create angle and scale
	double angle = 0.0;
	double scale = 1.0;

  int xOffset = 0;//m_Controls->m_XOffsetSpinBox->value();
  int yOffset = 0;//m_Controls->m_YOffsetSpinBox->value();
  int zoom = 0;//m_Controls->m_ZoomSpinBox->value();

	// Compute warp matrix
	srcTri[0].x = 0 + zoom;
	srcTri[0].y = 0 + zoom;
	srcTri[1].x = src->width - 1 - zoom;
	srcTri[1].y = 0 + zoom;
	srcTri[2].x = 0 + zoom;
	srcTri[2].y = src->height - 1 - zoom;

	dstTri[0].x = 0;
	dstTri[0].y = 0;
	dstTri[1].x = src->width - 1;
	dstTri[1].y = 0;
	dstTri[2].x = 0;
	dstTri[2].y = src->height - 1;

	cvGetAffineTransform( srcTri, dstTri, warp_mat );
	cvWarpAffine( src, dst, warp_mat );
	cvCopy ( dst, src );


	// Compute warp matrix
	srcTri[0].x = 0;
	srcTri[0].y = 0;
	srcTri[1].x = src->width - 1;
	srcTri[1].y = 0;
	srcTri[2].x = 0;
	srcTri[2].y = src->height - 1;

	dstTri[0].x = srcTri[0].x + xOffset;
	dstTri[0].y = srcTri[0].y + yOffset;
	dstTri[1].x = srcTri[1].x + xOffset;
	dstTri[1].y = srcTri[1].y + yOffset;
	dstTri[2].x = srcTri[2].x + xOffset;
	dstTri[2].y = srcTri[2].y + yOffset;

	cvGetAffineTransform( srcTri, dstTri, warp_mat );
	cvWarpAffine( src, dst, warp_mat );
	cvCopy ( dst, src );

  src->imageData = NULL;
	cvReleaseImage( &src );
	cvReleaseImage( &dst );
	cvReleaseMat( &rot_mat );
	cvReleaseMat( &warp_mat );

}

void QmitkToFUtilView::RenderWidget(QmitkRenderWindow* mitkWidget, QmitkToFImageBackground* imageBackground, 
                                    std::string& oldImageType, std::string newImageType,
                                    vtkColorTransferFunction* colorTransferFunction, unsigned char* videoTexture, unsigned char* tofTexture )
{
  if (newImageType.compare("Video") == 0)
  {
    if (this->m_VideoEnabled)
    {
      if (oldImageType.compare(newImageType)!=0)
      {
        imageBackground->AddRenderWindow(mitkWidget->GetRenderWindow(), this->m_VideoCaptureWidth, this->m_VideoCaptureHeight);
        oldImageType = newImageType;
      }
      imageBackground->UpdateBackground(videoTexture);
    }
  }
  else
  {
    if (oldImageType.compare(newImageType)!=0)
    {
      imageBackground->AddRenderWindow(mitkWidget->GetRenderWindow(), this->m_ToFCaptureWidth, this->m_ToFCaptureHeight);
      oldImageType = newImageType;
    }
    void* data = GetDataFromImage(newImageType);
    PrepareImageForBackground(colorTransferFunction, (float*)data, tofTexture);
    imageBackground->UpdateBackground(tofTexture);
  }
}

void QmitkToFUtilView::OnTextureCheckBoxChecked(bool checked)
{
  if (checked)
  {
    this->m_SurfaceNode->SetBoolProperty("scalar visibility", true);
  }
  else
  {
    this->m_SurfaceNode->SetBoolProperty("scalar visibility", false);
  }
}

void QmitkToFUtilView::OnVideoTextureCheckBoxChecked(bool checked)
{
  if (checked)
  {
    if (this->m_VideoEnabled)
    {
      this->m_ToFSurfaceVtkMapper3D->SetTexture(this->m_VideoTexture);
    }
    else
    {
      this->m_ToFSurfaceVtkMapper3D->SetTexture(NULL);
    }
  }
  else
  {
    this->m_ToFSurfaceVtkMapper3D->SetTexture(NULL);
  }
}

void QmitkToFUtilView::PrepareImageForBackground(vtkLookupTable* lut, float* floatData, unsigned char* image)
{
  vtkSmartPointer<vtkFloatArray> floatArrayDist = vtkFloatArray::New();
  floatArrayDist->Initialize();
  int numOfPixel = this->m_ToFCaptureWidth * this->m_ToFCaptureHeight;
  float* flippedFloatData = new float[numOfPixel];

  for (int i=0; i<this->m_ToFCaptureHeight; i++)
  {
    for (int j=0; j<this->m_ToFCaptureWidth; j++)
    {
      flippedFloatData[i*this->m_ToFCaptureWidth+j] = floatData[((this->m_ToFCaptureHeight-1-i)*this->m_ToFCaptureWidth)+j];
    }
  }

  floatArrayDist->SetArray(flippedFloatData, numOfPixel, 0);
  lut->MapScalarsThroughTable(floatArrayDist, image, VTK_RGB);

  delete[] flippedFloatData;
}

void QmitkToFUtilView::PrepareImageForBackground(vtkColorTransferFunction* colorTransferFunction, float* floatData, unsigned char* image)
{
  vtkSmartPointer<vtkFloatArray> floatArrayDist = vtkFloatArray::New();
  floatArrayDist->Initialize();
  int numOfPixel = this->m_ToFCaptureWidth * this->m_ToFCaptureHeight;
  float* flippedFloatData = new float[numOfPixel];

  for (int i=0; i<this->m_ToFCaptureHeight; i++)
  {
    for (int j=0; j<this->m_ToFCaptureWidth; j++)
    {
      flippedFloatData[i*this->m_ToFCaptureWidth+j] = floatData[((this->m_ToFCaptureHeight-1-i)*this->m_ToFCaptureWidth)+j];
    }
  }

  floatArrayDist->SetArray(flippedFloatData, numOfPixel, 0);
  colorTransferFunction->MapScalarsThroughTable(floatArrayDist, image, VTK_RGB);

  delete[] flippedFloatData;
}

mitk::DataNode::Pointer QmitkToFUtilView::ReplaceNodeData( std::string nodeName, mitk::BaseData* data )
{

  mitk::DataNode::Pointer node = this->GetDefaultDataStorage()->GetNamedNode(nodeName);
  if (node.IsNull())
  {
    node = mitk::DataNode::New();
    node->SetData(data);
    node->SetName(nodeName);
    this->GetDefaultDataStorage()->Add(node);
  }
  else
  {
    node->SetData(data);
  }
  return node;
}
