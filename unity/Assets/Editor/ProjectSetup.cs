using ClashHorizon.Game;
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

namespace ClashHorizon.Editor
{
    // Writes the generated project files: the URP asset, Boot.unity and the player settings. They are committed;
    // run this again after changing it (command in docs/unity/phases/P0.md).
    public static class ProjectSetup
    {
        public const string BootScene = "Assets/Boot.unity";
        const string UrpAsset = "Assets/Settings/URP.asset";

        public static void Run()
        {
            var urp = AssetDatabase.LoadAssetAtPath<UniversalRenderPipelineAsset>(UrpAsset);
            if (urp == null)  // made once, so a rerun keeps its GUID
            {
                if (!AssetDatabase.IsValidFolder("Assets/Settings")) AssetDatabase.CreateFolder("Assets", "Settings");
                var renderer = ScriptableObject.CreateInstance<UniversalRendererData>();
                renderer.postProcessData = AssetDatabase.LoadAssetAtPath<PostProcessData>(  // as URP's Create menu does
                    UniversalRenderPipelineAsset.packagePath + "/Runtime/Data/PostProcessData.asset");
                AssetDatabase.CreateAsset(renderer, "Assets/Settings/URP_Renderer.asset");
                urp = UniversalRenderPipelineAsset.Create(renderer);
                AssetDatabase.CreateAsset(urp, UrpAsset);
            }
            GraphicsSettings.defaultRenderPipeline = urp;

            PlayerSettings.productName = "Clash Horizon";  // the window title, like app.cpp
            PlayerSettings.colorSpace = ColorSpace.Linear;  // raw colour math: see the translation rules in PORT_PLAN.md
            PlayerSettings.fullScreenMode = FullScreenMode.Windowed;  // kScreenW x kScreenH, FLAG_WINDOW_RESIZABLE
            PlayerSettings.defaultScreenWidth = 1280;
            PlayerSettings.defaultScreenHeight = 720;
            PlayerSettings.resizableWindow = true;
            PlayerSettings.runInBackground = true;  // the C++ loop never pauses; Unity would freeze an unfocused window
            PlayerSettings.SplashScreen.show = false;
            PlayerSettings.SetScriptingBackend(NamedBuildTarget.Standalone, ScriptingImplementation.Mono2x);

            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene);
            var camera = new GameObject("Camera").AddComponent<Camera>();
            camera.clearFlags = CameraClearFlags.SolidColor;
            camera.backgroundColor = Color.black;
            new GameObject("GameMain").AddComponent<GameMain>();
            EditorSceneManager.SaveScene(scene, BootScene);
            EditorBuildSettings.scenes = new[] { new EditorBuildSettingsScene(BootScene, true) };
            AssetDatabase.SaveAssets();
        }
    }
}
