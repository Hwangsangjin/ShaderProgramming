//**************************************************************//
//  Effect File exported by RenderMonkey 1.6
//
//  - Although many improvements were made to RenderMonkey FX  
//    file export, there are still situations that may cause   
//    compilation problems once the file is exported, such as  
//    occasional naming conflicts for methods, since FX format 
//    does not support any notions of name spaces. You need to 
//    try to create workspaces in such a way as to minimize    
//    potential naming conflicts on export.                    
//    
//  - Note that to minimize resulting name collisions in the FX 
//    file, RenderMonkey will mangle names for passes, shaders  
//    and function names as necessary to reduce name conflicts. 
//**************************************************************//

//--------------------------------------------------------------//
// Lighting
//--------------------------------------------------------------//
//--------------------------------------------------------------//
// Pass 0
//--------------------------------------------------------------//
string Lighting_Pass_0_Model : ModelData = "..\\program files (x86)\\amd\\rendermonkey 1.82\\Examples\\Media\\Models\\Sphere.3ds";

float4x4 gWorldMatrix : World;
float4x4 gViewMatrix : View;
float4x4 gProjectionMatrix : Projection;

float4 gWorldLightPosition
<
   string UIName = "gWorldLightPosition";
   string UIWidget = "Direction";
   bool UIVisible =  false;
   float4 UIMin = float4( -10.00, -10.00, -10.00, -10.00 );
   float4 UIMax = float4( 10.00, 10.00, 10.00, 10.00 );
   bool Normalize =  false;
> = float4( 500.00, 500.00, -500.00, 1.00 );
float4 gWorldCameraPosition : ViewPosition;

struct VS_INPUT 
{
   float4 mPosition : POSITION;
   float3 mNormal: NORMAL;
};

struct VS_OUTPUT 
{
   float4 mPosition : POSITION;
   float3 mDiffuse : TEXCOORD1;
   float3 mViewDir: TEXCOORD2;
   float3 mReflection: TEXCOORD3;
};

VS_OUTPUT Lighting_Pass_0_Vertex_Shader_vs_main(VS_INPUT Input)
{
    VS_OUTPUT Output;

    Output.mPosition = mul(Input.mPosition, gWorldMatrix);

    float3 LightDir = Output.mPosition.xyz - gWorldLightPosition.xyz;
    LightDir = normalize(LightDir);
   
    float3 ViewDir = normalize(Output.mPosition.xyz - gWorldCameraPosition.xyz);
    Output.mViewDir = ViewDir;
   
    Output.mPosition = mul(Output.mPosition, gViewMatrix);
    Output.mPosition = mul(Output.mPosition, gProjectionMatrix);
   
    float3 WorldNormal = mul(Input.mNormal, (float3x3)gWorldMatrix);
    WorldNormal = normalize(WorldNormal);

    Output.mDiffuse = dot(-LightDir, WorldNormal);
    Output.mReflection = reflect(LightDir, WorldNormal);

   return Output;
}
struct PS_INPUT
{
    float3 mDiffuse : TEXCOORD1;
    float3 mViewDir: TEXCOORD2;
    float3 mReflection: TEXCOORD3;
};

float4 Lighting_Pass_0_Pixel_Shader_ps_main(PS_INPUT Input) : COLOR
{
    float3 Diffuse = saturate(Input.mDiffuse);
   
    float3 Reflection = normalize(Input.mReflection);
    float3 ViewDir = normalize(Input.mViewDir); 
    float3 Specular = 0;
    if (Diffuse.x > 0)
    {
        Specular = saturate(dot(Reflection, -ViewDir));
        Specular = pow(Specular, 20.0f);
    }

    float3 Ambient = float3(0.1f, 0.1f, 0.1f);
   
    return float4(Ambient + Diffuse + Specular, 1);
}
//--------------------------------------------------------------//
// Technique Section for Lighting
//--------------------------------------------------------------//
technique Lighting
{
   pass Pass_0
   {
      VertexShader = compile vs_2_0 Lighting_Pass_0_Vertex_Shader_vs_main();
      PixelShader = compile ps_2_0 Lighting_Pass_0_Pixel_Shader_ps_main();
   }

}

