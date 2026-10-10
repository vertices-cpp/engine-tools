#include "ParticleConfig.h"
#include "ParticleSystem.h"
#include "tinyxml2.h"

#include <cstring>
#include <cstdlib>
#include <unordered_map>

using namespace tinyxml2;

// ---------- 写工具 ----------
static void addReal(XMLDocument& doc, XMLElement* dict, const char* key, float v)
{
    auto* k = doc.NewElement("key");    k->SetText(key);   dict->InsertEndChild(k);
    auto* e = doc.NewElement("real");   e->SetText(v);     dict->InsertEndChild(e);
}
static void addInt(XMLDocument& doc, XMLElement* dict, const char* key, int v)
{
    auto* k = doc.NewElement("key");     k->SetText(key);  dict->InsertEndChild(k);
    auto* e = doc.NewElement("integer"); e->SetText(v);    dict->InsertEndChild(e);
}
static void addString(XMLDocument& doc, XMLElement* dict, const char* key, const char* v)
{
    auto* k = doc.NewElement("key");    k->SetText(key);   dict->InsertEndChild(k);
    auto* e = doc.NewElement("string"); e->SetText(v);     dict->InsertEndChild(e);
}

// ============================================================
// saveToPlist
// ============================================================
bool ParticleConfig::saveToPlist(const char* path) const
{
    XMLDocument doc;
    doc.InsertFirstChild(doc.NewUnknown(
        "DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
        "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\""));
    doc.InsertFirstChild(doc.NewDeclaration("xml version=\"1.0\" encoding=\"utf-8\""));
    

    auto* plist = doc.NewElement("plist");
    plist->SetAttribute("version", "1.0");
    doc.InsertEndChild(plist);

    auto* dict = doc.NewElement("dict");
    plist->InsertEndChild(dict);

    addReal(doc, dict, "angle", angle);
    addReal(doc, dict, "angleVariance", angleVariance);
    addInt(doc, dict, "blendFuncDestination", blendFuncDestination);
    addInt(doc, dict, "blendFuncSource", blendFuncSource);
    addReal(doc, dict, "duration", duration);
    addReal(doc, dict, "emitterType", (float)emitterType);
    addReal(doc, dict, "emissionRate", emissionRate);

    addReal(doc, dict, "finishColorAlpha", finishColorAlpha);
    addReal(doc, dict, "finishColorBlue", finishColorBlue);
    addReal(doc, dict, "finishColorGreen", finishColorGreen);
    addReal(doc, dict, "finishColorRed", finishColorRed);
    addReal(doc, dict, "finishColorVarianceAlpha", finishColorVarianceAlpha);
    addReal(doc, dict, "finishColorVarianceBlue", finishColorVarianceBlue);
    addReal(doc, dict, "finishColorVarianceGreen", finishColorVarianceGreen);
    addReal(doc, dict, "finishColorVarianceRed", finishColorVarianceRed);

    addReal(doc, dict, "rotationStart", rotationStart);
    addReal(doc, dict, "rotationStartVariance", rotationStartVariance);
    addReal(doc, dict, "rotationEnd", rotationEnd);
    addReal(doc, dict, "rotationEndVariance", rotationEndVariance);

    addReal(doc, dict, "finishParticleSize", finishParticleSize);
    addReal(doc, dict, "finishParticleSizeVariance", finishParticleSizeVariance);

    addReal(doc, dict, "gravityx", -gravityx);
    addReal(doc, dict, "gravityy", -gravityy);

    addReal(doc, dict, "maxParticles", (float)maxParticles);

    addReal(doc, dict, "maxRadius", maxRadius);
    addReal(doc, dict, "maxRadiusVariance", maxRadiusVariance);
    addReal(doc, dict, "minRadius", minRadius);
    addReal(doc, dict, "minRadiusVariance", minRadiusVariance);

    addReal(doc, dict, "particleLifespan", particleLifespan);
    addReal(doc, dict, "particleLifespanVariance", particleLifespanVariance);

    addReal(doc, dict, "radialAccelVariance", radialAccelVariance);
    addReal(doc, dict, "radialAcceleration", radialAcceleration);

    addReal(doc, dict, "rotatePerSecond", rotatePerSecond);
    addReal(doc, dict, "rotatePerSecondVariance", rotatePerSecondVariance);

    addReal(doc, dict, "sourcePositionVariancex", sourcePositionVariancex);
    addReal(doc, dict, "sourcePositionVariancey", sourcePositionVariancey);
    addReal(doc, dict, "sourcePositionx", sourcePositionx);
    addReal(doc, dict, "sourcePositiony", sourcePositiony);

    addReal(doc, dict, "speed", -speed);
    addReal(doc, dict, "speedVariance", speedVariance);

    addReal(doc, dict, "startColorAlpha", startColorAlpha);
    addReal(doc, dict, "startColorBlue", startColorBlue);
    addReal(doc, dict, "startColorGreen", startColorGreen);
    addReal(doc, dict, "startColorRed", startColorRed);
    addReal(doc, dict, "startColorVarianceAlpha", startColorVarianceAlpha);
    addReal(doc, dict, "startColorVarianceBlue", startColorVarianceBlue);
    addReal(doc, dict, "startColorVarianceGreen", startColorVarianceGreen);
    addReal(doc, dict, "startColorVarianceRed", startColorVarianceRed);

    addReal(doc, dict, "startParticleSize", startParticleSize);
    addReal(doc, dict, "startParticleSizeVariance", startParticleSizeVariance);

    addReal(doc, dict, "tangentialAccelVariance", tangentialAccelVariance);
    addReal(doc, dict, "tangentialAcceleration", tangentialAcceleration);

    addString(doc, dict, "textureFileName", textureFileName.c_str());

    addInt(doc, dict, "yCoordFlipped", yCoordFlipped);

    return doc.SaveFile(path) == XML_SUCCESS;
}

// ============================================================
// loadFromPlist
// ============================================================
static bool parsePlistDict(XMLElement* dict,
    std::unordered_map<std::string, std::string>& out)
{
    if (!dict) return false;
    for (auto* e = dict->FirstChildElement(); e; e = e->NextSiblingElement())
    {
        if (std::strcmp(e->Name(), "key") != 0) continue;
        const char* key = e->GetText();
        if (!key) continue;
        auto* val = e->NextSiblingElement();
        if (!val) break;
        const char* t = val->GetText();
        out[key] = t ? t : "";
    }
    return true;
}

bool ParticleConfig::loadFromPlist(const char* path)
{
    XMLDocument doc;
    if (doc.LoadFile(path) != XML_SUCCESS) return false;

    auto* plist = doc.FirstChildElement("plist");
    if (!plist) return false;
    auto* dict = plist->FirstChildElement("dict");
    if (!dict) return false;

    std::unordered_map<std::string, std::string> kv;
    if (!parsePlistDict(dict, kv)) return false;

    auto F = [&](const char* k, float def = 0.0f) -> float {
        auto it = kv.find(k);
        return it == kv.end() ? def : (float)std::atof(it->second.c_str());
    };
    auto I = [&](const char* k, int def = 0) -> int {
        auto it = kv.find(k);
        return it == kv.end() ? def : std::atoi(it->second.c_str());
    };
    auto S = [&](const char* k, const char* def = "") -> std::string {
        auto it = kv.find(k);
        return it == kv.end() ? std::string(def) : it->second;
    };

    angle = F("angle", 90.0f);
    angleVariance = F("angleVariance", 0.0f);
    blendFuncDestination = I("blendFuncDestination", 771);
    blendFuncSource = I("blendFuncSource", 770);
    duration = F("duration", -1.0f);
    emitterType = I("emitterType", 0);
   // emissionRate = F("emissionRate", 10.0f);
	// 改成：
	if (kv.find("emissionRate") != kv.end()) {
		emissionRate = F("emissionRate", 10.0f);
	}
	else {
		emissionRate = (particleLifespan > 0.0f) ? (maxParticles / particleLifespan) : 10.0f;
	}

    finishColorAlpha = F("finishColorAlpha", 1.0f);
    finishColorBlue = F("finishColorBlue", 1.0f);
    finishColorGreen = F("finishColorGreen", 1.0f);
    finishColorRed = F("finishColorRed", 1.0f);
    finishColorVarianceAlpha = F("finishColorVarianceAlpha", 0.0f);
    finishColorVarianceBlue = F("finishColorVarianceBlue", 0.0f);
    finishColorVarianceGreen = F("finishColorVarianceGreen", 0.0f);
    finishColorVarianceRed = F("finishColorVarianceRed", 0.0f);

    rotationStart = F("rotationStart", 0.0f);
    rotationStartVariance = F("rotationStartVariance", 0.0f);
    rotationEnd = F("rotationEnd", 0.0f);
    rotationEndVariance = F("rotationEndVariance", 0.0f);

    finishParticleSize = F("finishParticleSize", -1.0f);
    finishParticleSizeVariance = F("finishParticleSizeVariance", 0.0f);

    gravityx = -F("gravityx", 0.0f);
    gravityy = -F("gravityy", 0.0f);

    maxParticles = I("maxParticles", 150);

    maxRadius = F("maxRadius", 0.0f);
    maxRadiusVariance = F("maxRadiusVariance", 0.0f);
    minRadius = F("minRadius", 0.0f);
    minRadiusVariance = F("minRadiusVariance", 0.0f);

    particleLifespan = F("particleLifespan", 1.0f);
    particleLifespanVariance = F("particleLifespanVariance", 0.0f);

    radialAccelVariance = F("radialAccelVariance", 0.0f);
    radialAcceleration = F("radialAcceleration", 0.0f);

    rotatePerSecond = F("rotatePerSecond", 0.0f);
    rotatePerSecondVariance = F("rotatePerSecondVariance", 0.0f);

    sourcePositionVariancex = F("sourcePositionVariancex", 0.0f);
    sourcePositionVariancey = F("sourcePositionVariancey", 0.0f);
    sourcePositionx = F("sourcePositionx", 0.0f);
    sourcePositiony = F("sourcePositiony", 0.0f);

    speed = -F("speed", 0.0f);
    speedVariance = F("speedVariance", 0.0f);

    startColorAlpha = F("startColorAlpha", 1.0f);
    startColorBlue = F("startColorBlue", 1.0f);
    startColorGreen = F("startColorGreen", 1.0f);
    startColorRed = F("startColorRed", 1.0f);
    startColorVarianceAlpha = F("startColorVarianceAlpha", 0.0f);
    startColorVarianceBlue = F("startColorVarianceBlue", 0.0f);
    startColorVarianceGreen = F("startColorVarianceGreen", 0.0f);
    startColorVarianceRed = F("startColorVarianceRed", 0.0f);

    startParticleSize = F("startParticleSize", 10.0f);
    startParticleSizeVariance = F("startParticleSizeVariance", 0.0f);

    tangentialAccelVariance = F("tangentialAccelVariance", 0.0f);
    tangentialAcceleration = F("tangentialAcceleration", 0.0f);

    textureFileName = S("textureFileName", "fire.png");

    yCoordFlipped = I("yCoordFlipped", 1);

    return true;
}

// ============================================================
// applyTo
// ============================================================
void ParticleConfig::applyTo(ParticleSystem* ps) const
{
    if (!ps) return;
// 	ps->resetSystem();
// 	 ps->setTotalParticles(maxParticles);
	
	if (!ps->initWithTotalParticles(maxParticles))
	{
		return ;
	} 
 
    ps->setDuration(duration);
    ps->setEmitterMode((ParticleSystem::Mode)emitterType);
    ps->setEmissionRate(emissionRate);

    ps->setAngle(angle);
    ps->setAngleVar(angleVariance);

    Color4F sc;  sc.r = startColorRed;  sc.g = startColorGreen;
    sc.b = startColorBlue; sc.a = startColorAlpha;
    ps->setStartColor(sc);

    Color4F scv; scv.r = startColorVarianceRed;  scv.g = startColorVarianceGreen;
    scv.b = startColorVarianceBlue; scv.a = startColorVarianceAlpha;
    ps->setStartColorVar(scv);

    Color4F ec;  ec.r = finishColorRed;  ec.g = finishColorGreen;
    ec.b = finishColorBlue; ec.a = finishColorAlpha;
    ps->setEndColor(ec);

    Color4F ecv; ecv.r = finishColorVarianceRed;  ecv.g = finishColorVarianceGreen;
    ecv.b = finishColorVarianceBlue; ecv.a = finishColorVarianceAlpha;
    ps->setEndColorVar(ecv);

    ps->setStartSize(startParticleSize);
    ps->setStartSizeVar(startParticleSizeVariance);
    ps->setEndSize(finishParticleSize);
    ps->setEndSizeVar(finishParticleSizeVariance);

    ps->setStartSpin(rotationStart);
    ps->setStartSpinVar(rotationStartVariance);
    ps->setEndSpin(rotationEnd);
    ps->setEndSpinVar(rotationEndVariance);

    ps->setLife(particleLifespan);
    ps->setLifeVar(particleLifespanVariance);

    {
        Vec2 pos(sourcePositionx, sourcePositiony);
        ps->setPosition(pos);
    }
    ps->setPosVar(Vec2(sourcePositionVariancex, sourcePositionVariancey));

    ps->setGravity(Vec2(gravityx, gravityy));
    ps->setSpeed(speed);
    ps->setSpeedVar(speedVariance);
    ps->setRadialAccel(radialAcceleration);
    ps->setRadialAccelVar(radialAccelVariance);
    ps->setTangentialAccel(tangentialAcceleration);
    ps->setTangentialAccelVar(tangentialAccelVariance);

    ps->setStartRadius(maxRadius);
    ps->setStartRadiusVar(maxRadiusVariance);
    ps->setEndRadius(minRadius);
    ps->setEndRadiusVar(minRadiusVariance);
    ps->setRotatePerSecond(rotatePerSecond);
    ps->setRotatePerSecondVar(rotatePerSecondVariance);

    bool additive = (blendFuncSource == 770 && blendFuncDestination == 1);
    ps->setBlendAdditive(additive);
}

// ============================================================
// collectFrom
// ============================================================
void ParticleConfig::collectFrom(const ParticleSystem* ps)
{
    if (!ps) return;

    maxParticles = ps->getTotalParticles();
    duration = ps->getDuration();
    emitterType = (int)ps->getEmitterMode();
    emissionRate = ps->getEmissionRate();

    angle = ps->getAngle();
    angleVariance = ps->getAngleVar();

    Color4F sc = ps->getStartColor();
    startColorRed = sc.r; startColorGreen = sc.g;
    startColorBlue = sc.b; startColorAlpha = sc.a;

    Color4F scv = ps->getStartColorVar();
    startColorVarianceRed = scv.r;
    startColorVarianceGreen = scv.g;
    startColorVarianceBlue = scv.b;
    startColorVarianceAlpha = scv.a;

    Color4F ec = ps->getEndColor();
    finishColorRed = ec.r; finishColorGreen = ec.g;
    finishColorBlue = ec.b; finishColorAlpha = ec.a;

    Color4F ecv = ps->getEndColorVar();
    finishColorVarianceRed = ecv.r;
    finishColorVarianceGreen = ecv.g;
    finishColorVarianceBlue = ecv.b;
    finishColorVarianceAlpha = ecv.a;

    startParticleSize = ps->getStartSize();
    startParticleSizeVariance = ps->getStartSizeVar();
    finishParticleSize = ps->getEndSize();
    finishParticleSizeVariance = ps->getEndSizeVar();

    rotationStart = ps->getStartSpin();
    rotationStartVariance = ps->getStartSpinVar();
    rotationEnd = ps->getEndSpin();
    rotationEndVariance = ps->getEndSpinVar();

    particleLifespan = ps->getLife();
    particleLifespanVariance = ps->getLifeVar();

    {
        Vec2 p = const_cast<ParticleSystem*>(ps)->getPosition();
        sourcePositionx = p.x;
        sourcePositiony = p.y;
    }

    Vec2 pv = ps->getPosVar();
    sourcePositionVariancex = pv.x;
    sourcePositionVariancey = pv.y;

    Vec2 g = const_cast<ParticleSystem*>(ps)->getGravity();
    gravityx = g.x;
    gravityy = g.y;

    speed = ps->getSpeed();
    speedVariance = ps->getSpeedVar();

    radialAcceleration = ps->getRadialAccel();
    radialAccelVariance = ps->getRadialAccelVar();

    tangentialAcceleration = ps->getTangentialAccel();
    tangentialAccelVariance = ps->getTangentialAccelVar();

    maxRadius = ps->getStartRadius();
    maxRadiusVariance = ps->getStartRadiusVar();
    minRadius = ps->getEndRadius();
    minRadiusVariance = ps->getEndRadiusVar();
    rotatePerSecond = ps->getRotatePerSecond();
    rotatePerSecondVariance = ps->getRotatePerSecondVar();

    if (ps->isBlendAdditive()) {
        blendFuncSource = 770; blendFuncDestination = 1;
    }
    else {
        blendFuncSource = 770; blendFuncDestination = 771;
    }
}
