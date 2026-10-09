
#ifndef _PARTICLE_SYSTEM_H_
#define _PARTICLE_SYSTEM_H_

#include <vector>
#include <string>

#include <SDL.h>
#include "Mat3.h"
#include "Texture2D.h"
  

#define MATH_TOLERANCE              2e-37f
#define OG_DEGREES_TO_RADIANS(__ANGLE__) ((__ANGLE__) * 0.01745329252f) // PI / 180
#define OG_RADIANS_TO_DEGREES(__ANGLE__) ((__ANGLE__) * 57.29577951f) // PI * 180
 
extern std::string convertPathFormatToUnixStyle(const std::string& path);

struct particle_point
{
    float x;
    float y;
};

class  ParticleData
{
public:
   
public:
    std::vector<float> posx;
    std::vector<float> posy;
    std::vector<float> startPosX;
    std::vector<float> startPosY;

    std::vector<float> colorR;
    std::vector<float> colorG;
    std::vector<float> colorB;
    std::vector<float> colorA;

    std::vector<float> deltaColorR;
    std::vector<float> deltaColorG;
    std::vector<float> deltaColorB;
    std::vector<float> deltaColorA;

    std::vector<float> size;
    std::vector<float> deltaSize;
    std::vector<float> rotation;
    std::vector<float> deltaRotation;
    std::vector<float> timeToLive;
    std::vector<unsigned int> atlasIndex;

    //! Mode A: gravity, direction, radial accel, tangential accel
    struct {
        std::vector<float> dirX;
        std::vector<float> dirY;
        std::vector<float> radialAccel;
        std::vector<float> tangentialAccel;
    } modeA;

    //! Mode B: radius mode
    struct {
        std::vector<float> angle;
        std::vector<float> degreesPerSecond;
        std::vector<float> radius;
        std::vector<float> deltaRadius;
    } modeB;

    int maxCount;
    ParticleData();
    bool init(int count);
    void release();
    unsigned int getMaxCount() { return maxCount; }

    void copyParticle(int p1, int p2)
    {
        posx[p1] = posx[p2];
        posy[p1] = posy[p2];
        startPosX[p1] = startPosX[p2];
        startPosY[p1] = startPosY[p2];

        colorR[p1] = colorR[p2];
        colorG[p1] = colorG[p2];
        colorB[p1] = colorB[p2];
        colorA[p1] = colorA[p2];

        deltaColorR[p1] = deltaColorR[p2];
        deltaColorG[p1] = deltaColorG[p2];
        deltaColorB[p1] = deltaColorB[p2];
        deltaColorA[p1] = deltaColorA[p2];

        size[p1] = size[p2];
        deltaSize[p1] = deltaSize[p2];

        rotation[p1] = rotation[p2];
        deltaRotation[p1] = deltaRotation[p2];

        timeToLive[p1] = timeToLive[p2];

        atlasIndex[p1] = atlasIndex[p2];

        modeA.dirX[p1] = modeA.dirX[p2];
        modeA.dirY[p1] = modeA.dirY[p2];
        modeA.radialAccel[p1] = modeA.radialAccel[p2];
        modeA.tangentialAccel[p1] = modeA.tangentialAccel[p2];

        modeB.angle[p1] = modeB.angle[p2];
        modeB.degreesPerSecond[p1] = modeB.degreesPerSecond[p2];
        modeB.radius[p1] = modeB.radius[p2];
        modeB.deltaRadius[p1] = modeB.deltaRadius[p2];

    }
};




class  ParticleSystem  
{
public:
    /** Mode
     * @js cc.ParticleSystem.MODE_GRAVITY;
     */
    enum class Mode
    {
        GRAVITY,
        RADIUS,
    };
    // #ifdef RELATIVE
    // #undef RELATIVE
    // #endif

        /** PositionType
         Possible types of particle positions.
         * @js cc.ParticleSystem.TYPE_FREE
         */
    enum class PositionType
    {
        FREE, /** Living particles are attached to the world and are unaffected by emitter repositioning. */

        RELATIVE, /** Living particles are attached to the world but will follow the emitter repositioning.
                   Use case: Attach an emitter to an sprite, and you want that the emitter follows the sprite.*/

        GROUPED, /** Living particles are attached to the emitter and are translated along with it. */

    };

    //* @enum
    enum {
        /** The Particle emitter lives forever. */
        DURATION_INFINITY = -1,

        /** The starting size of the particle is equal to the ending size. */
        START_SIZE_EQUAL_TO_END_SIZE = -1,

        /** The starting radius of the particle is equal to the ending radius. */
        START_RADIUS_EQUAL_TO_END_RADIUS = -1,
    };

    enum PARTICLE {

    };

    static ParticleSystem * create(const std::string& plistFile);


    static ParticleSystem* createWithTotalParticles(int numberOfParticles);

    /** Gets all ParticleSystem references
     */
     //static std::vector<ParticleSystem*>& getAllParticleSystems();
public:
    void addParticles(int count);
    void stopSystem();
    void resetSystem();
    bool isFull();

    void updateParticleQuads();
    void postStep();
    void updateWithNoTime();

    virtual bool isAutoRemoveOnFinish() const;
    virtual void setAutoRemoveOnFinish(bool var);

    // mode A
    virtual const Vec2& getGravity();
    virtual void setGravity(const Vec2& g);
    virtual float getSpeed() const;
    virtual void setSpeed(float speed);
    virtual float getSpeedVar() const;
    virtual void setSpeedVar(float speed);
    virtual float getTangentialAccel() const;
    virtual void setTangentialAccel(float t);
    virtual float getTangentialAccelVar() const;
    virtual void setTangentialAccelVar(float t);
    virtual float getRadialAccel() const;
    virtual void setRadialAccel(float t);
    virtual float getRadialAccelVar() const;
    virtual void setRadialAccelVar(float t);
    virtual bool getRotationIsDir() const;
    virtual void setRotationIsDir(bool t);

    // mode B
    virtual float getStartRadius() const;
    virtual void setStartRadius(float startRadius);
    virtual float getStartRadiusVar() const;
    virtual void setStartRadiusVar(float startRadiusVar);
    virtual float getEndRadius() const;
    virtual void setEndRadius(float endRadius);
    virtual float getEndRadiusVar() const;
    virtual void setEndRadiusVar(float endRadiusVar);
    virtual float getRotatePerSecond() const;
    virtual void setRotatePerSecond(float degrees);
    virtual float getRotatePerSecondVar() const;
    virtual void setRotatePerSecondVar(float degrees);

    void setPosition(const Vec2 & v);

    void setSkewX(const float & skx);

    void setSkewY(const float & sky);

    virtual void setScale(float s);
    virtual void setRotation(float newRotation);
    virtual void setScaleX(float newScaleX);
    virtual void setScaleY(float newScaleY);

    virtual bool isActive() const;
    virtual bool isBlendAdditive() const;
    virtual void setBlendAdditive(bool value);

    int getAtlasIndex() const { return _atlasIndex; }
    void setAtlasIndex(int index) { _atlasIndex = index; }

    unsigned int getParticleCount() const { return _particleCount; }

    float getDuration() const { return _duration; }
    void setDuration(float duration) { _duration = duration; }

    const Vec2& getSourcePosition() const { return _sourcePosition; }
    void setSourcePosition(const Vec2& pos) { _sourcePosition = pos; }

    const Vec2& getPosVar() const { return _posVar; }
    void setPosVar(const Vec2& pos) { _posVar = pos; }

    float getLife() const { return _life; }
    void setLife(float life) { _life = life; }

    float getLifeVar() const { return _lifeVar; }
    void setLifeVar(float lifeVar) { _lifeVar = lifeVar; }

    float getAngle() const { return _angle; }
    void setAngle(float angle) { _angle = angle; }

    float getAngleVar() const { return _angleVar; }
    void setAngleVar(float angleVar) { _angleVar = angleVar; }

    Mode getEmitterMode() const { return _emitterMode; }
    void setEmitterMode(Mode mode) { _emitterMode = mode; }

    float getStartSize() const { return _startSize; }
    void setStartSize(float startSize) { _startSize = startSize; }

    float getStartSizeVar() const { return _startSizeVar; }
    void setStartSizeVar(float sizeVar) { _startSizeVar = sizeVar; }

    float getEndSize() const { return _endSize; }
    void setEndSize(float endSize) { _endSize = endSize; }

    float getEndSizeVar() const { return _endSizeVar; }
    void setEndSizeVar(float sizeVar) { _endSizeVar = sizeVar; }

    const  Color4F& getStartColor() const { return _startColor; }
    void setStartColor(const Color4F& color) { _startColor = color; }

    const Color4F& getStartColorVar() const { return _startColorVar; }
    void setStartColorVar(const Color4F& color) { _startColorVar = color; }

    const Color4F& getEndColor() const { return _endColor; }
    void setEndColor(const Color4F& color) { _endColor = color; }

    const Color4F& getEndColorVar() const { return _endColorVar; }
    void setEndColorVar(const Color4F& color) { _endColorVar = color; }

    float getStartSpin() const { return _startSpin; }
    void setStartSpin(float spin) { _startSpin = spin; }

    float getStartSpinVar() const { return _startSpinVar; }
    void setStartSpinVar(float pinVar) { _startSpinVar = pinVar; }

    float getEndSpin() const { return _endSpin; }
    void setEndSpin(float endSpin) { _endSpin = endSpin; }

    float getEndSpinVar() const { return _endSpinVar; }
    void setEndSpinVar(float endSpinVar) { _endSpinVar = endSpinVar; }

    float getEmissionRate() const { return _emissionRate; }
    void setEmissionRate(float rate) { _emissionRate = rate; }

    virtual int getTotalParticles() const;
    virtual void setTotalParticles(int totalParticles);

    void updateTexCoords();

    void initTexCoordsWithRect(const Rect & pointRect);

    void initIndices();

	const BlendFunc & getBlendFunc() const;

	void setBlendFunc(const BlendFunc & blendFunc);

    void setOpacityModifyRGB(bool opacityModifyRGB) { _opacityModifyRGB = opacityModifyRGB; }
    bool isOpacityModifyRGB() const { return _opacityModifyRGB; }

    PositionType getPositionType() const { return _positionType; }
    void setPositionType(PositionType type) { _positionType = type; }

    virtual void onEnter();
    virtual void onExit();
    virtual void update(float dt);

    virtual  Texture2D* getTexture() const;
    virtual void setTexture(Texture2D* texture);
    virtual void updateBlendFunc();

    const std::string& getResourceFile() const { return _plistFile; }

    virtual void start();
    virtual void stop();

    void setSourcePositionCompatible(bool sourcePositionCompatible) { _sourcePositionCompatible = sourcePositionCompatible; }
    bool isSourcePositionCompatible() const { return _sourcePositionCompatible; }

    ParticleSystem();
    virtual ~ParticleSystem();
    static void setTotalParticleCountFactor(float factor);
    static float getTotalParticleCountFactor() { return __totalParticleCountFactor; }
    bool init();
 
	//bool initWithParticlePlistData(const std::string & plistFile);
	virtual bool initWithTotalParticles(int numberOfParticles);

    virtual bool isPaused() const;
    virtual void pauseEmissions();
    virtual void resumeEmissions();

    void draw(Texture2D *rt);

 
    Vec2 getPosition() { return _position; }
    void setParticleRenderer(SDL_Renderer *ren) { _render = ren; }
    SDL_Renderer* getRenderer() { return _render; }
    bool isRenderer()const { return _render!=nullptr; }

    Vec2  convertToWorldSpace(const Vec2&  Point)const {
        Mat3 tmp(getNodeToWorldTransform());
        Vec2 vec2(Point.x, Point.y);
        Vec2 ret;
        tmp.transformPoint(vec2, &ret);
        return Vec2(ret.x, ret.y);
    }
    Mat3  getNodeToWorldTransform() const
    {
        Mat3 tmp;  
        return tmp * getNodeToParentTransform();
    } 
    const Mat3 & getNodeToParentTransform() const
    {
        if (_transformDirty)
        {
            Mat3 translation = Mat3::createTranslation(_position.x, _position.y);
            //初始化
            _transform.reset();
            if (_scaleX != 1.f || _scaleY != 1.f)
            {
                _transform = _transform.createScale(_scaleX, _scaleY);
            }
            if (_skewX != 0.0f || _skewY != 0.0f)
            {
                _transform = _transform * Mat3::createSkew(_skewX, _skewY);
            }

            Mat3 rotation;
            if (_rotation != 0.0f)
            {
                rotation = Mat3::createRotation(_center, _rotation);
            }
            //先缩放，倾斜，旋转， 再平移

            _transform = translation * rotation * _transform;
        }

        _transformDirty = false;

        return _transform;
    }
     

    Mat3  getWorldToNodeTransform() const
    {
        return getNodeToWorldTransform().getInversed();
    }
protected:
    bool _isBlendAdditive;
    bool _isAutoRemoveOnFinish;

    std::string _plistFile;
    float _elapsed;

    struct {
        Vec2 gravity;
        float speed;
        float speedVar;
        float tangentialAccel;
        float tangentialAccelVar;
        float radialAccel;
        float radialAccelVar;
        bool rotationIsDir;
    } modeA;

    struct {
        float startRadius;
        float startRadiusVar;
        float endRadius;
        float endRadiusVar;
        float rotatePerSecond;
        float rotatePerSecondVar;
    } modeB;

    ParticleData _particleData;

    std::string _configName;
    float _emitCounter;

    int _atlasIndex;
    bool _transformSystemDirty;
    int _allocatedParticles;

    bool _isActive;
    int _particleCount;

    static float __totalParticleCountFactor;

    float _duration;
    Vec2 _sourcePosition;
     Vec2 _posVar;
    float _life;
    float _lifeVar;
    float _angle;
    float _angleVar;

    Mode _emitterMode;

    float _startSize;
    float _startSizeVar;
    float _endSize;
    float _endSizeVar;
    Color4F _startColor;
    Color4F _startColorVar;
    Color4F _endColor;
    Color4F _endColorVar;
    float _startSpin;
    float _startSpinVar;
    float _endSpin;
    float _endSpinVar;
    float _emissionRate;
    int _totalParticles;

    BlendFunc _blendFunc;
    bool _opacityModifyRGB;
    int _yCoordFlipped;

    PositionType _positionType;
    bool _paused;
    bool _sourcePositionCompatible;
    Rect _clip = Rect::ZERO;
    //添加部分
    mutable Mat3 _transform;
    mutable bool _transformDirty = true,  _transformUpdated = true;
    float _rotation = 0.0f ;
    Vec2 _center = { 0.0f,0.0f };
    float _skewX = 0.0f;
    float _skewY = 0.0f;
    float _scaleX = 1.0f, _scaleY = 1.0f;
    bool  _additive = false;
    Vec2 _position;
    Size _contentSize;
    bool _visible = true;
    std::string _name;
    std::vector<V2F_C4B_T2F_Quad> _quads;
    std::vector<int> _indices;
    Texture2D *_texture;
    SDL_Renderer *_render =nullptr;

private: 
    ParticleSystem(const ParticleSystem &) = delete;  
    ParticleSystem &operator =(const ParticleSystem &) = delete;
};

// end of _2d group
/// @}


const unsigned char __firePngData[] = {
0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x08, 0x06, 0x00, 0x00, 0x00, 0x73, 0x7A, 0x7A,
0xF4, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4D, 0x41, 0x00, 0x00, 0xAF, 0xC8, 0x37, 0x05, 0x8A,
0xE9, 0x00, 0x00, 0x00, 0x19, 0x74, 0x45, 0x58, 0x74, 0x53, 0x6F, 0x66, 0x74, 0x77, 0x61, 0x72,
0x65, 0x00, 0x41, 0x64, 0x6F, 0x62, 0x65, 0x20, 0x49, 0x6D, 0x61, 0x67, 0x65, 0x52, 0x65, 0x61,
0x64, 0x79, 0x71, 0xC9, 0x65, 0x3C, 0x00, 0x00, 0x02, 0x64, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA,
0xC4, 0x97, 0x89, 0x6E, 0xEB, 0x20, 0x10, 0x45, 0xBD, 0xE1, 0x2D, 0x4B, 0xFF, 0xFF, 0x37, 0x5F,
0x5F, 0x0C, 0xD8, 0xC4, 0xAE, 0x2D, 0xDD, 0xA9, 0x6E, 0xA7, 0x38, 0xC1, 0x91, 0xAA, 0x44, 0xBA,
0xCA, 0x06, 0xCC, 0x99, 0x85, 0x01, 0xE7, 0xCB, 0xB2, 0x64, 0xEF, 0x7C, 0x55, 0x2F, 0xCC, 0x69,
0x56, 0x15, 0xAB, 0x72, 0x68, 0x81, 0xE6, 0x55, 0xFE, 0xE8, 0x62, 0x79, 0x62, 0x04, 0x36, 0xA3,
0x06, 0xC0, 0x9B, 0xCA, 0x08, 0xC0, 0x7D, 0x55, 0x80, 0xA6, 0x54, 0x98, 0x67, 0x11, 0xA8, 0xA1,
0x86, 0x3E, 0x0B, 0x44, 0x41, 0x00, 0x33, 0x19, 0x1F, 0x21, 0x43, 0x9F, 0x5F, 0x02, 0x68, 0x49,
0x1D, 0x20, 0x1A, 0x82, 0x28, 0x09, 0xE0, 0x4E, 0xC6, 0x3D, 0x64, 0x57, 0x39, 0x80, 0xBA, 0xA3,
0x00, 0x1D, 0xD4, 0x93, 0x3A, 0xC0, 0x34, 0x0F, 0x00, 0x3C, 0x8C, 0x59, 0x4A, 0x99, 0x44, 0xCA,
0xA6, 0x02, 0x88, 0xC7, 0xA7, 0x55, 0x67, 0xE8, 0x44, 0x10, 0x12, 0x05, 0x0D, 0x30, 0x92, 0xE7,
0x52, 0x33, 0x32, 0x26, 0xC3, 0x38, 0xF7, 0x0C, 0xA0, 0x06, 0x40, 0x0F, 0xC3, 0xD7, 0x55, 0x17,
0x05, 0xD1, 0x92, 0x77, 0x02, 0x20, 0x85, 0xB7, 0x19, 0x18, 0x28, 0x4D, 0x05, 0x19, 0x9F, 0xA1,
0xF1, 0x08, 0xC0, 0x05, 0x10, 0x57, 0x7C, 0x4F, 0x01, 0x10, 0xEF, 0xC5, 0xF8, 0xAC, 0x76, 0xC8,
0x2E, 0x80, 0x14, 0x99, 0xE4, 0xFE, 0x44, 0x51, 0xB8, 0x52, 0x14, 0x3A, 0x32, 0x22, 0x00, 0x13,
0x85, 0xBF, 0x52, 0xC6, 0x05, 0x8E, 0xE5, 0x63, 0x00, 0x86, 0xB6, 0x9C, 0x86, 0x38, 0xAB, 0x54,
0x74, 0x18, 0x5B, 0x50, 0x58, 0x6D, 0xC4, 0xF3, 0x89, 0x6A, 0xC3, 0x61, 0x8E, 0xD9, 0x03, 0xA8,
0x08, 0xA0, 0x55, 0xBB, 0x40, 0x40, 0x3E, 0x00, 0xD2, 0x53, 0x47, 0x94, 0x0E, 0x38, 0xD0, 0x7A,
0x73, 0x64, 0x57, 0xF0, 0x16, 0xFE, 0x95, 0x82, 0x86, 0x1A, 0x4C, 0x4D, 0xE9, 0x68, 0xD5, 0xAE,
0xB8, 0x00, 0xE2, 0x8C, 0xDF, 0x4B, 0xE4, 0xD7, 0xC1, 0xB3, 0x4C, 0x75, 0xC2, 0x36, 0xD2, 0x3F,
0x2A, 0x7C, 0xF7, 0x0C, 0x50, 0x60, 0xB1, 0x4A, 0x81, 0x18, 0x88, 0xD3, 0x22, 0x75, 0xD1, 0x63,
0x5C, 0x80, 0xF7, 0x19, 0x15, 0xA2, 0xA5, 0xB9, 0xB5, 0x5A, 0xB7, 0xA4, 0x34, 0x7D, 0x03, 0x48,
0x5F, 0x17, 0x90, 0x52, 0x01, 0x19, 0x95, 0x9E, 0x1E, 0xD1, 0x30, 0x30, 0x9A, 0x21, 0xD7, 0x0D,
0x81, 0xB3, 0xC1, 0x92, 0x0C, 0xE7, 0xD4, 0x1B, 0xBE, 0x49, 0xF2, 0x04, 0x15, 0x2A, 0x52, 0x06,
0x69, 0x31, 0xCA, 0xB3, 0x22, 0x71, 0xBD, 0x1F, 0x00, 0x4B, 0x82, 0x66, 0xB5, 0xA7, 0x37, 0xCF,
0x6F, 0x78, 0x0F, 0xF8, 0x5D, 0xC6, 0xA4, 0xAC, 0xF7, 0x23, 0x05, 0x6C, 0xE4, 0x4E, 0xE2, 0xE3,
0x95, 0xB7, 0xD3, 0x40, 0xF3, 0xA5, 0x06, 0x1C, 0xFE, 0x1F, 0x09, 0x2A, 0xA8, 0xF5, 0xE6, 0x3D,
0x00, 0xDD, 0xAD, 0x02, 0x2D, 0xC4, 0x4D, 0x66, 0xA0, 0x6A, 0x1F, 0xD5, 0x2E, 0xF8, 0x8F, 0xFF,
0x2D, 0xC6, 0x4F, 0x04, 0x1E, 0x14, 0xD0, 0xAC, 0x01, 0x3C, 0xAA, 0x5C, 0x1F, 0xA9, 0x2E, 0x72,
0xBA, 0x49, 0xB5, 0xC7, 0xFA, 0xC0, 0x27, 0xD2, 0x62, 0x69, 0xAE, 0xA7, 0xC8, 0x04, 0xEA, 0x0F,
0xBF, 0x1A, 0x51, 0x50, 0x61, 0x16, 0x8F, 0x1B, 0xD5, 0x5E, 0x03, 0x75, 0x35, 0xDD, 0x09, 0x6F,
0x88, 0xC4, 0x0D, 0x73, 0x07, 0x82, 0x61, 0x88, 0xE8, 0x59, 0x30, 0x45, 0x8E, 0xD4, 0x7A, 0xA7,
0xBD, 0xDA, 0x07, 0x67, 0x81, 0x40, 0x30, 0x88, 0x55, 0xF5, 0x11, 0x05, 0xF0, 0x58, 0x94, 0x9B,
0x48, 0xEC, 0x60, 0xF1, 0x09, 0xC7, 0xF1, 0x66, 0xFC, 0xDF, 0x0E, 0x84, 0x7F, 0x74, 0x1C, 0x8F,
0x58, 0x44, 0x77, 0xAC, 0x59, 0xB5, 0xD7, 0x67, 0x00, 0x12, 0x85, 0x4F, 0x2A, 0x4E, 0x17, 0xBB,
0x1F, 0xC6, 0x00, 0xB8, 0x99, 0xB0, 0xE7, 0x23, 0x9D, 0xF7, 0xCF, 0x6E, 0x44, 0x83, 0x4A, 0x45,
0x32, 0x40, 0x86, 0x81, 0x7C, 0x8D, 0xBA, 0xAB, 0x1C, 0xA7, 0xDE, 0x09, 0x87, 0x48, 0x21, 0x26,
0x5F, 0x4A, 0xAD, 0xBA, 0x6E, 0x4F, 0xCA, 0xFB, 0x23, 0xB7, 0x62, 0xF7, 0xCA, 0xAD, 0x58, 0x22,
0xC1, 0x00, 0x47, 0x9F, 0x0B, 0x7C, 0xCA, 0x73, 0xC1, 0xDB, 0x9F, 0x8C, 0xF2, 0x17, 0x1E, 0x4E,
0xDF, 0xF2, 0x6C, 0xF8, 0x67, 0xAF, 0x22, 0x7B, 0xF3, 0xEB, 0x4B, 0x80, 0x01, 0x00, 0xB8, 0x21,
0x72, 0x89, 0x08, 0x10, 0x07, 0x7D, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42,
0x60, 0x82
};

 


#endif //__CCPARTICLE_SYSTEM_H__


