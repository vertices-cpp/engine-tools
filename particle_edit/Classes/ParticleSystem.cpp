
#include "Texture2D.h"  
#include "ParticleSystem.h"
#include "tinyxml2.h"
#include "ParticleConfig.h"
#include <unordered_map>

 std::string convertPathFormatToUnixStyle(const std::string& path)
{
	std::string ret = path;
	int len = ret.length();
	for (int i = 0; i < len; ++i)
	{
		if (ret[i] == '\\')
		{
			ret[i] = '/';
		}
	}
	return ret;
}

inline void normalize_point(float x, float y, particle_point* out)
{
    float n = x * x + y * y;
    // Already normalized.
    if (n == 1.0f)
        return;

    n = sqrt(n);
    // Too close to zero.
    if (n < MATH_TOLERANCE)
        return;

    n = 1.0f / n;
    out->x = x * n;
    out->y = y * n;
}

/**
 A more effect random number getter function, get from ejoy2d.
 */
inline static float RANDOM_M11(unsigned int *seed) {
    *seed = *seed * 134775813 + 1;
    union {
        uint32_t d;
        float f;
    } u;
    u.d = (((uint32_t)(*seed) & 0x7fff) << 8) | 0x40000000;
    return u.f - 3.0f;
}

ParticleData::ParticleData()
{
    memset(this, 0, sizeof(ParticleData));
}

bool ParticleData::init(int count)
{
    if (count <= 0) return false;
    maxCount = (unsigned int)count;

    posx.resize(count);
    posy.resize(count);
    startPosX.resize(count);
    startPosY.resize(count);

    colorR.resize(count);
    colorG.resize(count);
    colorB.resize(count);
    colorA.resize(count);

    deltaColorR.resize(count);
    deltaColorG.resize(count);
    deltaColorB.resize(count);
    deltaColorA.resize(count);

    size.resize(count);
    deltaSize.resize(count);
    rotation.resize(count);
    deltaRotation.resize(count);
    timeToLive.resize(count);
    atlasIndex.resize(count);

    modeA.dirX.resize(count);
    modeA.dirY.resize(count);
    modeA.radialAccel.resize(count);
    modeA.tangentialAccel.resize(count);

    modeB.angle.resize(count);
    modeB.degreesPerSecond.resize(count);
    modeB.deltaRadius.resize(count);
    modeB.radius.resize(count);

    return true;
}

void ParticleData::release()
{
    posx.clear(); posy.clear();
    startPosX.clear(); startPosY.clear();
    colorR.clear(); colorG.clear(); colorB.clear(); colorA.clear();
    deltaColorR.clear(); deltaColorG.clear(); deltaColorB.clear(); deltaColorA.clear();
    size.clear(); deltaSize.clear();
    rotation.clear(); deltaRotation.clear();
    timeToLive.clear(); atlasIndex.clear();
    modeA.dirX.clear(); modeA.dirY.clear();
    modeA.radialAccel.clear(); modeA.tangentialAccel.clear();
    modeB.angle.clear(); modeB.degreesPerSecond.clear();
    modeB.deltaRadius.clear(); modeB.radius.clear();
    maxCount = 0;
}

//vector<ParticleSystem*> ParticleSystem::__allInstances;

float ParticleSystem::__totalParticleCountFactor = 1.0f;

ParticleSystem::ParticleSystem()
    : _isBlendAdditive(false)
    , _isAutoRemoveOnFinish(false)
    , _plistFile("")
    , _elapsed(0)
    , _configName("")
    , _emitCounter(0)
    //, _batchNode(nullptr)
    , _atlasIndex(0)
    , _transformSystemDirty(false)
    , _allocatedParticles(0)
    , _isActive(true)
    , _particleCount(0)
    , _duration(0)
    , _life(0)
    , _lifeVar(0)
    , _angle(0)
    , _angleVar(0)
    , _emitterMode(Mode::GRAVITY)
    , _startSize(0)
    , _startSizeVar(0)
    , _endSize(0)
    , _endSizeVar(0)
    , _startSpin(0)
    , _startSpinVar(0)
    , _endSpin(0)
    , _endSpinVar(0)
    , _emissionRate(0)
    , _totalParticles(0)
    , _texture(nullptr)
    , _blendFunc(BlendFunc::ADDITIVE)
    , _opacityModifyRGB(false)
    , _yCoordFlipped(1)
    , _positionType(PositionType::FREE)
    , _paused(false)
    , _sourcePositionCompatible(true) // In the furture this member's default value maybe false or be removed.
{
    _name = "ParticleSystem";
    modeA.gravity = Vec2::ZERO;
    modeA.speed = 0;
    modeA.speedVar = 0;
    modeA.tangentialAccel = 0;
    modeA.tangentialAccelVar = 0;
    modeA.radialAccel = 0;
    modeA.radialAccelVar = 0;
    modeA.rotationIsDir = false;
    modeB.startRadius = 0;
    modeB.startRadiusVar = 0;
    modeB.endRadius = 0;
    modeB.endRadiusVar = 0;
    modeB.rotatePerSecond = 0;
    modeB.rotatePerSecondVar = 0;

}
// implementation ParticleSystem

// ParticleSystem * ParticleSystem::create(const std::string& plistFile)
// {
//     ParticleSystem *ret = new (std::nothrow) ParticleSystem();
//     if (ret && ret->initWithFile(plistFile))
//     {
//       // // ret->autorelease();
//         return ret;
//     }
//   //  OG_SAFE_DELETE(ret);
//     return ret;
// }

ParticleSystem* ParticleSystem::createWithTotalParticles(int numberOfParticles)
{
    ParticleSystem *ret = new (std::nothrow) ParticleSystem();
    if (ret && ret->initWithTotalParticles(numberOfParticles))
    {
        //ret->autorelease();
        return ret;
    }
   // OG_SAFE_DELETE(ret);
    ret = nullptr;
    return ret;
}

// static
// vector<ParticleSystem*>& ParticleSystem::getAllParticleSystems()
// {
//     return __allInstances;
// }

void ParticleSystem::setTotalParticleCountFactor(float factor)
{
    __totalParticleCountFactor = factor;
}

bool ParticleSystem::init()
{
    return initWithTotalParticles(150);
}

static BlendFactor glFactorToSDL(int gl)
{
	switch (gl) {
	case 0x0000: return BlendFactor::ZERO;
	case 0x0001: return BlendFactor::ONE;
	case 0x0300: return BlendFactor::SRC_COLOR;
	case 0x0301: return BlendFactor::ONE_MINUS_SRC_COLOR;
	case 0x0302: return BlendFactor::SRC_ALPHA;
	case 0x0303: return BlendFactor::ONE_MINUS_SRC_ALPHA;
	case 0x0304: return BlendFactor::DST_ALPHA;
	case 0x0305: return BlendFactor::ONE_MINUS_DST_ALPHA;
	case 0x0306: return BlendFactor::DST_COLOR;
	case 0x0307: return BlendFactor::ONE_MINUS_DST_COLOR;
	default:     return BlendFactor::ONE;
	}
}
// 

// 
// static bool parsePlistDict(tinyxml2::XMLElement* dict,
// 	std::unordered_map<std::string, std::string>& out)
// {
// 	if (!dict) return false;
// 	for (auto* e = dict->FirstChildElement(); e; e = e->NextSiblingElement())
// 	{
// 		if (std::strcmp(e->Name(), "key") != 0) continue;
// 		const char* key = e->GetText();
// 		if (!key) continue;
// 		auto* val = e->NextSiblingElement();
// 		if (!val) break;
// 		const char* t = val->GetText();
// 		out[key] = t ? t : "";
// 	}
// 	return true;
// }
// bool  isAbsolutePath(const std::string& strPath) 
//{
//	if ((strPath.length() > 2
//		&& ((strPath[0] >= 'a' && strPath[0] <= 'z') || (strPath[0] >= 'A' && strPath[0] <= 'Z'))
//		&& strPath[1] == ':') || (strPath[0] == '/' && strPath[1] == '/'))
//	{
//		return true;
//	}
//	return false;
//}

// bool ParticleSystem::initWithParticlePlistData(const std::string  &plistFile)
// {
// 
// 	tinyxml2::XMLDocument doc;
// 	if (doc.Parse((const char*)plistFile.c_str()) != tinyxml2::XML_SUCCESS)
// 	{
// 		return false;
// 	}
// 
// 
// 	auto* plist = doc.FirstChildElement("plist");
// 	if (!plist) return false;
// 	auto* dict = plist->FirstChildElement("dict");
// 	if (!dict) return false;
// 
// 	std::unordered_map<std::string, std::string> kv;
// 	if (!parsePlistDict(dict, kv))
// 		return false;
// 
// 	auto F = [&](const char* k, float def = 0.0f) -> float {
// 		auto it = kv.find(k);
// 		return it == kv.end() ? def : (float)std::atof(it->second.c_str());
// 	};
// 	auto I = [&](const char* k, int def = 0) -> int {
// 		auto it = kv.find(k);
// 		return it == kv.end() ? def : std::atoi(it->second.c_str());
// 	};
// 	auto S = [&](const char* k, const char* def = "") -> std::string {
// 		auto it = kv.find(k);
// 		return it == kv.end() ? std::string(def) : it->second;
// 	};
// 
// 	// ---- 直接赋值给 ParticleSystem 自己的成员 ----
// 	_totalParticles = I("maxParticles", 150);
// 
// 	if (!this->initWithTotalParticles(_totalParticles))
// 	{
// 		return false;
// 	}
// 
// 	_angle = F("angle", 90.0f);
// 	_angleVar = F("angleVariance", 0.0f);
// 	int blendFuncSource = I("blendFuncSource", 770);
// 	int blendFuncDestination = I("blendFuncDestination", 771);
// 	bool additive = (blendFuncSource == 770 && blendFuncDestination == 1);
// 	setBlendAdditive(additive);
// 
// 	_duration = F("duration", -1.0f);
// 	_emitterMode = (Mode)I("emitterType", 0);
// 	_emissionRate = F("emissionRate", 10.0f);
// 
// 	_startColor.r = F("startColorRed", 1.0f);
// 	_startColor.g = F("startColorGreen", 1.0f);
// 	_startColor.b = F("startColorBlue", 1.0f);
// 	_startColor.a = F("startColorAlpha", 1.0f);
// 	_startColorVar.r = F("startColorVarianceRed", 0.0f);
// 	_startColorVar.g = F("startColorVarianceGreen", 0.0f);
// 	_startColorVar.b = F("startColorVarianceBlue", 0.0f);
// 	_startColorVar.a = F("startColorVarianceAlpha", 0.0f);
// 
// 	_endColor.r = F("finishColorRed", 1.0f);
// 	_endColor.g = F("finishColorGreen", 1.0f);
// 	_endColor.b = F("finishColorBlue", 1.0f);
// 	_endColor.a = F("finishColorAlpha", 1.0f);
// 	_endColorVar.r = F("finishColorVarianceRed", 0.0f);
// 	_endColorVar.g = F("finishColorVarianceGreen", 0.0f);
// 	_endColorVar.b = F("finishColorVarianceBlue", 0.0f);
// 	_endColorVar.a = F("finishColorVarianceAlpha", 0.0f);
// 
// 	_startSize = F("startParticleSize", 10.0f);
// 	_startSizeVar = F("startParticleSizeVariance", 0.0f);
// 	_endSize = F("finishParticleSize", -1.0f);
// 	_endSizeVar = F("finishParticleSizeVariance", 0.0f);
// 
// 	_startSpin = F("rotationStart", 0.0f);
// 	_startSpinVar = F("rotationStartVariance", 0.0f);
// 	_endSpin = F("rotationEnd", 0.0f);
// 	_endSpinVar = F("rotationEndVariance", 0.0f);
// 
// 	_life = F("particleLifespan", 1.0f);
// 	_lifeVar = F("particleLifespanVariance", 0.0f);
// 
// 	_sourcePosition.x = F("sourcePositionx", 0.0f);
// 	_sourcePosition.y = F("sourcePositiony", 0.0f);
// 	_posVar.x = F("sourcePositionVariancex", 0.0f);
// 	_posVar.y = F("sourcePositionVariancey", 0.0f);
// 
// 	// ★ 注意：plist 里存的是 -gravity，读回来要取反
// 	modeA.gravity.x = -F("gravityx", 0.0f);
// 	modeA.gravity.y = -F("gravityy", 0.0f);
// 	modeA.speed = -F("speed", 0.0f);
// 	modeA.speedVar = F("speedVariance", 0.0f);
// 	modeA.radialAccel = F("radialAcceleration", 0.0f);
// 	modeA.radialAccelVar = F("radialAccelVariance", 0.0f);
// 	modeA.tangentialAccel = F("tangentialAcceleration", 0.0f);
// 	modeA.tangentialAccelVar = F("tangentialAccelVariance", 0.0f);
// 
// 	modeB.startRadius = F("maxRadius", 0.0f);
// 	modeB.startRadiusVar = F("maxRadiusVariance", 0.0f);
// 	modeB.endRadius = F("minRadius", 0.0f);
// 	modeB.endRadiusVar = F("minRadiusVariance", 0.0f);
// 	modeB.rotatePerSecond = F("rotatePerSecond", 0.0f);
// 	modeB.rotatePerSecondVar = F("rotatePerSecondVariance", 0.0f);
// 
// 	std::string _textureFileName = S("textureFileName", "fire.png");
// 
// 	Texture2D * tex = nullptr;
// 	do {
// 		if (!_textureFileName.empty())
// 		{
// 			if (!isAbsolutePath(_textureFileName))//如果是绝对路径
// 			{
// 				std::string PathFile = convertPathFormatToUnixStyle(_plistFile);
// 				size_t rPos = PathFile.rfind('/'); //查找路径
// 				std::string textureDir = "";
// 				if (rPos != std::string::npos)//如果上层只是文件名，无需拼接
// 				{
// 					textureDir = PathFile.substr(0, rPos + 1);//截止到目录 
// 					_textureFileName = textureDir + _textureFileName;
// 				}
// 
// 
// 			}//如果没找到则调用
// 			FILE *fp = fopen(_textureFileName.c_str(), "rb");
// 			if (fp == NULL) {
// 				// 文件不存在或无法打开，处理错误
// 				break;
// 			}
// 			fseek(fp, 0, SEEK_END);
// 			long fileSize = ftell(fp);
// 			fseek(fp, 0, SEEK_SET); // 回到文件开头
// 
// 			// 2. 分配足够的内存
// 			std::vector<unsigned char> buffer(fileSize);
// 
// 			// 3. 一次性读取整个文件
// 			size_t bytesRead = fread(buffer.data(), 1, fileSize, fp);
// 			fclose(fp);
// 
// 			// 检查是否读满（文件可能被截断或读取失败）
// 			if (bytesRead != static_cast<size_t>(fileSize)) {
// 				// 处理读取不完整的情况，这里简单返回空
// 				return {};
// 			}
// 			tex = new Texture2D;
// 			tex->loadMemData(rener, buffer.data(), buffer.size());
// 
// 			tex->setMultipliedAlpha(true);
// 			setTexture(tex);
// 
// 
// 		}
// 	} while (0);
// 	_yCoordFlipped = I("yCoordFlipped", 1);
// 
// 	// ---- 内嵌图片数据（可选）----
// // 	auto itData = kv.find("textureImageData");
// // 	if (itData != kv.end() && !itData->second.empty())
// // 		_embeddedTextureData = itData->second;
// 
// 	return true;
// 
// }
bool ParticleSystem::initWithTotalParticles(int numberOfParticles)
{
    _totalParticles = numberOfParticles;

    //  _particleData.release();

    if (!_particleData.init(_totalParticles))
    {
        //         CCLOG("Particle system: not enough memory");
        //         this->release();
                //return false;
    }
    _allocatedParticles = numberOfParticles;
   
    //     if (_batchNode)
    //     {
    //         for (int i = 0; i < _totalParticles; i++)
    //         {
    //             _particleData.atlasIndex[i] = i;
    //         }
    //     }
        // default, active
    _isActive = true;

    // default blend function
    _blendFunc = BlendFunc::ADDITIVE;
    //_blendFunc = BlendFunc::DISABLE;
    // default movement type;
    _positionType = PositionType::FREE;

    // by default be in mode A:
    _emitterMode = Mode::GRAVITY;

    // default: modulate
    // FIXME:: not used
    //    colorModulate = YES;

    _isAutoRemoveOnFinish = false;

    // Optimization: compile updateParticle method
    //updateParticleSel = @selector(updateQuadWithParticle:newPosition:);
    //updateParticleImp = (OG_UPDATE_PARTICLE_IMP) [self methodForSelector:updateParticleSel];
    //for batchNode
    _transformSystemDirty = false;

	_quads.resize(_totalParticles);
	_indices.resize(_totalParticles * 6);

	//防止加载中途list崩_particleCount渲染会越界
	_particleCount = 0;
	_emitCounter = 0.0f;
	_elapsed = 0.0f;

    return true;
}

ParticleSystem::~ParticleSystem()
{
    // Since the scheduler retains the "target (in this case the ParticleSystem)
    // it is not needed to call "unscheduleUpdate" here. In fact, it will be called in "cleanup"
   // setNotUpdate();
    _particleData.release(); 
    if (_texture)
    {
        delete _texture;
    }
}

void ParticleSystem::addParticles(int count)
{
    if (_paused)
        return;
    uint32_t RANDSEED = rand();

    int start = _particleCount;
    _particleCount += count;

    //life
    for (int i = start; i < _particleCount; ++i)
    {
        float theLife = _life + _lifeVar * RANDOM_M11(&RANDSEED);
        _particleData.timeToLive[i] = (float)fmax(0, theLife);
    }

    //position
    for (int i = start; i < _particleCount; ++i)
    {
        _particleData.posx[i] = _sourcePosition.x + _posVar.x * RANDOM_M11(&RANDSEED);
    }

    for (int i = start; i < _particleCount; ++i)
    {
        _particleData.posy[i] = _sourcePosition.y + _posVar.y * RANDOM_M11(&RANDSEED);
    }

    //color
#define SET_COLOR(c, b, v)\
for (int i = start; i < _particleCount; ++i)\
{\
c[i] = clampf( b + v * RANDOM_M11(&RANDSEED) , 0 , 1 );\
}

    SET_COLOR(_particleData.colorR, _startColor.r, _startColorVar.r);
    SET_COLOR(_particleData.colorG, _startColor.g, _startColorVar.g);
    SET_COLOR(_particleData.colorB, _startColor.b, _startColorVar.b);
    SET_COLOR(_particleData.colorA, _startColor.a, _startColorVar.a);

    SET_COLOR(_particleData.deltaColorR, _endColor.r, _endColorVar.r);
    SET_COLOR(_particleData.deltaColorG, _endColor.g, _endColorVar.g);
    SET_COLOR(_particleData.deltaColorB, _endColor.b, _endColorVar.b);
    SET_COLOR(_particleData.deltaColorA, _endColor.a, _endColorVar.a);

#define SET_DELTA_COLOR(c, dc)\
for (int i = start; i < _particleCount; ++i)\
{\
dc[i] = (dc[i] - c[i]) / _particleData.timeToLive[i];\
}

    SET_DELTA_COLOR(_particleData.colorR, _particleData.deltaColorR);
    SET_DELTA_COLOR(_particleData.colorG, _particleData.deltaColorG);
    SET_DELTA_COLOR(_particleData.colorB, _particleData.deltaColorB);
    SET_DELTA_COLOR(_particleData.colorA, _particleData.deltaColorA);

    //size
    for (int i = start; i < _particleCount; ++i)
    {
        _particleData.size[i] = _startSize + _startSizeVar * RANDOM_M11(&RANDSEED);
        _particleData.size[i] = (float)fmax(0, _particleData.size[i]);
    }

    if (_endSize != START_SIZE_EQUAL_TO_END_SIZE)
    {
        for (int i = start; i < _particleCount; ++i)
        {
            float endSize = _endSize + _endSizeVar * RANDOM_M11(&RANDSEED);
            endSize = (float)fmax(0, endSize);
            _particleData.deltaSize[i] = (endSize - _particleData.size[i]) / _particleData.timeToLive[i];
        }
    }
    else
    {
        for (int i = start; i < _particleCount; ++i)
        {
            _particleData.deltaSize[i] = 0.0f;
        }
    }

    // rotation
    for (int i = start; i < _particleCount; ++i)
    {
        _particleData.rotation[i] = _startSpin + _startSpinVar * RANDOM_M11(&RANDSEED);
    }
    for (int i = start; i < _particleCount; ++i)
    {
        float endA = _endSpin + _endSpinVar * RANDOM_M11(&RANDSEED);
        _particleData.deltaRotation[i] = (endA - _particleData.rotation[i]) / _particleData.timeToLive[i];
    }

    // position添加一处，初始化的位置
    Vec2 pos;

    if (_positionType == PositionType::FREE)
    {
        pos = this->convertToWorldSpace(Vec2::ZERO);
    }
    else if (_positionType == PositionType::RELATIVE)
    {
        pos = _position;
    }
    for (int i = start; i < _particleCount; ++i)
    {
        _particleData.startPosX[i] = pos.x;
    }
    for (int i = start; i < _particleCount; ++i)
    {
        _particleData.startPosY[i] = pos.y;
    }

    // Mode Gravity: A
    if (_emitterMode == Mode::GRAVITY)
    {

        // radial accel
        for (int i = start; i < _particleCount; ++i)
        {
            _particleData.modeA.radialAccel[i] = modeA.radialAccel + modeA.radialAccelVar * RANDOM_M11(&RANDSEED);
        }

        // tangential accel
        for (int i = start; i < _particleCount; ++i)
        {
            _particleData.modeA.tangentialAccel[i] = modeA.tangentialAccel + modeA.tangentialAccelVar * RANDOM_M11(&RANDSEED);
        }

        // rotation is dir
        if (modeA.rotationIsDir)
        {
            for (int i = start; i < _particleCount; ++i)
            {
                float a = OG_DEGREES_TO_RADIANS(_angle + _angleVar * RANDOM_M11(&RANDSEED));
                Vec2 v(cosf(a), sinf(a));
                float s = modeA.speed + modeA.speedVar * RANDOM_M11(&RANDSEED);
                Vec2 dir = v * s;
                _particleData.modeA.dirX[i] = dir.x;//v * s ;
                _particleData.modeA.dirY[i] = dir.y;
                _particleData.rotation[i] = -((dir.getAngle()) * 57.29577951f);
            }
        }
        else
        {
            for (int i = start; i < _particleCount; ++i)
            {
                float a = OG_DEGREES_TO_RADIANS(_angle + _angleVar * RANDOM_M11(&RANDSEED));
                Vec2 v(cosf(a), sinf(a));
                float s = modeA.speed + modeA.speedVar * RANDOM_M11(&RANDSEED);
                Vec2 dir = v * s;
                _particleData.modeA.dirX[i] = dir.x;//v * s ;
                _particleData.modeA.dirY[i] = dir.y;
            }
        }

    }

    // Mode Radius: B
    else
    {
        //Need to check by Jacky
        // Set the default diameter of the particle from the source position
        for (int i = start; i < _particleCount; ++i)
        {
            _particleData.modeB.radius[i] = modeB.startRadius + modeB.startRadiusVar * RANDOM_M11(&RANDSEED);
        }

        for (int i = start; i < _particleCount; ++i)
        {
            _particleData.modeB.angle[i] = OG_DEGREES_TO_RADIANS(_angle + _angleVar * RANDOM_M11(&RANDSEED));
        }

        for (int i = start; i < _particleCount; ++i)
        {
            _particleData.modeB.degreesPerSecond[i] = OG_DEGREES_TO_RADIANS(modeB.rotatePerSecond + modeB.rotatePerSecondVar * RANDOM_M11(&RANDSEED));
        }

        if (modeB.endRadius == START_RADIUS_EQUAL_TO_END_RADIUS)
        {
            for (int i = start; i < _particleCount; ++i)
            {
                _particleData.modeB.deltaRadius[i] = 0.0f;
            }
        }
        else
        {
            for (int i = start; i < _particleCount; ++i)
            {
                float endRadius = modeB.endRadius + modeB.endRadiusVar * RANDOM_M11(&RANDSEED);
                _particleData.modeB.deltaRadius[i] = (endRadius - _particleData.modeB.radius[i]) / _particleData.timeToLive[i];
            }
        }
    }
}
// 
void ParticleSystem::onEnter()
{
#if OG_ENABLE_SCRIPT_BINDING
    if (_scriptType == kScriptTypeJavascript)
    {
        if (ScriptEngineManager::sendNodeEventToJSExtended(this, kNodeOnEnter))
            return;
    }
#endif

    //Node::onEnter();

    //// update after action in run!
    //this->setUpdate();

    // __allInstances.pushBack(this);
}
// 
void ParticleSystem::onExit()
{
#if OG_ENABLE_SCRIPT_BINDING
    if (_scriptType == kScriptTypeJavascript)
    {
        if (ScriptEngineManager::sendNodeEventToJSExtended(this, kNodeOnExit))
            return;
    }
#endif
    /*
        this->setNotUpdate();
        Node::onExit();*/

    //     auto iter = std::find(__allInstances.begin(),  __allInstances.end(), this);
    //     if (iter != __allInstances.end())
    //     {
    //         __allInstances.erase(iter);
    //     }
}

void ParticleSystem::stopSystem()
{
    _isActive = false;
    _elapsed = _duration;
    _emitCounter = 0;
}

void ParticleSystem::resetSystem()
{
	//防止加载plist中途 卡
	_emitCounter = 0.0f;      // ← 加上
	_particleCount = 0;       // ← 加上，关键
    _isActive = true;
    _elapsed = 0;
    for (int i = 0; i < _particleCount; ++i)
    {
        _particleData.timeToLive[i] = 0.0f;
    }
}

bool ParticleSystem::isFull()
{
    return (_particleCount == _totalParticles);
}



void ParticleSystem::updateWithNoTime()
{
    this->update(0.0f);
}




void ParticleSystem::postStep()
{
    // should be overridden
}

// ParticleSystem - Texture protocol
void ParticleSystem::setTexture(Texture2D* var)
{
    if (_texture != var)
    {
        delete _texture;
        _texture = var;  
      //  _center = _contentSize / 2;
   
        _clip = Rect::ZERO;
        _clip= _texture->getImageSize();
        updateBlendFunc();
    }
}

//  
void ParticleSystem::updateBlendFunc()
{
    //  //CCASSERT(! _batchNode, "Can't change blending functions when the particle is being batched");

    if (_texture == nullptr) return;

    // 统一预乘
    _opacityModifyRGB = true;
    if (_additive)
    {
        _blendFunc = BlendFunc::ADDITIVE;
    }
    else
    {
        //  if( _texture && ! _texture->hasPremultipliedAlpha() )
        _blendFunc = BlendFunc::ALPHA_NON_PREMULTIPLIED;
        //         else 
        //             _blendFunc = BlendFunc::ALPHA_PREMULTIPLIED;
    }
}

Texture2D * ParticleSystem::getTexture() const
{
    return _texture;
}

// ParticleSystem - Additive Blending
void ParticleSystem::setBlendAdditive(bool additive)
{
    if (additive)
    {
        _blendFunc = BlendFunc::ADDITIVE;
    }
    else
    {
        //  if( _texture && ! _texture->hasPremultipliedAlpha() )
        _blendFunc = BlendFunc::ALPHA_NON_PREMULTIPLIED;
        //         else 
        //             _blendFunc = BlendFunc::ALPHA_PREMULTIPLIED;
    }
    _additive = additive;
}



bool ParticleSystem::isBlendAdditive() const
{
    return   (_blendFunc.src ==  BlendFactor::SRC_ALPHA && _blendFunc.dst ==  BlendFactor::ONE);
}

// ParticleSystem - Properties of Gravity Mode 
void ParticleSystem::setTangentialAccel(float t)
{
    //  //CCASSERT( _emitterMode == Mode::GRAVITY, "Particle Mode should be Gravity");
    modeA.tangentialAccel = t;
}

float ParticleSystem::getTangentialAccel() const
{
    ////CCASSERT( _emitterMode == Mode::GRAVITY, "Particle Mode should be Gravity");
    return modeA.tangentialAccel;
}

void ParticleSystem::setTangentialAccelVar(float t)
{
    // //CCASSERT(_emitterMode == Mode::GRAVITY, "Particle Mode should be Gravity");
    modeA.tangentialAccelVar = t;
}

float ParticleSystem::getTangentialAccelVar() const
{
    // //CCASSERT(_emitterMode == Mode::GRAVITY, "Particle Mode should be Gravity");
    return modeA.tangentialAccelVar;
}

void ParticleSystem::setRadialAccel(float t)
{
    // //CCASSERT(_emitterMode == Mode::GRAVITY, "Particle Mode should be Gravity");
    modeA.radialAccel = t;
}

float ParticleSystem::getRadialAccel() const
{
    ////CCASSERT(_emitterMode == Mode::GRAVITY, "Particle Mode should be Gravity");
    return modeA.radialAccel;
}

void ParticleSystem::setRadialAccelVar(float t)
{
    // //CCASSERT(_emitterMode == Mode::GRAVITY, "Particle Mode should be Gravity");
    modeA.radialAccelVar = t;
}

float ParticleSystem::getRadialAccelVar() const
{
    //  //CCASSERT(_emitterMode == Mode::GRAVITY, "Particle Mode should be Gravity");
    return modeA.radialAccelVar;
}

void ParticleSystem::setRotationIsDir(bool t)
{
    // //CCASSERT(_emitterMode == Mode::GRAVITY, "Particle Mode should be Gravity");
    modeA.rotationIsDir = t;
}

bool ParticleSystem::getRotationIsDir() const
{
    //  //CCASSERT(_emitterMode == Mode::GRAVITY, "Particle Mode should be Gravity");
    return modeA.rotationIsDir;
}

void ParticleSystem::setGravity(const Vec2& g)
{
    // //CCASSERT(_emitterMode == Mode::GRAVITY, "Particle Mode should be Gravity");
    modeA.gravity = g;
}

const Vec2& ParticleSystem::getGravity()
{
    //  //CCASSERT(_emitterMode == Mode::GRAVITY, "Particle Mode should be Gravity");
    return modeA.gravity;
}

void ParticleSystem::setSpeed(float speed)
{
    //  //CCASSERT(_emitterMode == Mode::GRAVITY, "Particle Mode should be Gravity");
    modeA.speed = speed;
}

float ParticleSystem::getSpeed() const
{
    // //CCASSERT(_emitterMode == Mode::GRAVITY, "Particle Mode should be Gravity");
    return modeA.speed;
}

void ParticleSystem::setSpeedVar(float speedVar)
{
    //  //CCASSERT(_emitterMode == Mode::GRAVITY, "Particle Mode should be Gravity");
    modeA.speedVar = speedVar;
}

float ParticleSystem::getSpeedVar() const
{
    //  //CCASSERT(_emitterMode == Mode::GRAVITY, "Particle Mode should be Gravity");
    return modeA.speedVar;
}

// ParticleSystem - Properties of Radius Mode
void ParticleSystem::setStartRadius(float startRadius)
{
    //  //CCASSERT(_emitterMode == Mode::RADIUS, "Particle Mode should be Radius");
    modeB.startRadius = startRadius;
}

float ParticleSystem::getStartRadius() const
{
    //  //CCASSERT(_emitterMode == Mode::RADIUS, "Particle Mode should be Radius");
    return modeB.startRadius;
}

void ParticleSystem::setStartRadiusVar(float startRadiusVar)
{
    //  //CCASSERT(_emitterMode == Mode::RADIUS, "Particle Mode should be Radius");
    modeB.startRadiusVar = startRadiusVar;
}

float ParticleSystem::getStartRadiusVar() const
{
    //  //CCASSERT(_emitterMode == Mode::RADIUS, "Particle Mode should be Radius");
    return modeB.startRadiusVar;
}

void ParticleSystem::setEndRadius(float endRadius)
{
    ////   //CCASSERT(_emitterMode == Mode::RADIUS, "Particle Mode should be Radius");
    modeB.endRadius = endRadius;
}

float ParticleSystem::getEndRadius() const
{
    //  //CCASSERT(_emitterMode == Mode::RADIUS, "Particle Mode should be Radius");
    return modeB.endRadius;
}

void ParticleSystem::setEndRadiusVar(float endRadiusVar)
{
    //  //CCASSERT(_emitterMode == Mode::RADIUS, "Particle Mode should be Radius");
    modeB.endRadiusVar = endRadiusVar;
}

float ParticleSystem::getEndRadiusVar() const
{
    //   //CCASSERT(_emitterMode == Mode::RADIUS, "Particle Mode should be Radius");
    return modeB.endRadiusVar;
}

void ParticleSystem::setRotatePerSecond(float degrees)
{
    //   //CCASSERT(_emitterMode == Mode::RADIUS, "Particle Mode should be Radius");
    modeB.rotatePerSecond = degrees;
}

float ParticleSystem::getRotatePerSecond() const
{
    //   //CCASSERT(_emitterMode == Mode::RADIUS, "Particle Mode should be Radius");
    return modeB.rotatePerSecond;
}

void ParticleSystem::setRotatePerSecondVar(float degrees)
{
    //   //CCASSERT(_emitterMode == Mode::RADIUS, "Particle Mode should be Radius");
    modeB.rotatePerSecondVar = degrees;
}

float ParticleSystem::getRotatePerSecondVar() const
{
    //   //CCASSERT(_emitterMode == Mode::RADIUS, "Particle Mode should be Radius");
    return modeB.rotatePerSecondVar;
}

bool ParticleSystem::isActive() const
{
    return _isActive;
}

int ParticleSystem::getTotalParticles() const
{
    return _totalParticles;
}

void ParticleSystem::setTotalParticles(int var)
{
    //   //CCASSERT( var <= _allocatedParticles, "Particle: resizing particle array only supported for quads");
    //_totalParticles = var;
   // if (var > _allocatedParticles)
    {
        //_particleData.release();
        if (!_particleData.init(var))
        {
           // CCLOG("Particle system: not enough memory");
            return;
        }
        _totalParticles = var;
        _particleCount = 0;
        _quads.resize(_totalParticles);
        _indices.resize(_totalParticles * 6);

        setEmissionRate(_totalParticles / _life);
    }

}
void ParticleSystem::updateTexCoords()
{
    if (_texture)
    {
        const Size& s = _texture->getImageSize();
        initTexCoordsWithRect(Rect(0, 0, s.w, s.h));
    }
}

void ParticleSystem::initTexCoordsWithRect(const Rect& pointRect)
{
    float uvX = _clip.x / pointRect.w;
    float uvY = _clip.y / pointRect.h;
    float uvW = (_clip.x + _clip.w) / pointRect.w;
    float uvH = (_clip.y + _clip.h) / pointRect.h;

    // ===== 遍历 _quads，直接变换 + 填 UV =====
    int vertCount = 0;

    for (int i = 0; i < _particleCount; ++i)
    { 
        // UV（每个 quad 都一样）
        _quads[i].tl.texCoords = { uvX, uvY };
        _quads[i].tr.texCoords = { uvW, uvY };
        _quads[i].bl.texCoords = { uvX, uvH };
        _quads[i].br.texCoords = { uvW, uvH };
 
        // 索引
        int vC = vertCount * 4;
        int iC = vertCount * 6;
        _indices[iC + 0] = vC + 0;
        _indices[iC + 1] = vC + 1;
        _indices[iC + 2] = vC + 2;
        _indices[iC + 3] = vC + 1;
        _indices[iC + 4] = vC + 3;
        _indices[iC + 5] = vC + 2;
         
    }
}
void  ParticleSystem::initIndices()
{
    for (int i = 0; i < _totalParticles; ++i)
    {
        const unsigned int i6 = i * 6;
        const unsigned int i4 = i * 4;
        _indices[i6 + 0] = (unsigned short)i4 + 0;
        _indices[i6 + 1] = (unsigned short)i4 + 1;
        _indices[i6 + 2] = (unsigned short)i4 + 2;

        _indices[i6 + 5] = (unsigned short)i4 + 1;
        _indices[i6 + 4] = (unsigned short)i4 + 2;
        _indices[i6 + 3] = (unsigned short)i4 + 3;
    }
}
const BlendFunc& ParticleSystem::getBlendFunc() const
{
    return _blendFunc;
}

 void ParticleSystem::setBlendFunc(const BlendFunc &blendFunc)
 {
     if( _blendFunc.src != blendFunc.src || _blendFunc.dst != blendFunc.dst ) {
         _blendFunc = blendFunc;
         this->updateBlendFunc();
     }
 }

bool ParticleSystem::isAutoRemoveOnFinish() const
{
    return _isAutoRemoveOnFinish;
}

void ParticleSystem::setAutoRemoveOnFinish(bool var)
{
    _isAutoRemoveOnFinish = var;
}


// ParticleSystem - methods for batchNode rendering

// ParticleBatchNode* ParticleSystem::getBatchNode() const
// {
//     return _batchNode;
// }

// void ParticleSystem::setBatchNode(ParticleBatchNode* batchNode)
// {
//     if( _batchNode != batchNode ) {
// 
//         _batchNode = batchNode; // weak reference
// 
//         if( batchNode ) {
//             //each particle needs a unique index
//             for (int i = 0; i < _totalParticles; i++)
//             {
//                 _particleData.atlasIndex[i] = i;
//             }
//         }
//     }
// }
void ParticleSystem::setPosition(const Vec2& v)
{
    _position = v;
    _transformDirty = true;
}
void ParticleSystem::setSkewX(const float& skx) {
    _skewX = skx;
    _transformDirty = true;
}
void ParticleSystem::setSkewY(const float& sky) {
    _skewY = sky;
    _transformDirty = true;
}
//don't use a transform matrix, this is faster
void ParticleSystem::setScale(float s)
{
    _transformSystemDirty = true;
    _scaleX =_scaleY = s;
    _transformDirty = true;
}

void ParticleSystem::setRotation(float newRotation)
{
    _transformSystemDirty = true;
    _rotation = newRotation;
    _transformDirty = true;
}

void ParticleSystem::setScaleX(float newScaleX)
{
    _transformSystemDirty = true;
    _scaleX = _scaleY = newScaleX;
    _transformDirty = true;
}

void ParticleSystem::setScaleY(float newScaleY)
{
    _transformSystemDirty = true;
    _scaleY = newScaleY;
    _transformDirty = true;
}

void ParticleSystem::start()
{
    resetSystem();
}

void ParticleSystem::stop()
{
    stopSystem();
}

bool ParticleSystem::isPaused() const
{
    return _paused;
}

void ParticleSystem::pauseEmissions()
{
    _paused = true;
}

void ParticleSystem::resumeEmissions()
{
    _paused = false;
}


inline void updatePosWithParticle(V2F_C4B_T2F_Quad *quad, const Vec2& newPosition, float size, float rotation)
{
    // vertices
    float size_2 = size / 2;
    float x1 = -size_2;
    float y1 = -size_2;

    float x2 = size_2;
    float y2 = size_2;
    float x = newPosition.x;
    float y = newPosition.y;

    float r = (float)-(rotation * 0.01745329252f);
    float cr = cosf(r);
    float sr = sinf(r);
    float ax = x1 * cr - y1 * sr + x;
    float ay = x1 * sr + y1 * cr + y;
    float bx = x2 * cr - y1 * sr + x;
    float by = x2 * sr + y1 * cr + y;
    float cx = x2 * cr - y2 * sr + x;
    float cy = x2 * sr + y2 * cr + y;
    float dx = x1 * cr - y2 * sr + x;
    float dy = x1 * sr + y2 * cr + y;


    //原Y 轴向上（OpenGL 风格）颠倒了，需要改回来

    // top-left vertex:
    quad->tl.vertices.x = ax;
    quad->tl.vertices.y = ay;

    // top-right vertex:
    quad->tr.vertices.x = bx;
    quad->tr.vertices.y = by;

    // bottom-left
    quad->bl.vertices.x = dx;
    quad->bl.vertices.y = dy;

    // bottom-right vertex:
    quad->br.vertices.x = cx;
    quad->br.vertices.y = cy;
}

void ParticleSystem::updateParticleQuads()
{
    if (_particleCount <= 0) {
        return;
    }

    Vec2 currentPosition;
    if (_positionType == PositionType::FREE)
    {
        currentPosition = this->convertToWorldSpace(Vec2::ZERO);
   
    }
    else if (_positionType == PositionType::RELATIVE)
    {
        currentPosition = _position;
    }

    V2F_C4B_T2F_Quad *startQuad;
    Vec2 pos = Vec2::ZERO;
    // 	if (_batchNode)
    // 	{
    // 		V3F_C4B_T2F_Quad *batchQuads = _batchNode->getTextureAtlas()->getQuads();
    // 		startQuad = &(batchQuads[_atlasIndex]);
    // 		pos = _position;
    // 	}
    // 	else
    {
        startQuad = &(_quads[0]);
    }
    // 取引用，写起来短，性能等价
    auto& pd = _particleData;

    if (_positionType == PositionType::FREE)
    {
        Vec2 p1(currentPosition.x, currentPosition.y);
        Mat3 worldToNodeTM = getWorldToNodeTransform();
        worldToNodeTM.out(&p1);
        Vec2 p2;
        Vec2 newPos;
        // 		float* startX = _particleData.startPosX;
        // 		float* startY = _particleData.startPosY;
        // 		float* x = _particleData.posx;
        // 		float* y = _particleData.posy;
        // 		float* s = _particleData.size;
        // 		float* r = _particleData.rotation;
        V2F_C4B_T2F_Quad* quadStart = startQuad;
        for (int i = 0; i < _particleCount; ++i, ++quadStart)
        {
            p2.set(pd.startPosX[i], pd.startPosY[i]);
            worldToNodeTM.out(&p2);
            newPos.set(pd.posx[i], pd.posy[i]);
            p2 = p1 - p2;
            newPos.x -= p2.x - pos.x;
            newPos.y -= p2.y - pos.y;
            updatePosWithParticle(quadStart, newPos, pd.size[i], pd.rotation[i]);
        }
    }
    else if (_positionType == PositionType::RELATIVE)
    {
        Vec2 newPos;
        // 		float* startX = _particleData.startPosX;
        // 		float* startY = _particleData.startPosY;
        // 		float* x = _particleData.posx;
        // 		float* y = _particleData.posy;
        // 		float* s = _particleData.size;
        // 		float* r = _particleData.rotation;
        V2F_C4B_T2F_Quad* quadStart = startQuad;
        for (int i = 0; i < _particleCount; ++i, ++quadStart)
        {
            newPos.set(pd.posx[i], pd.posy[i]);
            newPos.x = pd.posx[i] - (currentPosition.x - pd.startPosX[i]);
            newPos.y = pd.posy[i] - (currentPosition.y - pd.startPosY[i]);
            newPos += pos;
            updatePosWithParticle(quadStart, newPos, pd.size[i], pd.rotation[i]);
        }
    }
    else
    {
        Vec2 newPos;
        // 		float* startX = _particleData.startPosX;
        // 		float* startY = _particleData.startPosY;
        // 		float* x = _particleData.posx;
        // 		float* y = _particleData.posy;
        // 		float* s = _particleData.size;
        // 		float* r = _particleData.rotation;
        V2F_C4B_T2F_Quad* quadStart = startQuad;
        for (int i = 0; i < _particleCount; ++i, ++quadStart)
        {
            newPos.set(pd.posx[i] + pos.x, pd.posy[i] + pos.y);
            updatePosWithParticle(quadStart, newPos, pd.size[i], pd.rotation[i]);
        }
    }

    //set color
    if (_opacityModifyRGB)
    {
        V2F_C4B_T2F_Quad* quad = startQuad;
        // 		float* r = _particleData.colorR;
        // 		float* g = _particleData.colorG;
        // 		float* b = _particleData.colorB;
        // 		float* a = _particleData.colorA;

        for (int i = 0; i < _particleCount; ++i, ++quad)
        {
            uint8_t colorR = (uint8_t)(pd.colorR[i] * pd.colorA[i] * 255);
            uint8_t colorG = (uint8_t)(pd.colorG[i] * pd.colorA[i] * 255);
            uint8_t colorB = (uint8_t)(pd.colorB[i] * pd.colorA[i] * 255);
            uint8_t colorA = (uint8_t)(pd.colorA[i] * 255);
            quad->bl.colors.set(colorR, colorG, colorB, colorA);
            quad->br.colors.set(colorR, colorG, colorB, colorA);
            quad->tl.colors.set(colorR, colorG, colorB, colorA);
            quad->tr.colors.set(colorR, colorG, colorB, colorA);
        }
    }
    else
    {
        V2F_C4B_T2F_Quad* quad = startQuad;
        // 		float* r = _particleData.colorR;
        // 		float* g = _particleData.colorG;
        // 		float* b = _particleData.colorB;
        // 		float* a = _particleData.colorA;

        for (int i = 0; i < _particleCount; ++i, ++quad)
        {
            uint8_t colorR = (uint8_t)(pd.colorR[i] * 255);
            uint8_t colorG = (uint8_t)(pd.colorG[i] * 255);
            uint8_t colorB = (uint8_t)(pd.colorB[i] * 255);
            uint8_t colorA = (uint8_t)(pd.colorA[i] * 255);
            quad->bl.colors.set(colorR, colorG, colorB, colorA);
            quad->br.colors.set(colorR, colorG, colorB, colorA);
            quad->tl.colors.set(colorR, colorG, colorB, colorA);
            quad->tr.colors.set(colorR, colorG, colorB, colorA);
        }
    }
}
// ParticleSystem - MainLoop
void ParticleSystem::update(float dt)
{
    //    OG_PROFILER_START_CATEGORY(kProfilerCategoryParticles , "CCParticleSystem - update");

    if (_isActive && _emissionRate)
    {
        float rate = 1.0f / _emissionRate;
        int totalParticles = static_cast<int>(_totalParticles * __totalParticleCountFactor);

        //issue #1201, prevent bursts of particles, due to too high emitCounter
        if (_particleCount < totalParticles)
        {
            _emitCounter += dt;
            if (_emitCounter < 0.f)
                _emitCounter = 0.f;
        }

        int emitCount = (int)std::fmin(totalParticles - _particleCount, _emitCounter / rate);
        addParticles(emitCount);
        _emitCounter -= rate * emitCount;

        _elapsed += dt;
        if (_elapsed < 0.f)
            _elapsed = 0.f;
        if (_duration != DURATION_INFINITY && _duration < _elapsed)
        {
            this->stopSystem();
        }
    }

    {
        for (int i = 0; i < _particleCount; ++i)
        {
            _particleData.timeToLive[i] -= dt;
        }

        for (int i = 0; i < _particleCount; ++i)
        {
            if (_particleData.timeToLive[i] <= 0.0f)
            {
                int j = _particleCount - 1;
                while (j > 0 && _particleData.timeToLive[j] <= 0)
                {
                    _particleCount--;
                    j--;
                }
                _particleData.copyParticle(i, _particleCount - 1);
                //                 if (_batchNode)
                //                 {
                //                     //disable the switched particle
                //                     int currentIndex = _particleData.atlasIndex[i];
                //                     _batchNode->disableParticle(_atlasIndex + currentIndex);
                //                     //switch indexes
                //                     _particleData.atlasIndex[_particleCount - 1] = currentIndex;
                //                 }
                --_particleCount;
                if (_particleCount == 0 && _isAutoRemoveOnFinish)
                {
                    //                     this->unscheduleUpdate();
                    //                     _parent->removeChild(this, true);
                    return;
                }
            }
        }

        if (_emitterMode == Mode::GRAVITY)
        {
            for (int i = 0; i < _particleCount; ++i)
            {
                particle_point tmp, radial = { 0.0f, 0.0f }, tangential;

                // radial acceleration
                if (_particleData.posx[i] || _particleData.posy[i])
                {
                    normalize_point(_particleData.posx[i], _particleData.posy[i], &radial);
                }
                tangential = radial;
                radial.x *= _particleData.modeA.radialAccel[i];
                radial.y *= _particleData.modeA.radialAccel[i];

                // tangential acceleration
                std::swap(tangential.x, tangential.y);
                tangential.x *= -_particleData.modeA.tangentialAccel[i];
                tangential.y *= _particleData.modeA.tangentialAccel[i];

                // (gravity + radial + tangential) * dt
                tmp.x = radial.x + tangential.x + modeA.gravity.x;
                tmp.y = radial.y + tangential.y + modeA.gravity.y;
                tmp.x *= dt;
                tmp.y *= dt;

                _particleData.modeA.dirX[i] += tmp.x;
                _particleData.modeA.dirY[i] += tmp.y;

                // this is cocos2d-x v3.0
                // if (_configName.length()>0 && _yCoordFlipped != -1)

                // this is cocos2d-x v3.0
                tmp.x = _particleData.modeA.dirX[i] * dt * _yCoordFlipped;
                tmp.y = _particleData.modeA.dirY[i] * dt * _yCoordFlipped;
                _particleData.posx[i] += tmp.x;
                _particleData.posy[i] += tmp.y;
            }
        }
        else
        {
            //Why use so many for-loop separately instead of putting them together?
            //When the processor needs to read from or write to a location in memory,
            //it first checks whether a copy of that data is in the cache.
            //And every property's memory of the particle system is continuous,
            //for the purpose of improving cache hit rate, we should process only one property in one for-loop AFAP.
            //It was proved to be effective especially for low-end machine. 
            for (int i = 0; i < _particleCount; ++i)
            {
                _particleData.modeB.angle[i] += _particleData.modeB.degreesPerSecond[i] * dt;
            }

            for (int i = 0; i < _particleCount; ++i)
            {
                _particleData.modeB.radius[i] += _particleData.modeB.deltaRadius[i] * dt;
            }

            for (int i = 0; i < _particleCount; ++i)
            {
                _particleData.posx[i] = -cosf(_particleData.modeB.angle[i]) * _particleData.modeB.radius[i];
            }
            for (int i = 0; i < _particleCount; ++i)
            {
                _particleData.posy[i] = -sinf(_particleData.modeB.angle[i]) * _particleData.modeB.radius[i] * _yCoordFlipped;
            }
        }

        //color r,g,b,a
        for (int i = 0; i < _particleCount; ++i)
        {
            _particleData.colorR[i] += _particleData.deltaColorR[i] * dt;
        }

        for (int i = 0; i < _particleCount; ++i)
        {
            _particleData.colorG[i] += _particleData.deltaColorG[i] * dt;
        }

        for (int i = 0; i < _particleCount; ++i)
        {
            _particleData.colorB[i] += _particleData.deltaColorB[i] * dt;
        }

        for (int i = 0; i < _particleCount; ++i)
        {
            _particleData.colorA[i] += _particleData.deltaColorA[i] * dt;
        }
        //size
        for (int i = 0; i < _particleCount; ++i)
        {
            _particleData.size[i] += (_particleData.deltaSize[i] * dt);
            _particleData.size[i] = (float)std::fmax(0, _particleData.size[i]);
        }
        //angle
        for (int i = 0; i < _particleCount; ++i)
        {
            _particleData.rotation[i] += _particleData.deltaRotation[i] * dt;
        }

        updateParticleQuads();
        _transformSystemDirty = false;
    }

    // only update gl buffer when visible
    if (_visible /*&& ! _batchNode*/)
    {
        postStep();
    }

    //   OG_PROFILER_STOP_CATEGORY(kProfilerCategoryParticles , "CCParticleSystem - update");
}
void ParticleSystem::draw(Texture2D *rt)
{
    if (_texture == nullptr) return;
    if (_particleCount <= 0) return;
    if (!_visible) return;
//	_texture->setBlendMode(_blendFunc.getBlendMode());
     
    // ===== UV   =====
    Size imageSize = _texture->getImageSize();
    float uvX = _clip.x / imageSize.w;
    float uvY = _clip.y / imageSize.h;
    float uvW = (_clip.x + _clip.w) / imageSize.w;
    float uvH = (_clip.y + _clip.h) / imageSize.h;

    // ===== 遍历 _quads，直接变换 + 填 UV =====
    int vertCount = 0;

    for (int i = 0; i < _particleCount; ++i)
    {
        auto& q = _quads[i];

        // 死粒子跳过（size 和颜色在 updateParticleQuads 里判断过，但颜色在这还要用到）
        if (_particleData.size[i] <= 0 || _particleData.colorA[i] <= 0)
            continue;
        _texture->setBlendMode(_blendFunc.getBlendMode());
        // UV（每个 quad 都一样）
        q.tl.texCoords = { uvX, uvY };
        q.tr.texCoords = { uvW, uvY };
        q.bl.texCoords = { uvX, uvH };
        q.br.texCoords = { uvW, uvH };

        // 颜色（在 updateParticleQuads 里已经填过，这里也可以重填，看你需要）
        // 如果 updateParticleQuads 已经填了 colors，这里可以省

        // 顶点变换：局部 -> 屏幕
        V2F_C4B_T2F* verts[4] = { &q.tl, &q.tr, &q.bl, &q.br };
        for (int j = 0; j < 4; ++j)
        {
            float vx = verts[j]->vertices.x;
            float vy = verts[j]->vertices.y;
            _transform.out(vx, vy);
            verts[j]->vertices.x = vx;
            verts[j]->vertices.y = vy;
        }

        // 索引
        int vC = vertCount * 4;
        int iC = vertCount * 6;
        _indices[iC + 0] = vC + 0;
        _indices[iC + 1] = vC + 1;
        _indices[iC + 2] = vC + 2;
        _indices[iC + 3] = vC + 1;
        _indices[iC + 4] = vC + 3;
        _indices[iC + 5] = vC + 2;

        vertCount++;
    }

    if (vertCount <= 0) return;
   // SDL_SetRenderDrawBlendMode(_render, SDL_BLENDMODE_BLEND);
    // 画粒子到 rt
    SDL_SetRenderTarget(_render, rt->getTexture());
   // SDL_RenderSetViewport(_render, nullptr);
    SDL_SetRenderDrawColor(_render, 0, 0, 0, 0);
    SDL_RenderClear(_render);
   
//     SDL_BlendMode blendMode = SDL_ComposeCustomBlendMode(
//         static_cast<SDL_BlendFactor>(BlendFactor::SRC_ALPHA),
//         static_cast<SDL_BlendFactor>(BlendFactor::ONE),
//         static_cast<SDL_BlendOperation>(BlendOperation::ADD),
//         static_cast<SDL_BlendFactor>(BlendFactor::SRC_ALPHA),
//         static_cast<SDL_BlendFactor>(BlendFactor::ONE_MINUS_SRC_ALPHA),
//         static_cast<SDL_BlendOperation>(BlendOperation::ADD)
//     );
    //SDL_SetTextureBlendMode(_texture->getTexture(), blendMode);
    // ... 画粒子 ...
    SDL_RenderGeometry(_render, _texture->getTexture(), (SDL_Vertex*)_quads.data(), vertCount * 4, _indices.data(), vertCount * 6);
    // 恢复
    SDL_SetRenderTarget(_render, nullptr);
   // SDL_RenderSetViewport(_render, nullptr);   // 恢复全屏
}
