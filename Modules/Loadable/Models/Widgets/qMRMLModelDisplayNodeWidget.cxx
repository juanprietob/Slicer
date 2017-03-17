/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// QT includes
#include <QColor>

// qMRML includes
#include "qMRMLModelDisplayNodeWidget.h"
#include "ui_qMRMLModelDisplayNodeWidget.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLSelectionNode.h>

// VTK includes
#include <vtkDataArray.h>
#include <vtkPointData.h>
#include <vtkPointSet.h>
#include <vtkSmartPointer.h>

//------------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Models
class qMRMLModelDisplayNodeWidgetPrivate: public QWidget, public Ui_qMRMLModelDisplayNodeWidget
{
  Q_DECLARE_PUBLIC(qMRMLModelDisplayNodeWidget);

protected:
  qMRMLModelDisplayNodeWidget* const q_ptr;
  typedef QWidget Superclass;

public:
  qMRMLModelDisplayNodeWidgetPrivate(qMRMLModelDisplayNodeWidget& object);
  void init();

  vtkSmartPointer<vtkMRMLModelDisplayNode> MRMLModelDisplayNode;
  vtkSmartPointer<vtkMRMLNode>  ModelOrHierarchyNode;
};

//------------------------------------------------------------------------------
qMRMLModelDisplayNodeWidgetPrivate::qMRMLModelDisplayNodeWidgetPrivate(qMRMLModelDisplayNodeWidget& object)
  : q_ptr(&object)
{
}

//------------------------------------------------------------------------------
void qMRMLModelDisplayNodeWidgetPrivate::init()
{
  Q_Q(qMRMLModelDisplayNodeWidget);
  this->setupUi(q);

  // Without this infinite loop of widget update/VTK data set update occurs when updating a VTK data set
  // that is generated by an algorithm that  temorarily removes all arrays from its output temporarily
  // (for example the vtkGlyph3D filter behaves like this).
  // The root cause of the problem is that if none option is not enabled then the combobox
  // automatically selects the first array, which triggers a data set change, which removes all arrays,
  // which triggers a widget update, etc. - until stack overflow occurs.
  //this->ActiveScalarComboBox->setNoneEnabled(true);


  QObject::connect(this->ScalarsVisibilityCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(setScalarsVisibility(bool)));
  QObject::connect(this->ActiveScalarComboBox, SIGNAL(currentArrayChanged(QString)),
                   q, SLOT(setActiveScalarName(QString)));
  QObject::connect(this->ScalarsColorNodeComboBox,
                   SIGNAL(currentNodeChanged(vtkMRMLNode*)),
                   q, SLOT(setScalarsColorNode(vtkMRMLNode*)));

  // scalar range
  QObject::connect(this->DisplayedScalarRangeModeComboBox, SIGNAL(currentIndexChanged(int)),
                   q, SLOT(setScalarRangeMode(int)));
  QObject::connect(this->DisplayedScalarRangeWidget, SIGNAL(valuesChanged(double,double)),
                   q, SLOT(setScalarsDisplayRange(double,double)));

  // Thresholding
  this->ThresholdCheckBox->setChecked(false);
  this->ThresholdRangeWidget->setEnabled(false);
  QObject::connect(this->ThresholdCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(setTresholdEnabled(bool)));
  QObject::connect(this->ThresholdRangeWidget, SIGNAL(valuesChanged(double,double)),
                   q, SLOT(setThresholdRange(double,double)));

  // update range mode
  q->setScalarRangeMode(qMRMLModelDisplayNodeWidget::Data); // former auto

  if (this->MRMLModelDisplayNode.GetPointer())
  {
    q->setEnabled(true);
    q->setMRMLModelDisplayNode(this->MRMLModelDisplayNode);
  }

  this->MRMLDisplayNodeWidget->setSelectedVisible(false);
}

//------------------------------------------------------------------------------
qMRMLModelDisplayNodeWidget::qMRMLModelDisplayNodeWidget(QWidget *_parent)
  : QWidget(_parent)
  , d_ptr(new qMRMLModelDisplayNodeWidgetPrivate(*this))
{
  Q_D(qMRMLModelDisplayNodeWidget);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLModelDisplayNodeWidget::~qMRMLModelDisplayNodeWidget()
{
}

//------------------------------------------------------------------------------
vtkMRMLModelDisplayNode* qMRMLModelDisplayNodeWidget::mrmlModelDisplayNode()const
{
  Q_D(const qMRMLModelDisplayNodeWidget);
  return d->MRMLModelDisplayNode;
}

//------------------------------------------------------------------------------
vtkMRMLNode* qMRMLModelDisplayNodeWidget::mrmlDisplayableNode()const
{
  Q_D(const qMRMLModelDisplayNodeWidget);
  return d->ModelOrHierarchyNode;
}

//------------------------------------------------------------------------------
void qMRMLModelDisplayNodeWidget::setMRMLModelOrHierarchyNode(vtkMRMLNode* node)
{
  Q_D(qMRMLModelDisplayNodeWidget);

  d->ModelOrHierarchyNode = node;

  // can be set from a model node or a model hierarchy node
  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(node);
  vtkMRMLModelHierarchyNode *hierarchyNode = vtkMRMLModelHierarchyNode::SafeDownCast(node);
  vtkMRMLModelDisplayNode *modelDisplayNode = 0;
  if (modelNode)
    {
    vtkMRMLSelectionNode* selectionNode = this->getSelectionNode(modelNode->GetScene());
    std::string displayNodeName;
    if (selectionNode)
      {
      displayNodeName = selectionNode->GetModelHierarchyDisplayNodeClassName(
                        modelNode->GetClassName());
      }

    int nDisplayNodes = modelNode->GetNumberOfDisplayNodes();
    for (int i=0; i<nDisplayNodes; i++)
      {
        modelDisplayNode = vtkMRMLModelDisplayNode::SafeDownCast(modelNode->GetNthDisplayNode(i));
        //qvtkReconnect(modelDisplayNode, modelDisplayNode, vtkCommand::ModifiedEvent,
        //        this, SLOT(updateWidgetFromMRML()));
        if (displayNodeName.empty() || modelDisplayNode->IsA(displayNodeName.c_str()))
          {
          break;
          }
      }
    }
  else if (hierarchyNode)
    {
    modelDisplayNode = hierarchyNode->GetModelDisplayNode();
    }
  this->setMRMLModelDisplayNode(modelDisplayNode);
}

//------------------------------------------------------------------------------
void qMRMLModelDisplayNodeWidget::setMRMLModelDisplayNode(vtkMRMLNode* node)
{
  this->setMRMLModelDisplayNode(vtkMRMLModelDisplayNode::SafeDownCast(node));
}

//------------------------------------------------------------------------------
void qMRMLModelDisplayNodeWidget::setMRMLModelDisplayNode(vtkMRMLModelDisplayNode* ModelDisplayNode)
{
  Q_D(qMRMLModelDisplayNodeWidget);
  qvtkReconnect(d->MRMLModelDisplayNode, ModelDisplayNode, vtkCommand::ModifiedEvent,
                this, SLOT(updateWidgetFromMRML()));
  d->MRMLModelDisplayNode = ModelDisplayNode;
  d->MRMLDisplayNodeWidget->setMRMLDisplayNode(ModelDisplayNode);
  this->updateWidgetFromMRML();
}

//------------------------------------------------------------------------------
void qMRMLModelDisplayNodeWidget::setScalarsVisibility(bool visible)
{
  Q_D(qMRMLModelDisplayNodeWidget);
  if (!d->MRMLModelDisplayNode.GetPointer())
    {
    return;
    }

  d->MRMLModelDisplayNode->SetScalarVisibility(visible);
}

//------------------------------------------------------------------------------
bool qMRMLModelDisplayNodeWidget::scalarsVisibility()const
{
  Q_D(const qMRMLModelDisplayNodeWidget);
  return d->ScalarsVisibilityCheckBox->isChecked();
}

//------------------------------------------------------------------------------
void qMRMLModelDisplayNodeWidget::setActiveScalarName(const QString& arrayName)
{
  Q_D(qMRMLModelDisplayNodeWidget);
  if (!d->MRMLModelDisplayNode.GetPointer())
    {
    return;
    }

  d->MRMLModelDisplayNode->SetActiveScalarName(arrayName.toLatin1());

  // if there's no color node set for a non empty array name, use a default
  if (!arrayName.isEmpty() &&
      d->MRMLModelDisplayNode->GetColorNodeID() == NULL)
    {
    const char *colorNodeID = "vtkMRMLColorTableNodeRainbow";

    d->MRMLModelDisplayNode->SetAndObserveColorNodeID(colorNodeID);
    }

}

//------------------------------------------------------------------------------
QString qMRMLModelDisplayNodeWidget::activeScalarName()const
{
  Q_D(const qMRMLModelDisplayNodeWidget);
  // TODO: use currentArrayName()
  vtkAbstractArray* array = d->ActiveScalarComboBox->currentArray();
  return array ? array->GetName() : "";
}

//------------------------------------------------------------------------------
void qMRMLModelDisplayNodeWidget::setScalarsColorNode(vtkMRMLNode* colorNode)
{
  this->setScalarsColorNode(vtkMRMLColorNode::SafeDownCast(colorNode));
}

//------------------------------------------------------------------------------
void qMRMLModelDisplayNodeWidget::setScalarsColorNode(vtkMRMLColorNode* colorNode)
{
  Q_D(qMRMLModelDisplayNodeWidget);
  if (!d->MRMLModelDisplayNode.GetPointer())
    {
    return;
    }

  d->MRMLModelDisplayNode->SetAndObserveColorNodeID(colorNode ? colorNode->GetID() : NULL);
}

//------------------------------------------------------------------------------
vtkMRMLColorNode* qMRMLModelDisplayNodeWidget::scalarsColorNode()const
{
  Q_D(const qMRMLModelDisplayNodeWidget);
  return vtkMRMLColorNode::SafeDownCast(
    d->ScalarsColorNodeComboBox->currentNode());
}

// --------------------------------------------------------------------------
void qMRMLModelDisplayNodeWidget::setScalarRangeMode(ControlMode scalarRangeMode)
{
  Q_D(qMRMLModelDisplayNodeWidget);

  if (!d->MRMLModelDisplayNode.GetPointer())
    {
    return;
    }

  int flag = d->MRMLModelDisplayNode->GetScalarRangeFlag();
  switch (scalarRangeMode)
    {
    case qMRMLModelDisplayNodeWidget::Data :
      if(flag == vtkMRMLDisplayNode::UseDataScalarRange)
        {
        return;
        }
      d->DisplayedScalarRangeWidget->setEnabled(false);
      d->MRMLModelDisplayNode->SetScalarRangeFlag(vtkMRMLDisplayNode::UseDataScalarRange);
      break;
    case qMRMLModelDisplayNodeWidget::LUT :
      if(flag == vtkMRMLDisplayNode::UseColorNodeScalarRange)
        {
        return;
        }
      d->DisplayedScalarRangeWidget->setEnabled(false);
      d->MRMLModelDisplayNode->SetScalarRangeFlag(vtkMRMLDisplayNode::UseColorNodeScalarRange);
      break;
    case qMRMLModelDisplayNodeWidget::DataType :
      if(flag == vtkMRMLDisplayNode::UseDataTypeScalarRange)
        {
        return;
        }
      d->DisplayedScalarRangeWidget->setEnabled(false);
      d->MRMLModelDisplayNode->SetScalarRangeFlag(vtkMRMLDisplayNode::UseDataTypeScalarRange);
      break;
    case qMRMLModelDisplayNodeWidget::Manual :
      if(flag == vtkMRMLDisplayNode::UseManualScalarRange)
        {
        return;
        }
      d->DisplayedScalarRangeWidget->setEnabled(true);
      d->MRMLModelDisplayNode->SetScalarRangeFlag(vtkMRMLDisplayNode::UseManualScalarRange);
      break;
    }

  emit this->scalarRangeModeValueChanged(scalarRangeMode);
}

// --------------------------------------------------------------------------
void qMRMLModelDisplayNodeWidget::setScalarRangeMode(int scalarRangeMode)
{
  switch(scalarRangeMode)
    {
    case 0:
      this->setScalarRangeMode(qMRMLModelDisplayNodeWidget::Data);
      break;
    case 1:
      this->setScalarRangeMode(qMRMLModelDisplayNodeWidget::LUT);
      break;
    case 2:
      this->setScalarRangeMode(qMRMLModelDisplayNodeWidget::DataType);
      break;
    case 3:
      this->setScalarRangeMode(qMRMLModelDisplayNodeWidget::Manual);
      break;
    default:
      break;
    }
}

// --------------------------------------------------------------------------
qMRMLModelDisplayNodeWidget::ControlMode qMRMLModelDisplayNodeWidget::scalarRangeMode() const
{
  Q_D(const qMRMLModelDisplayNodeWidget);
  switch( d->DisplayedScalarRangeModeComboBox->currentIndex() )
    {
    case 0:
      return qMRMLModelDisplayNodeWidget::Data;
      break;
    case 1:
      return qMRMLModelDisplayNodeWidget::LUT;
      break;
    case 2:
      return qMRMLModelDisplayNodeWidget::DataType;
      break;
    case 3:
      return qMRMLModelDisplayNodeWidget::Manual;
      break;
    default:
      break;
    }
  return qMRMLModelDisplayNodeWidget::Data;
}

//------------------------------------------------------------------------------
void qMRMLModelDisplayNodeWidget::setScalarsDisplayRange(double min, double max)
{
  Q_D(qMRMLModelDisplayNodeWidget);
  if (!d->MRMLModelDisplayNode.GetPointer())
    {
    return;
    }
  double *range = d->MRMLModelDisplayNode->GetScalarRange();
  if (range[0] != min || range[1] != max)
    {
    qvtkBlock(d->MRMLModelDisplayNode, vtkCommand::ModifiedEvent, this);
    d->MRMLModelDisplayNode->SetScalarRange(min, max);
    qvtkUnblock(d->MRMLModelDisplayNode, vtkCommand::ModifiedEvent, this);
    d->ThresholdRangeWidget->setRange(min,max);
    d->ThresholdRangeWidget->setValues(min,max);
    }
}

//------------------------------------------------------------------------------
void qMRMLModelDisplayNodeWidget::setTresholdEnabled(bool b)
{
  Q_D(qMRMLModelDisplayNodeWidget);
  if (!d->MRMLModelDisplayNode.GetPointer())
    {
    return;
    }

  qvtkBlock(d->MRMLModelDisplayNode, vtkCommand::ModifiedEvent, this);
  d->MRMLModelDisplayNode->SetThresholdEnabled(b);
  qvtkUnblock(d->MRMLModelDisplayNode, vtkCommand::ModifiedEvent, this);

  d->ThresholdRangeWidget->setEnabled(b);
}

//------------------------------------------------------------------------------
void qMRMLModelDisplayNodeWidget::setThresholdRange(double min, double max)
{
  Q_D(qMRMLModelDisplayNodeWidget);
  if (!d->MRMLModelDisplayNode.GetPointer())
    {
    return;
    }
  double oldMin = d->MRMLModelDisplayNode->GetThresholdMin();
  double oldMax = d->MRMLModelDisplayNode->GetThresholdMax();
  if (oldMin != min || oldMax != max)
    {
    qvtkBlock(d->MRMLModelDisplayNode, vtkCommand::ModifiedEvent, this);
    d->MRMLModelDisplayNode->SetThresholdRange(min, max);
    qvtkUnblock(d->MRMLModelDisplayNode, vtkCommand::ModifiedEvent, this);
    }
}

// --------------------------------------------------------------------------
double qMRMLModelDisplayNodeWidget::minimumValue() const
{
  Q_D(const qMRMLModelDisplayNodeWidget);

  return d->DisplayedScalarRangeWidget->minimumValue();
}

// --------------------------------------------------------------------------
double qMRMLModelDisplayNodeWidget::maximumValue() const
{
  Q_D(const qMRMLModelDisplayNodeWidget);

  return d->DisplayedScalarRangeWidget->maximumValue();
}

// --------------------------------------------------------------------------
void qMRMLModelDisplayNodeWidget::setMinimumValue(double min)
{
  Q_D(const qMRMLModelDisplayNodeWidget);

  d->DisplayedScalarRangeWidget->setMinimumValue(min);
}

// --------------------------------------------------------------------------
void qMRMLModelDisplayNodeWidget::setMaximumValue(double max)
{
  Q_D(const qMRMLModelDisplayNodeWidget);

  d->DisplayedScalarRangeWidget->setMaximumValue(max);
}

//------------------------------------------------------------------------------
void qMRMLModelDisplayNodeWidget::updateWidgetFromMRML()
{
  bool wasBlocking;

  Q_D(qMRMLModelDisplayNodeWidget);
  this->setEnabled(d->MRMLModelDisplayNode.GetPointer() != 0);
  if (!d->MRMLModelDisplayNode.GetPointer())
    {
    return;
    }
  if (d->ScalarsVisibilityCheckBox->isChecked() !=
      (bool)d->MRMLModelDisplayNode->GetScalarVisibility())
    {
    wasBlocking = d->ScalarsVisibilityCheckBox->blockSignals(true);
    d->ScalarsVisibilityCheckBox->setChecked(
      d->MRMLModelDisplayNode->GetScalarVisibility());
    d->ScalarsVisibilityCheckBox->blockSignals(wasBlocking);
    }

  // Update scalar values, range, decimals and single step
  double *displayRange =  d->MRMLModelDisplayNode->GetScalarRange();
  double precision = (displayRange[1] - displayRange[0])/100.0;
  double newMin = displayRange[0];
  double newMax = displayRange[1];
  int decimals = 0;
  if (precision != 0.0)
    {
    newMin = (floor(displayRange[0]/precision) - 20 ) * precision;
    newMax = (ceil(displayRange[1]/precision) + 20 ) * precision;
    while (precision <= 1.0)
      {
      precision *= 10.0;
      decimals++;
      }
    precision = pow(10.0, -decimals);
    }
  else
    {
    precision = 1;
    }
  wasBlocking = d->DisplayedScalarRangeWidget->blockSignals(true);
  d->DisplayedScalarRangeWidget->setRange(newMin, newMax);
  d->DisplayedScalarRangeWidget->setValues(displayRange[0], displayRange[1]);
  d->DisplayedScalarRangeWidget->setDecimals(decimals);
  d->DisplayedScalarRangeWidget->setSingleStep(precision);
  d->DisplayedScalarRangeWidget->blockSignals(wasBlocking);

  //wasBlocking = d->ThresholdRangeWidget->blockSignals(true);
  d->ThresholdRangeWidget->setRange(displayRange[0] - precision, displayRange[1] + precision);
  d->ThresholdRangeWidget->setValues(displayRange[0] - precision, displayRange[1] + precision);
  d->ThresholdRangeWidget->setDecimals(decimals);
  d->ThresholdRangeWidget->setSingleStep(precision);
  //d->ThresholdRangeWidget->blockSignals(wasBlocking);

  wasBlocking = d->ThresholdCheckBox->blockSignals(true);
  d->ThresholdCheckBox->setChecked(d->MRMLModelDisplayNode->GetThresholdEnabled());
  d->ThresholdCheckBox->blockSignals(wasBlocking);

  wasBlocking = d->DisplayedScalarRangeModeComboBox->blockSignals(true);
  switch (d->MRMLModelDisplayNode->GetScalarRangeFlag())
    {
    case vtkMRMLDisplayNode::UseDataScalarRange :
      d->DisplayedScalarRangeWidget->setEnabled(false);
      d->DisplayedScalarRangeModeComboBox->setCurrentIndex(qMRMLModelDisplayNodeWidget::Data);
      break;
    case vtkMRMLDisplayNode::UseColorNodeScalarRange :
      d->DisplayedScalarRangeWidget->setEnabled(false);
      d->DisplayedScalarRangeModeComboBox->setCurrentIndex(qMRMLModelDisplayNodeWidget::LUT);
      break;
    case vtkMRMLDisplayNode::UseDataTypeScalarRange :
      d->DisplayedScalarRangeWidget->setEnabled(false);
      d->DisplayedScalarRangeModeComboBox->setCurrentIndex(qMRMLModelDisplayNodeWidget::DataType);
      break;
    case vtkMRMLDisplayNode::UseManualScalarRange :
      d->DisplayedScalarRangeWidget->setEnabled(true);
      d->DisplayedScalarRangeModeComboBox->setCurrentIndex(qMRMLModelDisplayNodeWidget::Manual);
      break;
    }
  d->DisplayedScalarRangeModeComboBox->blockSignals(wasBlocking);

  wasBlocking = d->ActiveScalarComboBox->blockSignals(true);
  d->ActiveScalarComboBox->setDataSet(d->MRMLModelDisplayNode->GetInputMesh());
  d->ActiveScalarComboBox->blockSignals(wasBlocking);

  if (d->ActiveScalarComboBox->currentArrayName() !=
      d->MRMLModelDisplayNode->GetActiveScalarName())
    {
    d->ActiveScalarComboBox->setCurrentArray(
      d->MRMLModelDisplayNode->GetActiveScalarName());
    }
  d->ActiveScalarComboBox->blockSignals(wasBlocking);

  wasBlocking = d->ScalarsColorNodeComboBox->blockSignals(true);
  if (d->ScalarsColorNodeComboBox->mrmlScene() !=
      d->MRMLModelDisplayNode->GetScene())
    {
    d->ScalarsColorNodeComboBox->setMRMLScene(
      d->MRMLModelDisplayNode->GetScene());
    }
  if (d->ScalarsColorNodeComboBox->currentNodeID() !=
      d->MRMLModelDisplayNode->GetColorNodeID())
    {
    d->ScalarsColorNodeComboBox->setCurrentNodeID(
      d->MRMLModelDisplayNode->GetColorNodeID());
    }
  d->ScalarsColorNodeComboBox->blockSignals(wasBlocking);
}

//------------------------------------------------------------------------------
vtkMRMLSelectionNode* qMRMLModelDisplayNodeWidget::getSelectionNode(vtkMRMLScene *mrmlScene)
{
  vtkMRMLSelectionNode* selectionNode = 0;
  if (mrmlScene)
    {
    selectionNode =
      vtkMRMLSelectionNode::SafeDownCast(mrmlScene->GetNodeByID("vtkMRMLSelectionNodeSingleton"));
    }
  return selectionNode;
}
