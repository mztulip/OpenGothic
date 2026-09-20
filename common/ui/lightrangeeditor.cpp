#include "lightrangeeditor.h"

#include <Tempest/Painter>
#include <algorithm>

#include "mainwindow.h"
#include "utils/gthfont.h"
#include "utils/string_frm.h"
#include "resources.h"
#include "gothic.h"
#include "world/world.h"
#include "graphics/sky/sky.h"

using namespace Tempest;

LightRangeEditor::LightRangeEditor(MainWindow& owner):mainWindow(owner) {
    setSizePolicy(Fixed);
    setVisible(false);

    toggles.push_back({"VOB LABELS", [](){ return Gothic::inst().doVobLabels(); }, [](bool v){ Gothic::inst().setVobLabels(v); }});
    toggles.push_back({"VOB BOX",    [](){ return Gothic::inst().doVobBox();    }, [](bool v){ Gothic::inst().setVobBox(v);    }});
    toggles.push_back({"VOB RAYS",   [](){ return Gothic::inst().doVobRays();   }, [](bool v){ Gothic::inst().setVobRays(v);   }});
    toggles.push_back({"FPS",        [](){ return Gothic::inst().doFrate();     }, [](bool v){ Gothic::inst().setFRate(v);     }});
    toggles.push_back({"CLOCK",      [](){ return Gothic::inst().doClock();     }, [](bool v){ Gothic::inst().setClock(v);     }});
    toggles.push_back({"AMBIENT", [](){ return Gothic::inst().doAmbient(); }, [](bool v){ Gothic::inst().setAmbient(v); }});
    toggles.push_back({"SKY",     [](){ return Gothic::inst().doSkyDraw(); }, [](bool v){ Gothic::inst().setSkyDraw(v); }});
    toggles.push_back({"SUN/MOON",[](){ return Gothic::inst().doSunMoon(); }, [](bool v){ Gothic::inst().setSunMoon(v); }});
    toggles.push_back({"FOG/RAYS", [](){ return Gothic::inst().doFog(); }, [](bool v){ Gothic::inst().setFog(v); }});

    extraSliders.push_back({"Sun mul",     [](){ return Sky::sunMultiplier(); },     [](float v){ Sky::sunMultiplier()     = v; }, 0.f, 3.f});
    extraSliders.push_back({"Ambient mul", [](){ return Sky::ambientMultiplier(); }, [](float v){ Sky::ambientMultiplier() = v; }, 0.f, 3.f});
    extraSliders.push_back({"Moon mul", [](){ return Sky::moonMultiplier(); }, [](float v){ Sky::moonMultiplier() = v; }, 0.f, 5.f});
    extraSliders.push_back({"Night ambient", [](){ return Sky::nightAmbientIntensity(); }, [](float v){ Sky::nightAmbientIntensity() = v; }, 0.f, 2.f});
}

void LightRangeEditor::toggle() {
  setVisible(!isVisible());
  }

int LightRangeEditor::togglesRowCount() const {
  return int((toggles.size()+togglesPerRow-1)/togglesPerRow);
  }

Rect LightRangeEditor::saveButtonRect() const {
  auto& table = LightGroup::rangeMap();
  int y = 10 + int(table.size())*rowH + 6;
  return Rect(10, y, 80, btnH);
  }

Rect LightRangeEditor::loadButtonRect() const {
  auto r = saveButtonRect();
  r.x += r.w + 10;
  return r;
  }

Rect LightRangeEditor::toggleButtonRect(size_t idx) const {
  auto save = saveButtonRect();
  int  y0   = save.y + btnH + 14;

  int col = int(idx) % togglesPerRow;
  int row = int(idx) / togglesPerRow;

  return Rect(10 + col*(btnW+btnGap), y0 + row*(btnH+btnGap), btnW, btnH);
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
  float t = std::clamp(float(x-trackX)/float(trackW), 0.f, 1.f);
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
    if(auto* w = Gothic::inst().world())
      const_cast<LightGroup&>(w->view()->lights()).invalidateAll();
    update();
    return;
    }

  for(size_t i=0; i<toggles.size(); ++i) {
    if(toggleButtonRect(i).contains(e.x, e.y)) {
      toggles[i].set(!toggles[i].get());
      update();
      return;
      }
    }

   int exIdx = extraSliderAt(e.y);
    if(exIdx>=0 && e.x>=trackX && e.x<=trackX+trackW) {
    setExtraSliderFromX(size_t(exIdx), e.x);
    dragRow = -1000 - exIdx;   // specjalne kodowanie: ujemne = suwak dodatkowy
    return;
    }

  int row = rowAt(e.y);
  if(row<0 || e.x<trackX || e.x>trackX+trackW)
    return;
  dragRow = row;
  setValueFromX(size_t(row), e.x);
  }

void LightRangeEditor::mouseMoveEvent(MouseEvent& e) {
    if(dragRow<=-1000) {
        setExtraSliderFromX(size_t(-1000-dragRow), e.x);
        return;
    }
    if(dragRow<0)
        return;
    setValueFromX(size_t(dragRow), e.x);
  }

void LightRangeEditor::mouseUpEvent(MouseEvent&) {
  dragRow = -1;
  }

void LightRangeEditor::mouseWheelEvent(MouseEvent& e) {
  int row = rowAt(e.y);
  if(row<0 || e.x<trackX-40 || e.x>trackX+trackW)
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

    const int togglesRows = togglesRowCount();
    const int panelW = trackX + trackW + 20;
    int panelHcalc = extraSliders.empty()
        ? (saveButtonRect().y + btnH + 14 + togglesRows*(btnH+btnGap) + 10)
        : (lightSlidersBottom() + btnH + 14 + togglesRows*(btnH+btnGap) + 14 + int(extraSliders.size())*rowH + 10);
    panelHcalc +=  rowH; //Ambient mode text height

    const int panelH = panelHcalc;

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

  for(size_t i=0; i<toggles.size(); ++i) {
    auto r  = toggleButtonRect(i);
    bool on = toggles[i].get();

    p.setBrush(on ? Color(0.2f,0.6f,0.2f,1.f) : Color(0.3f,0.3f,0.3f,1.f));
    p.drawRect(r);
    p.setPen(Color(1,1,1,1));
    fnt.drawText(p, r.x+8, r.y+16, toggles[i].label);
    }

    for(size_t i=0; i<extraSliders.size(); ++i) {
    auto& s = extraSliders[i];
    auto  r = extraSliderRect(i);

    string_frm label(s.label, ": ", s.get());
    p.setPen(Color(1,1,1,1));
    fnt.drawText(p, 10, r.y+rowH-14, label);

    p.setBrush(Color(0.2f,0.2f,0.2f,1.f));
    p.drawRect(r);

    float t = std::clamp((s.get()-s.minV)/(s.maxV-s.minV), 0.f, 1.f);
    p.setBrush(Color(0.7f,0.3f,0.8f,1.f));
    p.drawRect(r.x, r.y, int(float(r.w)*t), r.h);
    }

    y = extraSliders.empty()
      ? (saveButtonRect().y + btnH + 14 + togglesRowCount()*(btnH+btnGap) + 20)
      : (extraSliderRect(extraSliders.size()-1).y + rowH + 10);

    string_frm info("Ambient mode: ", Renderer::ambientModeName());
    p.setPen(Color(0.6f,1.0f,0.6f,1.f));
    fnt.drawText(p, 10, y, info);
  }

int LightRangeEditor::lightSlidersBottom() const {
  auto& table = LightGroup::rangeMap();
  return 10 + int(table.size())*rowH;
  }

Rect LightRangeEditor::extraSliderRect(size_t idx) const {
  int y = lightSlidersBottom() + btnH + 14 + togglesRowCount()*(btnH+btnGap) + 14 + int(idx)*rowH;
  return Rect(trackX, y+4, trackW, rowH-10);
  }

int LightRangeEditor::extraSliderAt(int y) const {
  int y0 = lightSlidersBottom() + btnH + 14 + togglesRowCount()*(btnH+btnGap) + 14;
  int row = (y-y0)/rowH;
  if(row<0 || size_t(row)>=extraSliders.size())
    return -1;
  return row;
  }

void LightRangeEditor::setExtraSliderFromX(size_t idx, int x) {
  auto& s = extraSliders[idx];
  float t = std::clamp(float(x-trackX)/float(trackW), 0.f, 1.f);
  s.set(s.minV + t*(s.maxV-s.minV));
  update();
  }