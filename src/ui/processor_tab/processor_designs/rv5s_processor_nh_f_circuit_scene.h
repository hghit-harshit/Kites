#pragma once
#include "../circuit_scene.h"
namespace Kites
{
class RV5StageVM_NH_F_CircuitScene : public CircuitScene
{
    Q_OBJECT
  public:
    explicit RV5StageVM_NH_F_CircuitScene(QObject *parent = nullptr) : CircuitScene(parent)
    {
        qDebug() << "Loading RVSS Circuit Scene";
        loadScene(":/circuit_designs/NH_F_Processor.json");
    }
};
} // namespace Kites
