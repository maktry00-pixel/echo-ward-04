using UnrealBuildTool;
using System.Collections.Generic;
public class EchoWardEditorTarget : TargetRules
{
    public EchoWardEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("EchoWard");
    }
}
