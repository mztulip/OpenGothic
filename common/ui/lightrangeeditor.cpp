#include "lightrangeeditor.h"

#include <Tempest/Painter>
#include <algorithm>

#include "mainwindow.h"
#include "utils/gthfont.h"
#include "utils/string_frm.h"
#include "resources.h"
#include "gothic.h"
#include "world/world.h"

using namespace Tempest;


LightRangeEditor::LightRangeEditor(MainWindow& owner):mainWindow(owner) {
  setSizePolicy(Fixed);
  setSizeHint(Size(trackX+trackW+20, 10));
  setVisible(false);
  }



void LightRangeEditor::toggle() {
  setVisible(!isVisible());
  }

int LightRangeEditor::rowAt(int y) const {
  auto&  table = LightGroup::rangeMap();
  int    row   = (y-10)/rowH;
  if(row<0 || size_t(row)>=table.size())
    return -1;
  return row;
  }

void LightRangeEditor::setValueFromX(size_t row, int x) {
  auto& table = LightGroup::rangeMap();
  float t = float(x-trackX)/float(trackW);
  t = std::clamp(t, 0.f, 1.f);
  float newVal = t*maxVal;
  if(std::abs(table[row].corrected - newVal) < 0.5f)  // nic sie nie zmienilo - pomin
    return;

  table[row].corrected = t*maxVal;

    if(auto* w = Gothic::inst().world())
    const_cast<LightGroup&>(w->view()->lights()).invalidateAll();


  update();
  }

void LightRangeEditor::mouseDownEvent(MouseEvent& e) {
  int row = rowAt(e.y);
  if(row<0 || e.x<trackX || e.x>trackX+trackW)
    return;
  dragRow = row;
  setValueFromX(size_t(row), e.x);
  }

void LightRangeEditor::mouseMoveEvent(MouseEvent& e) {
  if(dragRow<0)
    return;
  setValueFromX(size_t(dragRow), e.x);
  }

void LightRangeEditor::mouseUpEvent(MouseEvent&) {
  dragRow = -1;
  }

void LightRangeEditor::mouseWheelEvent(MouseEvent& e) {
  int row = rowAt(e.y);
  if(row<0)
    return;

  if(row<0 || e.x<trackX-40 || e.x>trackX+trackW)  // -40 zeby objac tez etykiete
    return;
  auto& table = LightGroup::rangeMap();
  table[size_t(row)].corrected = std::max(0.f, table[size_t(row)].corrected + (e.delta>0 ? 100.f : -100.f));

    if(auto* w = Gothic::inst().world())
    const_cast<LightGroup&>(w->view()->lights()).invalidateAll();

  update();
  }

void LightRangeEditor::keyDownEvent(KeyEvent& e) {
  if(e.key==Event::K_ESCAPE)
    setVisible(false);
  }

void LightRangeEditor::paintEvent(PaintEvent& e) {
  auto& table = LightGroup::rangeMap();

  Painter p(e);

  const int panelW = trackX + trackW + 20;
  const int panelH = 10 + int(table.size())*rowH + 10;

  // tło TYLKO pod panelem, nie pod całym ekranem
  p.setBrush(Color(0,0,0,0.75f));
  p.drawRect(0, 0, panelW, panelH);

  const float scale = Gothic::interfaceScale(&mainWindow);
  auto& fnt = Resources::font(scale);

  int y = 10;
  for(size_t i=0; i<table.size(); ++i) {
    auto& pt = table[i];

    string_frm label(i, ": orig=", int(pt.original), "  corr=", int(pt.corrected));
    p.setPen(Color(1,1,1,1));
    fnt.drawText(p, 10, y+rowH-6, label);

    p.setBrush(Color(0.2f,0.2f,0.2f,1.f));
    p.drawRect(trackX, y+4, trackW, rowH-10);

    float t = std::clamp(pt.corrected/maxVal, 0.f, 1.f);
    p.setBrush(dragRow==int(i) ? Color(1.0f,0.6f,0.1f,1.f) : Color(0.2f,0.6f,1.f,1.f));
    p.drawRect(trackX, y+4, int(trackW*t), rowH-10);

    y += rowH;
    }
  }