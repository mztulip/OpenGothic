#include "lightgroup.h"

#include <Tempest/Dir>
#include <Tempest/Log>

#include <zenkit/Archive.hh>

#include "graphics/shaders.h"
#include "graphics/sceneglobals.h"
#include "utils/string_frm.h"
#include "world/world.h"
#include "utils/dbgpainter.h"
#include "gothic.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wtemplate-body"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>
#pragma GCC diagnostic pop
#include <Tempest/File>

#include "utils/fileutil.h"

using namespace Tempest;

static float clampRange(float r) {
  return std::min(r, 2000.f); //this could be reduced to 300
  }

static const char16_t* RANGE_MAP_FILE = u"lightranges.json";

static void loadRangeMapOverrides(std::vector<LightGroup::RangeMapPoint>& table) {
  if(!FileUtil::exists(RANGE_MAP_FILE))
    return;

  try {
    Tempest::RFile f(RANGE_MAP_FILE);
    std::string    json(f.size(), ' ');
    f.read(json.data(), json.size());

    rapidjson::Document doc;
    doc.Parse(json.c_str());
    if(doc.HasParseError() || !doc.IsArray()) {
      Log::e("lightranges.json: parse error - ", rapidjson::GetParseError_En(doc.GetParseError()));
      return;
      }

    for(auto& e : doc.GetArray()) {
      if(!e.IsObject() || !e.HasMember("original") || !e.HasMember("corrected"))
        continue;
      float orig = float(e["original"].GetDouble());
      float corr = float(e["corrected"].GetDouble());
      for(auto& p : table) {
        if(std::abs(p.original-orig) < 0.01f) {
          p.corrected = corr;
          break;
          }
        }
      }
    Log::i("lightranges.json: loaded ", doc.Size(), " overrides");
    }
  catch(...) {
    Log::e("unable to read \"lightranges.json\"");
    }
  }

std::vector<LightGroup::RangeMapPoint>& LightGroup::rangeMap() {
  static std::vector<RangeMapPoint> table = [](){
    std::vector<RangeMapPoint> t = {
      {   15.f,  100.f },
      {   50.f,  100.f },
      {   80.f,  100.f },
      {  100.f,  100.f },
      {  150.f,  150.f },
      {  200.f,  200.f },
      {  250.f,  250.f },
      {  300.f,  300.f },
      {  350.f,  350.f },
      {  400.f,  400.f },
      {  500.f,  500.f },
      {  600.f,  600.f },
      {  650.f,  650.f },
      {  700.f,  700.f },
      {  800.f,  800.f },
      {  900.f,  900.f },
      { 1000.f, 1000.f },
      { 1200.f,  300.f },
      { 1500.f, 3000.f },
      { 2000.f, 3000.f },
      { 3000.f, 3000.f },
      };
    loadRangeMapOverrides(t);   // <- nadpisz wartościami z lightranges.json, jesli istnieje
    return t;
    }();
  return table;
  }

float LightGroup::correctedRange(float range) {
  auto&        table = rangeMap();
  const size_t count = table.size();

  if(range<=table[0].original)
    return table[0].corrected;
  if(range>=table[count-1].original)
    return table[count-1].corrected;

  for(size_t i=1; i<count; ++i) {
    if(range<=table[i].original) {
      const auto& a = table[i-1];
      const auto& b = table[i];
      float t = (range-a.original)/(b.original-a.original);
      return a.corrected + t*(b.corrected-a.corrected);
      }
    }
  return range; // nieosiagalne, ale kompilator wymaga zwrotu
  }

void LightGroup::invalidateAll() {
  std::lock_guard<std::mutex> guard(sync);
  for(size_t i = 0; i < lightSourceDesc.size(); ++i) {
    // Ponownie przeliczamy SSBO na podstawie obecnego stanu opisowego światła
    lightSourceData[i] = lightToSsbo(lightSourceDesc[i]);
    // Oznaczamy światło do przesłania na GPU w następnej klatce
    markAsDurtyNoSync(i);
  }
}

 LightGroup::LightSsbo LightGroup::lightToSsbo(const LightSource& l) {
  LightGroup::LightSsbo lx;

  lx.pos   = l.position();
  lx.color = l.currentColor();
  lx.range = l.isEnabled()
           ? clampRange(correctedRange(l.currentRange()))
           : 0;

  return lx;
}


LightGroup::Light::Light(LightGroup::Light&& oth):owner(oth.owner), id(oth.id) {
  oth.owner = nullptr;
  }

LightGroup::Light& LightGroup::Light::operator =(LightGroup::Light&& other) {
  std::swap(owner,other.owner);
  std::swap(id,other.id);
  return *this;
  }

LightGroup::Light::~Light() {
  if(owner!=nullptr)
    owner->free(id);
  }

void LightGroup::Light::setPosition(float x, float y, float z) {
  setPosition(Vec3(x,y,z));
  }

void LightGroup::Light::setPosition(const Vec3& p) {
  if(owner==nullptr)
    return;
  auto& data = owner->lightSourceDesc[id];
  data.setPosition(p);

  auto& ssbo = owner->lightSourceData[id];
  ssbo.pos = p;
  owner->markAsDurty(id);
  }

void LightGroup::Light::setEnabled(bool e) {
  if(owner==nullptr)
    return;
  auto& data = owner->lightSourceDesc[id];
  data.setEnabled(e);

  auto& ssbo = owner->lightSourceData[id];
  ssbo.range = 0;
  owner->markAsDurty(id);
  }

void LightGroup::Light::setRange(float r) {
  if(owner==nullptr)
    return;
  auto& data = owner->lightSourceDesc[id];
  data.setRange(r);

  owner->lightSourceData[id] = lightToSsbo(data);
  owner->markAsDurty(id);
  }

void LightGroup::Light::setColor(const Vec3& c) {
  if(owner==nullptr)
    return;
  auto& data = owner->lightSourceDesc[id];
  data.setColor(c);

  auto& ssbo = owner->lightSourceData[id];
  ssbo.color = c;
  owner->markAsDurty(id);
  }

void LightGroup::Light::setColor(const std::vector<Vec3>& c, float fps, bool smooth) {
  if(owner==nullptr)
    return;
  auto& data = owner->lightSourceDesc[id];
  data.setColor(c,fps,smooth);

  auto& ssbo = owner->lightSourceData[id];
  ssbo.color = data.currentColor();
  owner->markAsDurty(id);
  }

void LightGroup::Light::setTimeOffset(uint64_t t) {
  if(owner==nullptr)
    return;
  auto& data = owner->lightSourceDesc[id];
  data.setTimeOffset(t);
  }

uint64_t LightGroup::Light::effectPrefferedTime() const {
  if(owner==nullptr)
    return 0;
  auto& data = owner->lightSourceDesc[id];
  return data.effectPrefferedTime();
  }

LightGroup::LightGroup(const SceneGlobals& scene) {
  try {
    std::unique_ptr<zenkit::Read> read;
    auto zen = Resources::openReader("LIGHTPRESETS.ZEN", read);

    zenkit::ArchiveObject obj {};
    auto count = zen->read_int();
    for(int i = 0; i < count; ++i) {
      zen->read_object_begin(obj);

      zenkit::LightPreset preset {};
      preset.load(*zen, Gothic::inst().version().game == 1 ? zenkit::GameVersion::GOTHIC_1
                                                           : zenkit::GameVersion::GOTHIC_2);
      presets.emplace_back(std::move(preset));

      if(!zen->read_object_end()) {
        zen->skip_object(true);
        }
      }
    }
  catch(...) {
    Log::e("unable to load Zen-file: \"LIGHTPRESETS.ZEN\"");
    }
  }

LightGroup::Light LightGroup::add(const zenkit::LightPreset& vob) {
  LightSource l;
  l.setPosition(Vec3(0, 0, 0));
  l.setDebugName(vob.preset);

  if(!vob.range_animation_scale.empty()) {
    l.setRange(vob.range_animation_scale,vob.range,vob.range_animation_fps,vob.range_animation_smooth);
    } else {
    l.setRange(vob.range);
    }

  if(!vob.color_animation_list.empty()) {
    l.setColor(vob.color_animation_list,vob.color_animation_fps,vob.color_animation_smooth);
    } else {
    l.setColor(Vec3(vob.color.r / 255.f, vob.color.g / 255.f, vob.color.b / 255.f));
    }

  std::lock_guard<std::mutex> guard(sync);
  size_t id = alloc(l.isDynamic());
  auto   lx = Light(*this, id);

  auto& ssbo = lightSourceData[lx.id];
  ssbo = lightToSsbo(l);

  auto& data = lightSourceDesc[lx.id];
  data = std::move(l);

  markAsDurtyNoSync(lx.id);
  return lx;
  }

LightGroup::Light LightGroup::add(const zenkit::VLight& vob) {
  auto l = add(static_cast<const zenkit::LightPreset&>(vob));
  l.setPosition(Vec3(vob.position.x,vob.position.y,vob.position.z));
  return l;
  }

LightGroup::Light LightGroup::add(std::string_view preset) {
  return add(findPreset(preset));
  }

void LightGroup::dbgLights(DbgPainter& p) const {
  static bool ddraw=true;
  if(!ddraw)
    return;

  if(!Gothic::inst().doVobLabels())
    return;

  auto        camera   = Gothic::inst().camera();
  const float nearDist = 3000.f*3000.f;   // ten sam promień co drawVobBoxNpcNear

  p.setPen(Color(1,1,0,1.f));  // żółty, żeby odróżnić od reszty etykiet

  for(auto& i:lightSourceDesc) {
    if(!i.isEnabled())
      continue;

    auto pt = i.position();
    if(camera!=nullptr && (pt-camera->originLwc()).quadLength() > nearDist)
      continue;

    float range = correctedRange(i.currentRange());
    string_frm label("LIGHT: ", i.debugName(), "  range=", int(range));

    p.drawText(pt, label);

    //Maluje znacznik jako X w pozycji światła
    float l = 10;
    p.drawLine(pt-Vec3(l,0,0),pt+Vec3(l,0,0));
    p.drawLine(pt-Vec3(0,l,0),pt+Vec3(0,l,0));
    p.drawLine(pt-Vec3(0,0,l),pt+Vec3(0,0,l));
    /*
    float r  = i.range();
    auto  pt = i.position();
    Vec3 px[9] = {};
    px[0] = pt+Vec3(-r,-r,-r);
    px[1] = pt+Vec3( r,-r,-r);
    px[2] = pt+Vec3( r, r,-r);
    px[3] = pt+Vec3(-r, r,-r);
    px[4] = pt+Vec3(-r,-r, r);
    px[5] = pt+Vec3( r,-r, r);
    px[6] = pt+Vec3( r, r, r);
    px[7] = pt+Vec3(-r, r, r);
    px[8] = pt;

    for(auto& i:px) {
      p.mvp.project(i.x,i.y,i.z);
      i.x = (i.x+1.f)*0.5f;
      i.y = (i.y+1.f)*0.5f;
      }

    int x = int(px[8].x*float(p.w));
    int y = int(px[8].y*float(p.h));

    int x0 = x, x1 = x;
    int y0 = y, y1 = y;
    float z0=px[8].z, z1=px[8].z;

    for(auto& i:px) {
      int x = int(i.x*float(p.w));
      int y = int(i.y*float(p.h));
      x0 = std::min(x0, x);
      y0 = std::min(y0, y);
      x1 = std::max(x1, x);
      y1 = std::max(y1, y);
      z0 = std::min(z0, i.z);
      z1 = std::max(z1, i.z);
      }

    if(z1<0.f || z0>1.f)
      continue;
    if(x1<0 || x0>int(p.w))
      continue;
    if(y1<0 || y0>int(p.h))
      continue;

    cnt++;
    p.painter.drawRect(x0,y0,x1-x0,y1-y0);
    p.painter.drawRect(x0,y0,3,3);
    */
    }

  string_frm name("light count = ", lightSourceDesc.size());
  p.drawText(10,50,name);
  }

size_t LightGroup::alloc(bool dynamic) {
  if(freeList.size()>0) {
    auto ret = freeList.back();
    freeList.pop_back();
    if(dynamic)
      animatedLights.insert(ret);
    markAsDurtyNoSync(ret);
    return ret;
    }
  lightSourceData.emplace_back();
  lightSourceDesc.emplace_back();
  duryBit.resize((lightSourceData.size()+32u-1u)/32u);

  auto ret = lightSourceData.size()-1;
  if(dynamic)
    animatedLights.insert(ret);
  markAsDurtyNoSync(ret);
  return ret;
  }

void LightGroup::free(size_t id) {
  std::lock_guard<std::mutex> guard(sync);
  markAsDurtyNoSync(id);
  animatedLights.erase(id);
  if(id+1==lightSourceData.size()) {
    lightSourceData.pop_back();
    lightSourceDesc.pop_back();
    duryBit.resize((lightSourceData.size()+32u-1u)/32u);
    } else {
    lightSourceDesc[id].setRange(0);
    lightSourceData[id] = LightSsbo();
    freeList.push_back(id);
    }
  }

void LightGroup::markAsDurty(size_t id) {
  std::lock_guard<std::mutex> guard(sync);
  markAsDurtyNoSync(id);
  }

void LightGroup::markAsDurtyNoSync(size_t id) {
  duryBit[id/32] |= (1u << (id%32));
  }

void LightGroup::resetDurty() {
  std::memset(duryBit.data(), 0, duryBit.size()*sizeof(duryBit[0]));
  }

const zenkit::LightPreset& LightGroup::findPreset(std::string_view preset) const {
  for(auto& i:presets) {
    if(i.preset!=preset)
      continue;
    return i;
    }
  Log::e("unknown light preset: \"",preset,"\"");
  static zenkit::LightPreset zero {};
  return zero;
  }

void LightGroup::tick(uint64_t time) {
  for(size_t i : animatedLights) {
    auto& light = lightSourceDesc[i];
    light.update(time);

    LightSsbo ssbo = lightToSsbo(light);

    auto& dst = lightSourceData[i];
    if(std::memcmp(&dst, &ssbo, sizeof(ssbo))==0)
      continue;
    dst = ssbo;
    markAsDurtyNoSync(i);
    }
  }

bool LightGroup::updateLights() {
  auto& device = Resources::device();

  if(lightSourceSsbo.byteSize()<lightSourceData.size()*sizeof(LightSsbo)) {
    Resources::recycle(std::move(lightSourceSsbo));
    lightSourceSsbo = device.ssbo(lightSourceData);
    resetDurty();
    return true;
    }
  return false;
  }

void LightGroup::prepareGlobals(Tempest::Encoder<Tempest::CommandBuffer>& cmd, uint8_t fId) {
  std::vector<Path>      patchBlock;
  std::vector<LightSsbo> patchData;

  for(size_t i=0; i<lightSourceDesc.size(); ++i) {
    if(i%32==0 && duryBit[i/32]==0) {
      i+=31;
      continue;
      }
    if((duryBit[i/32] & (1u<<i%32))==0)
      continue;

    patchData.push_back(lightSourceData[i]);

    Path p;
    p.dst  = uint32_t(i);
    p.src  = uint32_t(patchData.size()-1);
    p.size = 1;
    if(patchBlock.size()>0) {
      auto& b = patchBlock.back();
      const uint32_t maxBlockSize = 16;
      if(b.dst+b.size==p.dst && b.size<maxBlockSize) {
        b.size++;
        continue;
        }
      }
    patchBlock.push_back(p);
    }

  if(patchBlock.empty())
    return;
  resetDurty();

  const size_t headerSize = patchBlock.size()*sizeof(Path);
  const size_t dataSize   = patchData .size()*sizeof(LightSsbo);
  for(auto& i:patchBlock) {
    i.dst  *= uint32_t(sizeof(LightSsbo));
    i.src  *= uint32_t(sizeof(LightSsbo));
    i.size *= uint32_t(sizeof(LightSsbo));

    i.src  += uint32_t(headerSize);

    // uint's in shader
    i.dst  /= sizeof(uint32_t);
    i.src  /= sizeof(uint32_t);
    i.size /= sizeof(uint32_t);
    }

  auto& device  = Resources::device();
  auto& patch   = patchSsbo[fId];
  if(patch.byteSize()<headerSize+dataSize) {
    Resources::recycle(std::move(patch));
    patch = device.ssbo(Tempest::BufferHeap::Upload, Tempest::Uninitialized, headerSize+dataSize);
    }
  patch.update(patchBlock.data(), 0,          headerSize);
  patch.update(patchData.data(),  headerSize, dataSize);

  cmd.setFramebuffer({});
  cmd.setBinding(0, lightSourceSsbo);
  cmd.setBinding(1, patch);
  cmd.setPipeline(Shaders::inst().patch);
  cmd.dispatch(patchBlock.size());
  }

  void LightGroup::saveRangeMap() {
  auto& table = rangeMap();

  rapidjson::StringBuffer buf;
  rapidjson::Writer<rapidjson::StringBuffer> w(buf);
  w.StartArray();
  for(auto& p : table) {
    w.StartObject();
    w.Key("original");  w.Double(double(p.original));
    w.Key("corrected"); w.Double(double(p.corrected));
    w.EndObject();
    }
  w.EndArray();

  try {
    Tempest::WFile f(RANGE_MAP_FILE);
    f.write(buf.GetString(), buf.GetSize());
    f.flush();
    }
  catch(...) {
    Log::e("unable to save \"lightranges.json\"");
    }
  }

void LightGroup::loadRangeMap() {
  loadRangeMapOverrides(rangeMap());
  }