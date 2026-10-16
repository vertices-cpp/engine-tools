#pragma once
#ifndef _PARTICLE_CONFIG_H_
#define _PARTICLE_CONFIG_H_


#include <string> 
class ParticleSystem;   // 前置声明，避免头文件循环

struct ParticleConfig
{
    // ---------- 基础 ----------
    float angle              = 90.0f;
    float angleVariance      = 0.0f;
    float duration           = -1.0f;      // -1 = 无限
    int   emitterType        = 0;          // 0=Gravity, 1=Radius
    float emissionRate       = 10.0f;
    int   maxParticles       = 150;

    // ---------- 生命 ----------
    float particleLifespan         = 1.0f;
    float particleLifespanVariance = 0.0f;

    // ---------- 起始颜色 ----------
    float startColorRed        = 1.0f;
    float startColorGreen      = 1.0f;
    float startColorBlue       = 1.0f;
    float startColorAlpha      = 1.0f;
    float startColorVarianceRed   = 0.0f;
    float startColorVarianceGreen = 0.0f;
    float startColorVarianceBlue  = 0.0f;
    float startColorVarianceAlpha = 0.0f;

    // ---------- 结束颜色 ----------
    float finishColorRed        = 1.0f;
    float finishColorGreen      = 1.0f;
    float finishColorBlue       = 1.0f;
    float finishColorAlpha      = 1.0f;
    float finishColorVarianceRed   = 0.0f;
    float finishColorVarianceGreen = 0.0f;
    float finishColorVarianceBlue  = 0.0f;
    float finishColorVarianceAlpha = 0.0f;

    // ---------- 大小 ----------
    float startParticleSize          = 10.0f;
    float startParticleSizeVariance  = 0.0f;
    float finishParticleSize         = -1.0f;   // -1 = 与起始相同
    float finishParticleSizeVariance = 0.0f;

    // ---------- 旋转 ----------
    float rotationStart         = 0.0f;
    float rotationStartVariance = 0.0f;
    float rotationEnd           = 0.0f;
    float rotationEndVariance   = 0.0f;

    // ---------- 重力模式 ----------
    float gravityx                = 0.0f;
    float gravityy                = 0.0f;
    float speed                   = 0.0f;
    float speedVariance           = 0.0f;
    float radialAcceleration      = 0.0f;
    float radialAccelVariance     = 0.0f;
    float tangentialAcceleration  = 0.0f;
    float tangentialAccelVariance = 0.0f;

    // ---------- 半径模式 ----------
    float maxRadius               = 0.0f;
    float maxRadiusVariance       = 0.0f;
    float minRadius               = 0.0f;
    float minRadiusVariance       = 0.0f;
    float rotatePerSecond         = 0.0f;
    float rotatePerSecondVariance = 0.0f;

    // ---------- 位置 ----------
    float sourcePositionx          = 0.0f;
    float sourcePositiony          = 0.0f;
    float sourcePositionVariancex  = 0.0f;
    float sourcePositionVariancey  = 0.0f;

    // ---------- 混合 ----------
    int blendFuncSource      = 770;   // GL_SRC_ALPHA
    int blendFuncDestination = 771;   // GL_ONE_MINUS_SRC_ALPHA

    // ---------- 纹理 ----------
    std::string textureFileName = "fire.png";

    // ---------- 可选 ----------
    int yCoordFlipped = 1;

    // ---------- IO ----------
    bool saveToPlist(const char* path) const;
    bool loadFromPlist(const char* path);

    // ---------- 与 ParticleSystem 互转 ----------
    void applyTo(ParticleSystem* ps) const;
    void collectFrom(const ParticleSystem* ps);
};

 
#endif // !_PARTICLE_CONFIG_H_
