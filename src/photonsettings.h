#ifndef PHOTONSETTINGS_H
#define PHOTONSETTINGS_H
class PhotonSettings
{
public:

    int superSamples; // for say, stratified sampling
    float attenuation; // refracted path absorption through non-vacuum spaces
    float scattering;
    float radiusScalingFactor;
    float noiseBiasRatio;
    bool useMedium; // enable volumetric mediums
    bool renderSplines;

    bool lightEnabled; // TODO: assume one light source for now

    // Recursion depth for photon scattering.
    int maxDepthScatter;
    // Recursion depth for rendering.
    int maxDepthRender;
    // Used for bumping.
    float epsilon;
    // Number of beamettes to shoot into the scene.
    int numBeamettesDir;
    int numBeamettesInDir;
    // Number of samples to take of direct light sources.
    int directSamples;
    // number of ray samples for final gather.
    int gatherSamples;
    // Max distance between intersection point and photons in map.
    //TODO: is this the same as radius scaling factor?
    float gatherRadius;
    // Whether or not to use final gather
    bool useFinalGather;
    // Expected raymarch step along the ray when scattering.
    // TODO: should this just be taken care of in the fog stuff?
    float dist;
    // The rendered intensity scale
    float beamIntensity;
    // Spread of the beam (angle that light can scatter from the emittor)
    float beamSpread;
};

#endif // PHOTONSETTINGS_H
