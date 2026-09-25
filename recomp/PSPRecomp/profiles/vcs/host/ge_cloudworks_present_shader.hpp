#pragma once

namespace vcs {

// Direct SM5.1 port of the CloudWorks implementation used by ProperShaders.
// Density profiles, adaptive integration, sun shadow march, Beer-Lambert
// extinction, Mie/Rayleigh lighting and front-to-back layer composition retain
// the source equations and constants. The only platform adaptation is the
// fullscreen input/ray contract used by VCSNative's DX12 backend.
// CloudWorks: Brian Tu (RTU), CC BY-NC-SA 3.0.
inline constexpr char kCloudWorksPresentShaderHlsl[] = R"CLOUD_HLSL(
// Cloud pass descriptor contract:
//   t0 = sparse march texture for CloudTemporalResolvePS, or resolved cloud
//        history for CloudCompositePS/PresentPS
//   t1 = previous resolved cloud history for CloudTemporalResolvePS
//   s0 = clamp sampler (linear for resolve/composite)
Texture2D<float4> CloudTexture0 : register(t0);
Texture2D<float4> CloudTexture1 : register(t1);
SamplerState CloudSampler : register(s0);

cbuffer CloudState : register(b0) {
    float3 CloudRayRight; float g_Time;
    float3 CloudRayUp; float randomSeed;
    float3 CloudRayForward; float g_Opacity;
    float3 CloudCameraPosition; uint g_Settings;
    float3 g_CloudCoverage; float g_CloudSpeed;
    float3 vSunLightDir; float fDayProgression;
    float3 g_vSunColor; float g_AtmDense;
    float3 g_vCloudBaseColor; float g_Mist;
    float3 g_FogColor; float g_FogDens;
    float g_Brightness; float3 CloudPadding;
};

// Temporal pass contract (20 DWORDs). Together with CloudState's 40 DWORDs,
// a two-SRV descriptor table and one sampler table this consumes 62 of the
// D3D12 root signature's 64 DWORD budget.
//
// Translation invalidates history on the CPU.  The previous ray basis below
// therefore only has to reproject camera rotation; it is the same world-ray
// basis used by CloudState and avoids importing matrix-layout conventions from
// either the PSP GE or D3D9 ProperShaders implementation.
cbuffer CloudTemporalState : register(b2) {
    float3 PrevCloudRayRight; float CloudHistoryValid;
    float3 PrevCloudRayUp; float CloudTemporalBlend;
    float3 PrevCloudRayForward; float CloudSpatialMix;
    float2 CloudTexelSize; float2 CloudSubPixel;
    float CloudFullResolutionMarch;
    float CloudClampExpand;
    float2 CloudTemporalPadding;
};

struct PresentVertexOutput { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };
PresentVertexOutput PresentVS(uint id : SV_VertexID) {
    PresentVertexOutput o;
    if (id == 0u) { o.position=float4(-1,-1,0,1); o.uv=float2(0,1); }
    else if (id == 1u) { o.position=float4(-1,3,0,1); o.uv=float2(0,-1); }
    else { o.position=float4(3,-1,0,1); o.uv=float2(2,1); }
    return o;
}
float4 PresentPS(PresentVertexOutput i) : SV_TARGET {
    return CloudTexture0.SampleLevel(CloudSampler, i.uv, 0.0);
}

static const float VC_HASH_MUL = 1332.03398875;
float hash(float n) { return frac(sin(n / 1873.1873 + randomSeed) * VC_HASH_MUL); }
float noise2d(float3 p) {
    float3 fr=floor(p), ft=frac(p); float n=1153*fr.x+2381*fr.y+p.z;
    float nr=n+1153, nd=n+2381, no=nr+2381;
    return lerp(lerp(hash(n),hash(nr),ft.x),lerp(hash(nd),hash(no),ft.x),ft.y);
}
float noise3d(float3 p) {
    float3 fr=floor(p), ft=frac(p); float n=1153*fr.x+2381*fr.y+fr.z;
    float nr=n+1153, nd=n+2381, no=nr+2381;
    float v=lerp(hash(n),hash(n+1),ft.z), vr=lerp(hash(nr),hash(nr+1),ft.z);
    float vd=lerp(hash(nd),hash(nd+1),ft.z), vo=lerp(hash(no),hash(no+1),ft.z);
    return lerp(lerp(v,vr,ft.x),lerp(vd,vo,ft.x),ft.y);
}
float map(float x,float a,float b,float c,float d) { return (x-b)/(a-b)*(c-d)+d; }
float clampMap(float x,float a,float b,float c,float d) { return saturate((x-b)/(a-b))*(c-d)+d; }

static const float atmosphereStep=15.0, lightStep=3.0, fix=0.00001;
static const float3 sunLightStrength=685.0*float3(1.0,0.96,0.949);
static const float LightingDecay=300.0, rayleighStrength=1.85, rayleighDecay=900.0;
static const float3 waveLengthFactor=float3(6.5*6.5*6.5*6.5,5.4*5.4*5.4*5.4,4.5*4.5*4.5*4.5);
static const float3 scatteringFactor=waveLengthFactor/rayleighDecay;
static const float earthRadius=6.421, groundHeight=6.371, game2atm=1000000.0;
static const float3 AtmOrigin=float3(0,0,groundHeight);
static const float4 earth=float4(0,0,0,earthRadius);
float3 Game2Atm(float3 p) { return p/game2atm+AtmOrigin; }
float3 Game2Atm_Alt(float3 p) { return float3(0,0,p.z/game2atm)+AtmOrigin; }
float4 sphereCast(float3 origin,float3 ray,float4 sphere,float steps,out float3 begin) {
    begin=origin;
    float3 p=origin-sphere.xyz; float r=length(p), d=length(cross(p,ray));
    if(d>sphere.w+fix){begin=0;return 0;} float sr=sqrt(sphere.w*sphere.w-d*d),dr=-dot(p,ray);
    float3 pc=origin+ray*dr,pf=pc+ray*sr,pb=pc-ray*sr; float sl;
    if(r>sphere.w){begin=pb;sl=sr*2/steps;}else{begin=origin;sl=length(pf-origin)/steps;}
    return float4(ray*sl,sl);
}
float2 Density(float3 pos,float4 sphere,float strength,float condense) {
    float atmDensity=3.0+g_AtmDense,fogDensity=0.25+g_Mist;
    float r=groundHeight,h=length(pos-sphere.xyz)-r,ep=exp(-(sphere.w-r)*condense);
    float fog=fogDensity*(1.0/(1200.0*h+0.5)-0.04)/1.96;
    if(h<0)return float2(strength,fogDensity);
    return float2((exp(-h*condense)-ep)/(1.0-ep)*strength,fog);
}
float3 rayleighScattering(float c){return (1+c*c)*rayleighStrength/waveLengthFactor;}
float MiePhase(float c){return 1.0+1.6*exp(20.0*(c-1.0));}
float MieScattering(float c){return (0.125+g_Mist*0.1)*MiePhase(c);}
float3 LightDecay(float r,float m){return exp(-r/scatteringFactor-m*100.0);}
float3 SunLight(float3 light,float3 position,float3 lightDirection,float4 sphere){
    float3 smp=position;float4 sms=sphereCast(position,lightDirection,sphere,lightStep,smp);float2 dl=0;
    [unroll]for(int j=0;j<3;j++){smp+=sms.xyz/2;dl+=Density(smp,sphere,3.0+g_AtmDense,1.0)*sms.w;smp+=sms.xyz/2;}
    return light*LightDecay(dl.x,dl.y)/LightingDecay;
}
float3 LightSource(float3 active,float day,float3 color,out float3 source){
    source=active;
    if(day<-0.2)return sunLightStrength*smoothstep(0.1,0.3,-day)*color;
    return sunLightStrength*smoothstep(-0.2,-0.1,day)*color;
}
float3 AtmosphereScattering(float3 background,float3 marchPos,float4 marchStep,float3 ray,
 float3 lightStrength,float3 lightDirection,float strength,float4 sphere){
    float3 intensity=0;float ang=dot(ray,lightDirection),mie=MieScattering(ang);float3 raylei=rayleighScattering(ang);
    if(marchStep.w>0.015)marchStep/=marchStep.w/0.015;float2 dv=0;
    [loop]for(int i=0;i<15;i++){float3 smp=marchPos;float4 sms=sphereCast(marchPos,lightDirection,sphere,lightStep,smp);
      float2 sampling=Density(marchPos,sphere,3.0+g_AtmDense,1.0)*marchStep.w;dv+=sampling/2;float2 dl=dv;
      [unroll]for(int j=0;j<3;j++){smp+=sms.xyz;dl+=Density(smp,sphere,3.0+g_AtmDense,1.0)*sms.w;}
      intensity+=LightDecay(dl.x,dl.y)*(raylei*sampling.x+mie*sampling.y);dv+=sampling/2;marchPos+=marchStep.xyz;}
    return lightStrength*intensity*strength+background*LightDecay(dv.x,dv.y);
}
float3 atmosphere_scattering(float strength,float3 color,float3 camera,float3 ray,float distance,float3 sunDirection,float4 sphere){
    if(distance<200)return color;float fade=smoothstep(200,300,distance);float4 step=0;
    step.w=15.0*distance/atmosphereStep/game2atm;step.xyz=ray*step.w;float3 lightDir=sunDirection;
    float3 light=LightSource(sunDirection,fDayProgression,1.0,lightDir);
    float3 scattered=AtmosphereScattering(color,Game2Atm_Alt(camera),step,ray,light,lightDir,1.0,sphere);
    return lerp(color,scattered,fade);
}
)CLOUD_HLSL"
R"CLOUD_HLSL(

struct CloudBaseColor {float3 BaseColor;float3 BaseColor_Day;float3 BaseColor_Sunset;};
struct CloudProfile {
 float4 march;float2 cutoff;float2 volumeBox;float4 shape;float brightness;float3 range;
 float2 solidness;float2 densityChunk;float4 shadow;float4 distortion;float fade;
 float3 densityDetail;float3 scaleChunk;float3 scaleDetail;float3 cloudShift;
 float3 offsetA;float3 offsetB;float3 offsetC;float3 offsetD;
};
float gameTime(){return 1000.0+g_Time*g_CloudSpeed/100.0;}
float3 PosOnPlane(float3 o,float3 d,float h,inout float dist){dist=(h-o.z)/d.z;return o+d*dist;}
float4 CloudShape(float z,float4 shape,float3 range){float soft=map(z,shape.y,shape.x,range.z,range.y);
 return float4(smoothstep(shape.z,lerp(shape.y,shape.z,shape.w),z)*smoothstep(shape.x,lerp(shape.y,shape.x,shape.w),z),range.x+soft,range.x-soft,soft);}
float3 DistortionVec(float lump,float4 d){return float3(cos(lump*d.x)*d.y,0,-lump*d.z);}
float Chunk(float3 pos,float2 density,float3 scale,float3 shift,float3 oA,float3 oB,float cs){
 pos.z/=scale.z;pos+=shift*pos.z;float3 pA=(pos+oA)*scale.x,pB=(pos+oB)*scale.y;
 return noise3d(pA)*(noise3d(pB)*density.y+density.x)*cs;}
float DetailA(float3 pos,float3 density,float3 scale,float3 oC,float3 dist){return density.x*noise3d((pos+oC+dist)*scale.x);}
float DetailB(float lump,float3 pos,float3 density,float3 scale,float4 dp,float3 oC,float3 oD,float cs){
 float3 d=DistortionVec(lump,dp),pD=pos+oD;float dens=DetailA(pos,density,scale,oC,d);d.z-=dens*dp.w;
 dens+=density.y*noise3d((pD+d/3)*scale.y);dens+=dens*density.z*noise3d((pD+d*8)*scale.z);return dens;}
float GetDensity(float df,float height,float low,float high,float2 vb,float2 sol){return clampMap(df,low,high,0,clampMap(height,vb.y,vb.x,sol.y,sol.x));}
float ShadowMarching(float dens,float3 p,CloudProfile a,float3 threshold,float3 sunDir){
 uint shadowSteps=(g_Settings>>8)&15u;
 if(dens<=0.025)return dens*a.shadow.x;
 // ProperShaders used four large samples for the DX9/SM3 instruction budget.
 // DX12 can split the same physical shadow-march reach into up to eight
 // samples, removing the dark density contours without changing its extent.
 float stepLen=a.shadow.x*(8.0/max((float)shadowSteps,1.0));
 float limit=2.0/a.shadow.w/stepLen,d=0;float4 st=float4(sunDir*stepLen,stepLen);
 [loop]for(uint i=0;i<8;i++){if(i>=shadowSteps||d>=limit||p.z>=a.volumeBox.x||p.z<=a.volumeBox.y)break;
  p+=st.xyz;float4 cs=CloudShape(p.z,a.shape,threshold);float d1=Chunk(p,a.densityChunk,a.scaleChunk,a.cloudShift,a.offsetA,a.offsetB,cs.x);
  float d2=DetailA(p,a.densityDetail,a.scaleDetail,a.offsetC,DistortionVec(d1,a.distortion))*a.shadow.y;
  d+=GetDensity(d1*d2+d1,p.z,cs.z-a.shadow.z,cs.y,a.volumeBox,a.solidness);}
 return d*a.shadow.w*st.w;}

float4 CloudAtRay(CloudProfile a,CloudBaseColor b,float3 dir,float3 cam,float3 light,float3 lightDir,float time,inout float distance){
 float4 d=float4(0,0,0,a.march.y);if(abs(dir.z)<1e-6)return float4(0,0,0,1);
 float3 p=PosOnPlane(cam,dir,clamp(cam.z,a.volumeBox.y+0.001,a.volumeBox.x-0.001),d.x);d.y=d.x;
 if(d.x>=0&&distance>d.x){a.range.x=1/a.range.x;float3 fx=float3(0,0,1);float last=0,pdf=0;
  [loop]for(int i=0;i<64;i++){if(fx.z<=0||p.z>a.volumeBox.x||p.z<a.volumeBox.y||i>=(int)a.march.w||d.x-d.w>=distance||d.x>=a.fade)break;
   float3 cs=CloudShape(p.z,a.shape,a.range).xyz;float d1=Chunk(p,a.densityChunk,a.scaleChunk,a.cloudShift,a.offsetA,a.offsetB,cs.x);
   float d2=DetailB(d1,p,a.densityDetail,a.scaleDetail,a.distortion,a.offsetC,a.offsetD,cs.x);float df=d1*d2+d1;
   if(df>cs.z){float dens=GetDensity(df,p.z,cs.z,cs.y,a.volumeBox,a.solidness);float cd=(dens+last)*a.march.x/2;last=dens;
    if(d.x>=distance)cd*=d.z/d.w;if(cd>0)d.y=d.y*(1-fx.z)+fx.z*d.x;fx.y+=cd;fx.z=(exp(-fx.y)-a.cutoff.y)/(1-a.cutoff.y);d.z=distance-d.x;
    if(fx.y<2.3)fx.x+=cd*exp(-ShadowMarching(cd,p,a,a.range,lightDir)-fx.y);}
   d.w=clampMap(2*df-pdf,cs.z*0.85,a.cutoff.x,a.march.x,a.march.y);d.w*=clampMap(d.x,0,a.fade,1,a.march.z);
   // The stochastic step offset is intentional. ProperShaders averages it in
   // the reprojected temporal history instead of exposing one undersampled
   // march directly on screen.
   d.w+=noise2d(p+g_Time)*a.march.x;pdf=df;p+=dir*d.w;d.x+=d.w;}
  if(fx.z<1){fx=saturate(fx);float3 z=float3(0,0,cam.z);float3 cbright=SunLight(light,Game2Atm(z+dir*d.y),lightDir,earth)*a.brightness;
   float3 C=cbright*fx.x*MiePhase(dot(lightDir,dir))+(b.BaseColor*g_vCloudBaseColor)*(1-fx.z);
   C=atmosphere_scattering(1-fx.z,C,Game2Atm(z),dir,d.y/game2atm,lightDir,earth);
   distance=distance*fx.z+d.y*(1-fx.z);return float4(C,fx.z);}}
 return float4(0,0,0,1);}

float4 CloudAtRayHighLite(CloudBaseColor b,float3 dir,float3 cam,float3 light,float3 lightDir,float time,inout float distance){
 const float top=3600.0,bottom=3500.0;float4 d=float4(0,0,0,75);
 float3 p=PosOnPlane(cam,dir,clamp(cam.z,bottom+.001,top-.001),d.x);d.y=d.x;
 if(d.x>=0&&distance>d.x){
  float coverage=g_CloudCoverage.z;float3 range=float3(1+coverage*.3,.2,.35);
  float grow=noise3d(float3(3800,bottom,time/2000))*.45+.65;
  range.x*=grow*(1-coverage)+coverage;range.x=1/range.x;
  const float2 volumeBox=float2(top,bottom),solidness=float2(.25,0),densityChunk=float2(.4,.3);
  const float3 scaleChunk=float3(.00016,.0008,1.5),densityDetail=float3(.2,.1,.6),scaleDetail=float3(.004,.006667,.02);
  const float4 distortion=float4(2.5,15000,0,0);
  float3 oA=float3(1.3,-1.8,0)*-time,oB=float3(1.6,.8,0)*-time;
  float3 oC=float3(2.5,.2,.5)*-time,oD=float3(3,.1,-.1)*-time;
  float3 fx=float3(0,0,1);float last=0,pdf=0;
  [loop]for(int i=0;i<32;i++){
   if(fx.z<=0||p.z>top||p.z<bottom||d.x-d.w>=distance||d.x>=60000)break;
   float3 cs=CloudShape(p.z,float4(3800,3520,3450,0),range).xyz;
   float d1=Chunk(p,densityChunk,scaleChunk,0,oA,oB,cs.x);
   float d2=DetailB(d1,p,densityDetail,scaleDetail,distortion,oC,oD,cs.x);float df=d1*d2+d1;
   if(df>cs.z){float dens=GetDensity(df,p.z,cs.z,cs.y,volumeBox,solidness);float cd=(dens+last)*2.5;last=dens;
    if(d.x>=distance)cd*=d.z/d.w;if(cd>0)d.y=d.y*(1-fx.z)+fx.z*d.x;
    fx.y+=cd;fx.z=(exp(-fx.y)-.2)/.8;d.z=distance-d.x;if(fx.y<2.3)fx.x+=cd*exp(-fx.y);}
   d.w=clampMap(2*df-pdf,cs.z*.85,0,5,75);d.w*=clampMap(d.x,0,2000000,1,500);
   d.w+=noise2d(p+g_Time)*5;pdf=df;p+=dir*d.w;d.x+=d.w;}
  if(fx.z<1){fx=saturate(fx);float3 cbright=light*(.5/LightingDecay);
   float3 C=cbright*fx.x*MiePhase(dot(lightDir,dir))+(b.BaseColor*g_vCloudBaseColor)*(1-fx.z);
   distance=distance*fx.z+d.y*(1-fx.z);return float4(C,fx.z);}}
 return float4(0,0,0,1);}

CloudProfile BuildProfile0(float time,float coverage){CloudProfile p=(CloudProfile)0;
 p.march=float4(5,80,8,64);p.cutoff=float2(0,.2);p.volumeBox=float2(900,500);p.shape=float4(900,650,0,0);p.brightness=.5;
 p.range=float3(.9+coverage*.16,.1,.2+coverage*.4);p.solidness=float2(5,0)*coverage;p.densityChunk=float2(.3,.5);
 p.shadow=float4(60,1.75,.1,.03);p.distortion=float4(1.6,60,8,16);p.fade=6000;p.densityDetail=float3(.3,.2,.6);
 p.scaleChunk=float3(.0008,.005,1);p.scaleDetail=float3(.02,.04,.1);p.cloudShift=float3(-.5,0,0);
 p.offsetA=float3(1.8,-1,0)*-time;p.offsetB=float3(2,.2,0)*-time;p.offsetC=float3(3,0,.5)*-time;p.offsetD=float3(3.5,0,-.1)*-time;
 float grow=noise3d(float3(p.shape.x,p.volumeBox.y,time/2000))*.45+.65;p.range.x*=grow*(1-coverage)+coverage;return p;}
CloudProfile BuildProfile1(float time,float coverage){CloudProfile p=(CloudProfile)0;
 p.march=float4(12,70,8,64);p.cutoff=float2(0,.2);p.volumeBox=float2(1900,1500);p.shape=float4(2100,1650,0,0);p.brightness=.5;
 p.range=float3(.85+coverage*.78,0,.3+coverage*.16);p.solidness=float2(.35,.1)*coverage;p.densityChunk=float2(.25,.6);
 p.shadow=float4(30,1,.15,.1);p.distortion=float4(6,50,100,50);p.fade=20000;p.densityDetail=float3(.5,.25,.5);
 p.scaleChunk=float3(.0008,.004,1.5);p.scaleDetail=float3(.0142857,.0285714,.08);p.cloudShift=0;
 p.offsetA=float3(1.5,-1.2,0)*-time;p.offsetB=float3(1.9,.5,0)*-time;p.offsetC=float3(2.5,0,.5)*-time;p.offsetD=float3(3,.1,-.1)*-time;
 float grow=noise3d(float3(p.shape.x,p.volumeBox.y,time/2000))*.45+.65;p.range.x*=grow*(1-coverage)+coverage;return p;}
CloudProfile BuildProfile2(float time,float coverage){CloudProfile p=(CloudProfile)0;
 p.march=float4(5,75,500,50);p.cutoff=float2(0,.2);p.volumeBox=float2(3600,3500);p.shape=float4(3800,3520,3450,0);p.brightness=.5;
 p.range=float3(1+coverage*.9,.2,.35);p.solidness=float2(.25,0);p.densityChunk=float2(.4,.3);
 p.shadow=float4(50,1.5,.02,.1);p.distortion=float4(2.5,15000,0,0);p.fade=2000000;p.densityDetail=float3(.2,.1,.6);
 p.scaleChunk=float3(.00016,.0008,1.5);p.scaleDetail=float3(.004,.006667,.02);p.cloudShift=0;
 p.offsetA=float3(1.3,-1.8,0)*-time;p.offsetB=float3(1.6,.8,0)*-time;p.offsetC=float3(2.5,.2,.5)*-time;p.offsetD=float3(3,.1,-.1)*-time;
 float grow=noise3d(float3(p.shape.x,p.volumeBox.y,time/2000))*.45+.65;p.range.x*=grow*(1-coverage)+coverage;return p;}
CloudBaseColor GetCloudsColor(float3 sunDir){CloudBaseColor b=(CloudBaseColor)0;b.BaseColor=.2;b.BaseColor_Day=.2;b.BaseColor_Sunset=.2;
 float night=smoothstep(.3,.1,fDayProgression),day=smoothstep(-.03,.05,fDayProgression),sunset=night*day;
 b.BaseColor+=lerp(b.BaseColor_Day,b.BaseColor_Sunset,sunset)*day;return b;}
float4 RenderClouds(float3 dir,float3 cam){float time=gameTime();CloudBaseColor base=GetCloudsColor(vSunLightDir);
 float3 lightDir=normalize(vSunLightDir);float3 light=LightSource(lightDir,fDayProgression,g_vSunColor,lightDir);float distance=100000;
 float4 result=CloudAtRay(BuildProfile0(time,g_CloudCoverage.x),base,dir,cam,light,lightDir,time,distance);
 uint layers=g_Settings&3u;if(layers>=3u&&result.w>.01){float4 mid=CloudAtRay(BuildProfile1(time,g_CloudCoverage.y),base,dir,cam,light,lightDir,time,distance);result.rgb+=mid.rgb*result.w;result.w*=mid.w;}
 if(layers>=2u&&result.w>.01){float4 high=CloudAtRayHighLite(base,dir,cam,light,lightDir,time,distance);result.rgb+=high.rgb*result.w;result.w*=high.w;}
 return result;}
float3 WorldRay(float2 uv){float2 ndc=float2(uv.x*2-1,1-uv.y*2);return normalize(CloudRayForward+CloudRayRight*ndc.x+CloudRayUp*ndc.y);}

static const float VC_TEMPORAL_DIV=2.0;
static const float VC_INV_TEMPORAL_DIV=0.5;
static const float VC_RAIL_FLOOR=4.0;

// A sparse march target contains one texel for each 2x2 block of the resolved
// cloud history. SV_POSITION identifies the sparse texel exactly; deriving UV
// from it avoids interpolation and half-pixel disagreements between APIs.
float2 CloudMarchUV(float2 position){
 float2 pixel=floor(position);
 float2 sparseUV=(pixel*VC_TEMPORAL_DIV+CloudSubPixel+0.5)*CloudTexelSize;
 float2 fullUV=(pixel+0.5)*CloudTexelSize;
 return lerp(sparseUV,fullUV,saturate(CloudFullResolutionMarch));
}

// History stores CloudWorks' native representation: premultiplied scattered
// light in rgb and TRANSMITTANCE in alpha (clear sky is 0,0,0,1).
float4 CloudMarchPS(PresentVertexOutput i):SV_TARGET {
 if((g_Settings&0x10000u)==0u)return float4(0,0,0,1);
 float3 dir=WorldRay(CloudMarchUV(i.position.xy));
 return RenderClouds(dir,CloudCameraPosition);
}

// Kept as an entry-point alias while the backend moves from its former direct
// cloud target to the full/sparse march paths.
float4 CloudTargetPS(PresentVertexOutput i):SV_TARGET {
 return CloudMarchPS(i);
}

// Reproject a current world direction into the previous camera's ray basis.
// If d = q.x*R + q.y*U + q.z*F, previous NDC is q.xy/q.z. Cramer's rule keeps
// this exact for asymmetric projections and avoids a separate 4x4 matrix.
float3 PreviousCloudNdc(float3 dir){
 float det=dot(PrevCloudRayRight,cross(PrevCloudRayUp,PrevCloudRayForward));
 float safeDet=(abs(det)>1e-8)?det:((det<0)?-1e-8:1e-8);
 float3 q=float3(
  dot(dir,cross(PrevCloudRayUp,PrevCloudRayForward)),
  dot(PrevCloudRayRight,cross(dir,PrevCloudRayForward)),
  dot(PrevCloudRayRight,cross(PrevCloudRayUp,dir)))/safeDet;
 return float3(q.xy/max(q.z,1e-8),q.z);
}

float CloudSlabEntry(float cameraZ,float rayZ,float bottom,float top){
 if(abs(rayZ)<1e-6)return 50000.0;
 if(cameraZ>=bottom&&cameraZ<=top)return 0.0;
 float a=(bottom-cameraZ)/rayZ,b=(top-cameraZ)/rayZ;
 float entry=min(a,b),leave=max(a,b);
 return leave<0.0?50000.0:max(entry,0.0);
}

// Reproject translation as well as rotation. CloudPadding carries the previous
// camera position. A world point on the nearest enabled physical cloud slab is
// stable under the orbiting VCS third-person camera, unlike a direction-only
// reprojection that forces the history to reset whenever the camera moves.
float3 PreviousCloudDirection(float2 uv){
 float3 dir=WorldRay(uv);
 float distance=CloudSlabEntry(CloudCameraPosition.z,dir.z,500.0,900.0);
 if((g_Settings&3u)>=2u)
  distance=min(distance,CloudSlabEntry(CloudCameraPosition.z,dir.z,3500.0,3600.0));
 if((g_Settings&3u)>=3u)
  distance=min(distance,CloudSlabEntry(CloudCameraPosition.z,dir.z,1500.0,1900.0));
 distance=min(max(distance,1.0),50000.0);
 float3 worldPoint=CloudCameraPosition+dir*distance;
 return normalize(worldPoint-CloudPadding);
}

// Literal SM5 adaptation of ProperShaders' PS_TemporalResolve. The march fills
// one Bayer slot per 2x2 block; every other pixel keeps its own reprojected
// history and leaks slightly toward the smooth current-frame reconstruction.
float4 CloudTemporalResolvePS(PresentVertexOutput i):SV_TARGET {
 float2 pixel=floor(i.position.xy);
 float2 uv=(pixel+0.5)*CloudTexelSize;
 float2 marchTexel=CloudTexelSize*VC_TEMPORAL_DIV;

 float2 block=floor(pixel*VC_INV_TEMPORAL_DIV);
 float2 slot=pixel-block*VC_TEMPORAL_DIV;
 float2 blockUV=(block+0.5)*marchTexel;
 float4 c=CloudTexture0.SampleLevel(CloudSampler,blockUV,0.0);

 float2 freshDelta=abs(slot-CloudSubPixel);
 float fresh=step(freshDelta.x+freshDelta.y,0.5);

 float2 spatialUV=uv-(CloudSubPixel-(VC_TEMPORAL_DIV-1.0)*0.5)*CloudTexelSize;
 float2 sp=spatialUV/marchTexel-0.5;
 float2 sf=frac(sp);
 float2 sb=(floor(sp)+0.5)*marchTexel;
 float4 t00=CloudTexture0.SampleLevel(CloudSampler,sb,0.0);
 float4 t10=CloudTexture0.SampleLevel(CloudSampler,sb+float2(marchTexel.x,0),0.0);
 float4 t01=CloudTexture0.SampleLevel(CloudSampler,sb+float2(0,marchTexel.y),0.0);
 float4 t11=CloudTexture0.SampleLevel(CloudSampler,sb+marchTexel,0.0);
 float4 spatial=lerp(lerp(t00,t10,sf.x),lerp(t01,t11,sf.x),sf.y);

 float3 previous=PreviousCloudNdc(PreviousCloudDirection(uv));
 float2 prevUV=float2(previous.x*0.5+0.5,0.5-previous.y*0.5);
 float2 inside=step(float2(0,0),prevUV)*step(prevUV,float2(1,1));
 float valid=CloudHistoryValid*inside.x*inside.y*step(1e-8,previous.z);
 float4 hist=CloudTexture1.SampleLevel(CloudSampler,prevUV,0.0);

 // Loose safety rail from ProperShaders. RGB and transmittance must move as a
 // single value; clamping channels independently creates black pinholes.
 float4 mn=min(min(min(t00,t10),min(t01,t11)),c);
 float4 mx=max(max(max(t00,t10),max(t01,t11)),c);
 float4 mid=(mn+mx)*0.5;
 float3 spanRGB=mx.rgb-mn.rgb;
 float span=max(max(spanRGB.r,spanRGB.g),max(spanRGB.b,mx.a-mn.a));
 float ext=max(span*CloudClampExpand,VC_RAIL_FLOOR);
 float4 dev=abs(hist-mid)-ext;
 float over=max(max(dev.r,dev.g),max(dev.b,dev.a));
 hist=lerp(hist,mid,saturate(over));

 float4 src=lerp(spatial,c,fresh);
 float w=lerp(1.0-CloudSpatialMix,CloudTemporalBlend,fresh)*valid;
 return lerp(src,hist,w);
}

// Manual four-tap bilinear upscale matches ProperShaders even if the bound
// sampler is point-filtered. The backend blends this premultiplied result over
// the world target with ONE / INV_SRC_ALPHA.
float4 CloudCompositePS(PresentVertexOutput i):SV_TARGET {
 float2 texelPos=i.uv/CloudTexelSize-0.5;
 float2 f=frac(texelPos);
 float2 base=(floor(texelPos)+0.5)*CloudTexelSize;
 float4 c00=CloudTexture0.SampleLevel(CloudSampler,base,0.0);
 float4 c10=CloudTexture0.SampleLevel(CloudSampler,base+float2(CloudTexelSize.x,0),0.0);
 float4 c01=CloudTexture0.SampleLevel(CloudSampler,base+float2(0,CloudTexelSize.y),0.0);
 float4 c11=CloudTexture0.SampleLevel(CloudSampler,base+CloudTexelSize,0.0);
 float4 clouds=lerp(lerp(c00,c10,f.x),lerp(c01,c11,f.x),f.y);
 float alpha=saturate((1.0-clouds.a)*g_Opacity);
 return float4(clouds.rgb*g_Opacity*g_Brightness,alpha);
}
)CLOUD_HLSL";

} // namespace vcs
