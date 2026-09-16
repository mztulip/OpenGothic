#pragma once

#include <Tempest/Widget>
#include "graphics/lightgroup.h"

class MainWindow;

class LightRangeEditor : public Tempest::Widget {
  public:
    LightRangeEditor(MainWindow& owner);

    void toggle();

  private:
    void paintEvent     (Tempest::PaintEvent& e) override;
    void mouseDownEvent (Tempest::MouseEvent& e) override;
    void mouseUpEvent   (Tempest::MouseEvent& e) override;
    void mouseMoveEvent (Tempest::MouseEvent& e) override;
    void mouseWheelEvent(Tempest::MouseEvent& e) override;
    void keyDownEvent   (Tempest::KeyEvent&   e) override;
    Tempest::Rect saveButtonRect() const;
    Tempest::Rect loadButtonRect() const;
    
    int  rowAt(int y) const;
    void setValueFromX(size_t row, int x);

    MainWindow& mainWindow;
    int         dragRow = -1;

    static constexpr int   rowH   = 20;
    static constexpr int   trackX = 220;
    static constexpr int   trackW = 400;
    static constexpr float maxVal = 3000.f;
  };