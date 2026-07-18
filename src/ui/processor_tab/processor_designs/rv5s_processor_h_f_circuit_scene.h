#pragma once
#include "../circuit_scene.h"
namespace Kites
{
class RV5StageVM_H_F_CircuitScene : public CircuitScene
{
    Q_OBJECT
  public:
    explicit RV5StageVM_H_F_CircuitScene(QObject *parent = nullptr) : CircuitScene(parent)
    {
        loadScene(":/circuit_designs/H_F_Processor.json");
    }
};
} // namespace Kites
