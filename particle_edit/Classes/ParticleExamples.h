
#ifndef _PARTICLE_EXAMPLE_H_
#define _PARTICLE_EXAMPLE_H_

#include "Texture2D.h"
#include "ParticleSystem.h"

// #define OG_DISALLOW_COPY_AND_ASSIGN(TypeName) \
//     TypeName(const TypeName &) = delete; \
//     TypeName &operator =(const TypeName &) = delete;

 

class ParticleExample : public ParticleSystem{
public:
	enum PatticleStyle
	{
		NONE,
		FIRE,
		FIRE_WORK,
		SUN,
		GALAXY,
		FLOWER,
		METEOR,
		SPIRAL,
		EXPLOSION,
		SMOKE,
		SNOW,
		RAIN,
	};

	static ParticleExample* create();
	ParticleExample() {}
	virtual ~ParticleExample() {}
    void setRenderer(SDL_Renderer *ren) { setParticleRenderer(ren); }
	

	void setStyle(PatticleStyle style);
 
    PatticleStyle getStyle() { return _style; }

	void start_run();
protected:
	PatticleStyle _style = NONE;
	
private:
	ParticleExample(const ParticleExample &) = delete;
	ParticleExample &operator =(const ParticleExample &) = delete;
};
 

#endif
