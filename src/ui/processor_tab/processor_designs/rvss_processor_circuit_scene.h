#pragma once
#include "../circuit_scene.h"

namespace Kites
{
class RVSSCircuitScene : public CircuitScene
{
    Q_OBJECT
  public:
    explicit RVSSCircuitScene(QObject *parent = nullptr) : CircuitScene(parent)
    {
        loadScene(":/circuit_designs/single_cycle_draft1.json");
    }
};
} // namespace Kites
