#include "lightrangeeditor.h"

#include <Tempest/Painter>
#include <Tempest/Rect>
#include <Tempest/Log>
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

  if(std::abs(table[row].corrected - newVal) < 0.5f)
    return;

  table[row].corrected = newVal;

  if(auto* w = Gothic::inst().world())
    const_cast<LightGroup&>(w->view()->lights()).invalidateAll();

  update();
  }

void LightRangeEditor::mouseDownEvent(MouseEvent& e) {
  if(saveButtonRect().contains(e.x, e.y)) {
    LightGroup::saveRangeMap();
    return;
    }

  if(loadButtonRect().contains(e.x, e.y)) {
    LightGroup::loadRangeMap();

    auto& table = LightGroup::rangeMap();
    for(size_t i=0; i<table.size(); ++i)
        Tempest::Log::i("after load[",i,"] corrected=",table[i].corrected);

    if(auto* w = Gothic::inst().world())
      const_cast<LightGroup&>(w->view()->lights()).invalidateAll();

    dragRow = -1;
    update();
    return;
    }

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
  const int panelH = 10 + int(table.size())*rowH + 6 + 24 + 10;  // + wysokosc przyciskow + margines

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

    auto saveBtn = saveButtonRect();
    auto loadBtn = loadButtonRect();

    p.setBrush(Color(0.2f,0.5f,0.2f,1.f));
    p.drawRect(saveBtn);
    p.setPen(Color(1,1,1,1));
    fnt.drawText(p, saveBtn.x+16, saveBtn.y+16, "SAVE");

    p.setBrush(Color(0.5f,0.35f,0.15f,1.f));
    p.drawRect(loadBtn);
    p.setPen(Color(1,1,1,1));
    fnt.drawText(p, loadBtn.x+16, loadBtn.y+16, "LOAD");
  }

Rect LightRangeEditor::saveButtonRect() const {
  auto& table = LightGroup::rangeMap();
  int btnY = 10 + int(table.size())*rowH + 6;
  return Rect(10, btnY, 80, 24);
  }

Rect LightRangeEditor::loadButtonRect() const {
  auto r = saveButtonRect();
  r.x += r.w + 10;
  return r;
  }