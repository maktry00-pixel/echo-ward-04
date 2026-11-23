using UnrealBuildTool;
public class EchoWard : ModuleRules
{
    public EchoWard(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "Json", "JsonUtilities", "RHI", "RenderCore", "Slate", "SlateCore", "PhysicsCore", "UMG", "HairStrandsCore", "MeshDescription", "StaticMeshDescription", "ProceduralMeshComponent" });
    }
}
