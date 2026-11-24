using UnityEditor;
using UnityEditor.Build.Reporting;

namespace ClashHorizon.Editor
{
    // Player builds for scripts/unity_build.sh. Windows uses Mono too: IL2CPP for Windows needs a Windows PC.
    public static class Build
    {
        public static void Linux() => Player(BuildTarget.StandaloneLinux64, "Builds/Linux/ClashHorizon.x86_64");
        public static void Windows() => Player(BuildTarget.StandaloneWindows64, "Builds/Windows/ClashHorizon.exe");

        static void Player(BuildTarget target, string path)
        {
            var report = BuildPipeline.BuildPlayer(new[] { ProjectSetup.BootScene }, path, target, BuildOptions.None);
            if (report.summary.result != BuildResult.Succeeded) EditorApplication.Exit(1);
        }
    }
}
