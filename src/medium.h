#ifndef MEDIUM_H
#define MEDIUM_H

#include <G3D/G3DAll.h>

struct Medium
{

    float stepsize;
    Radiance3 attenuation;
    Radiance3 emission;

    virtual void init()
    {
        stepsize = 0.01f;
        attenuation = Radiance3::zero();
        emission = Radiance3::zero();
    }

    virtual void init( const Any &any )
    {
        init();
        if ( any.containsKey("stepsize") )
            stepsize = any["stepsize"];
        if ( any.containsKey("attenuation") )
            attenuation = any["attenuation"];
        if ( any.containsKey("emission") )
            emission = any["emission"];
    }

    float fixStepSize( float distance, float step ) const
    {
        return distance/max(1.f,round(distance/step));
    }

    virtual Radiance3 estimateAttenuation( const Ray&, float distance ) const = 0;
    virtual Radiance3 estimateAddedRadiance( const Ray&, float distance ) const = 0;

    static shared_ptr<Medium> create( const Any &any );

    virtual bool isVacuum() const = 0;
    virtual bool attenuates() const = 0;
    virtual bool emissive() const = 0;


    static Radiance3 exp( float d, const Radiance3 &tau )
    {
        return Radiance3( ::exp(-d*tau.r),
                          ::exp(-d*tau.g),
                          ::exp(-d*tau.b) );
    }

};

struct HomogeneousMedium : public Medium
{

    HomogeneousMedium() { init(); }

    HomogeneousMedium( const Any &any )
    {
        any.verifyName( "Medium" );
        any.verifyType( Any::TABLE );
        init( any );
    }

    // TODO: Implement functions below for homogeneous media
    virtual Radiance3 estimateAttenuation( const Ray&,
                                           float distance ) const
    {
        return Radiance3::black();
    }

    virtual Radiance3 estimateAddedRadiance( const Ray&,
                                             float distance ) const
    {
        return Radiance3::black();
    }

    virtual bool isVacuum() const { return false; }
    virtual bool attenuates() const { return false; }
    virtual bool emissive() const { return false; }

};

struct ExponentialDensityMedium : public Medium
{

    float density;
    float decay;
    float stepscale;

    virtual void init()
    {
        Medium::init();
        density = 0.f;
        decay = 0.f;
        stepscale = 1.f;
    }

    virtual void init( const Any &any )
    {
        Medium::init(any);
        if ( any.containsKey("density") )
            density = any["density"];
        if ( any.containsKey("decay") )
            decay = any["decay"];
        if ( any.containsKey("stepscale") )
            stepscale = any["stepscale"];
    }

    ExponentialDensityMedium() { init(); }

    ExponentialDensityMedium( const Any &any )
    {
        any.verifyName( "Medium" );
        any.verifyType( Any::TABLE );
        init(any);
    }

    // TODO: Implement functions below for exponential media

    virtual Radiance3 estimateAttenuation( const Ray &ray,
                                           float distance ) const
    {
        return Radiance3::black();
    }

    virtual Radiance3 estimateAddedRadiance( const Ray &ray,
                                             float distance ) const
    {
        return Radiance3::black();
    }

    virtual bool isVacuum() const
    {
        return false;
    }

    virtual bool attenuates() const { return false; }
    virtual bool emissive() const { return false; }

};

inline
shared_ptr<Medium>
Medium::create( const Any &any )
{
    any.verifyName( "Medium" );
    any.verifyType( Any::TABLE );

    if ( any.containsKey("type") ) {
        String type = any["type"];
        if ( type == "homogeneous" ) {
            return shared_ptr<Medium>( new HomogeneousMedium(any) );
        } else if ( type == "exponential" ) {
            return shared_ptr<Medium>( new ExponentialDensityMedium(any) );
        }
    }

    return shared_ptr<Medium>( new HomogeneousMedium() );

}

#endif // MEDIUM_H
