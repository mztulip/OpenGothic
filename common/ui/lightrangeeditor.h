#pragma once

#include <Tempest/Widget>
#include <Tempest/Rect>
#include <functional>
#include <vector>

#include "graphics/lightgroup.h"

class MainWindow;

class LightRangeEditor : public Tempest::Widget {
  public:
    LightRangeEditor(MainWindow& owner);

    void toggle();

  private:
    struct ToggleDef {
      const char*               label;
      std::function<bool()>     get;
      std::function<void(bool)> set;
    };

    struct SliderDef {
        const char*                 label;
        std::function<float()>      get;
        std::function<void(float)>  set;
        float                       minV;
        float                       maxV;
    };

    void paintEvent     (Tempest::PaintEvent& e) override;
    void mouseDownEvent (Tempest::MouseEvent& e) override;
    void mouseUpEvent   (Tempest::MouseEvent& e) override;
    void mouseMoveEvent (Tempest::MouseEvent& e) override;
    void mouseWheelEvent(Tempest::MouseEvent& e) override;
    void keyDownEvent   (Tempest::KeyEvent&   e) override;

    int  rowAt(int y) const;
    void setValueFromX(size_t row, int x);

    Tempest::Rect saveButtonRect()          const;
    Tempest::Rect loadButtonRect()          const;
    Tempest::Rect toggleButtonRect(size_t idx) const;
    int           togglesRowCount()         const;

    MainWindow&            mainWindow;
    int                    dragRow = -1;
    std::vector<ToggleDef> toggles;

    static constexpr int   rowH          = 20;
    static constexpr int   trackX        = 220;
    static constexpr int   trackW        = 400;
    static constexpr float maxVal        = 3000.f;

    static constexpr int   btnW          = 140;
    static constexpr int   btnH          = 24;
    static constexpr int   btnGap        = 8;
    static constexpr int   togglesPerRow = 3;

    std::vector<SliderDef>  extraSliders;

    Tempest::Rect extraSliderRect(size_t idx) const;
    int           extraSliderAt(int y) const;
    void          setExtraSliderFromX(size_t idx, int x);
    int           lightSlidersBottom() const;  

  };