#include "GoblinRenderer.h"
#include "GoblinRay.h"
#include "GoblinColor.h"
#include "GoblinCamera.h"
#include "GoblinFilm.h"
#include "GoblinUtils.h"

namespace Goblin {

    Renderer::Renderer(const RenderSetting& setting):
        mSamples(NULL), mSampler(NULL), mSetting(setting) {
    }

    Renderer::~Renderer() {
        if(mSamples) {
            delete [] mSamples;
            mSamples = NULL;
        }
        if(mSampler) {
            delete mSampler;
            mSampler = NULL;
        }
    }

    void Renderer::querySampleQuota(const ScenePtr& scene, Sampler* sampler) {
        uint32_t oneDOffset = sampler->requestOneDQuota(4);
        uint32_t twoDOffset = sampler->requestTwoDQuota(25);
    }

    void Renderer::render(const ScenePtr& scene) {
        const CameraPtr camera = scene->getCamera();
        Film* film = camera->getFilm();
        int xStart, xEnd, yStart, yEnd;
        film->getSampleRange(&xStart, &xEnd, &yStart, &yEnd);
        if(mSampler != NULL) {
            delete mSampler;
        }
        if(mSamples != NULL) {
            delete [] mSamples;
        }
        int samplePerPixel = mSetting.samplePerPixel;
        mSampler = new Sampler(xStart, xEnd, yStart, yEnd, 
            samplePerPixel);
        // bluh....api dependency on the above sampler
        // TODO wragle out the on APIdependency  
        querySampleQuota(scene, mSampler);
        int batchAmount = mSampler->maxSamplesPerRequest();
        mSamples = mSampler->allocateSampleBuffer(batchAmount);
        // temp progress reporter so that waiting can be not that boring...
        // TODO make this something more elegant
        unsigned long int accumulatedBuffer = 0;
        unsigned long int accumulatedSamples = 0;
        unsigned long int maxTotalSamples = mSampler->maxTotalSamples();
        unsigned long int reportStep = maxTotalSamples / 100;
        string backspace = "\b\b\b\b\b\b\b\b\b\b\b\b\b";

        int sampleNum = 0;
        while((sampleNum = mSampler->requestSamples(mSamples)) > 0) {
            for(int i = 0; i < sampleNum; ++i) {
                Ray ray;
                float w = camera->generateRay(mSamples[i], &ray);
                Color L = w * Li(scene, ray);
                film->addSample(mSamples[i], L);
            }

            // print out progress
            accumulatedBuffer += sampleNum;
            if(accumulatedBuffer > reportStep) {
                accumulatedSamples += accumulatedBuffer;
                accumulatedBuffer = 0;
                std::cout << backspace;
                std::cout << "progress %";
                std::cout << 100 * accumulatedSamples / maxTotalSamples;
                std::cout.flush();
            }
        }
        std::cout << backspace;

        //for now leave this here as debug purpose......
        //Ray ray(Vector3(0, 0, 0.0), Vector3(0, 0, 1), 0); 
        //Color L = Li(scene, ray);

        film->writeImage();
    }

    Color Renderer::Li(const ScenePtr& scene, const Ray& ray) const {
        Color Li = Color::Black;
        float epsilon;
        Intersection intersection;
        if(scene->intersect(ray, &epsilon, &intersection)) {
            const MaterialPtr& material = 
                intersection.primitive->getMaterial();
            // if intersect an area light
            Li += intersection.Le(-ray.d);

            const vector<Light*>& lights = scene->getLights();
            // direct light contribution
            for(size_t i = 0; i < lights.size(); ++i) {
                Vector3 wo = -ray.d;
                Vector3 wi;
                Ray shadowRay;
                const Fragment& fragment = intersection.fragment;
                Color L = lights[i]->sampleL(fragment.position, epsilon, &wi, 
                    &shadowRay);
                Color f = material->bsdf(fragment, wo, wi);
                if(f != Color::Black && !scene->intersect(shadowRay)) {
                    Li += f * L * absdot(fragment.normal, wi);
                }
            }
            // reflection and refraction
            if(ray.depth < mSetting.maxRayDepth) {
                Li += specularReflect(scene, ray, epsilon, intersection);
                Li += specularRefract(scene, ray, epsilon, intersection);
            }
        }
        return Li;
    }

    Color Renderer::specularReflect(const ScenePtr& scene, const Ray& ray, 
        float epsilon, const Intersection& intersection) const {
        Color L(Color::Black);
        const Vector3& n = intersection.fragment.normal;
        const Vector3& p = intersection.fragment.position;
        const MaterialPtr& material = 
            intersection.primitive->getMaterial();
        Vector3 wo = -ray.d;
        Vector3 wi;
        Color f = material->sampleBSDF(intersection.fragment, 
            wo, &wi, BSDFType(BSDFSpecular | BSDFReflection));
        if(f != Color::Black && absdot(wi, n) != 0.0f) {
            Ray reflectiveRay(p, wi, epsilon);
            reflectiveRay.depth = ray.depth + 1;
            Color Lr = Li(scene, reflectiveRay);
            L += f * Lr * absdot(wi, n);
        }
        return L;
    }

    Color Renderer::specularRefract(const ScenePtr& scene, const Ray& ray, 
        float epsilon, const Intersection& intersection) const {
        Color L(Color::Black);
        const Vector3& n = intersection.fragment.normal;
        const Vector3& p = intersection.fragment.position;
        const MaterialPtr& material = 
            intersection.primitive->getMaterial();
        Vector3 wo = -ray.d;
        Vector3 wi;
        Color f = material->sampleBSDF(intersection.fragment, 
            wo, &wi, BSDFType(BSDFSpecular | BSDFTransmission));
        if(f != Color::Black && absdot(wi, n) != 0.0f) {
            Ray refractiveRay(p, wi, epsilon);
            refractiveRay.depth = ray.depth + 1;
            Color Lr = Li(scene, refractiveRay);
            L += f * Lr * absdot(wi, n);
        }
        return L;
    }

}
