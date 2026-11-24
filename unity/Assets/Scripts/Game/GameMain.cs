using System;
using System.Globalization;
using UnityEngine;

namespace ClashHorizon.Game
{
    // The game's only MonoBehaviour (app.cpp and main.cpp). P0 stub: only --quit-after FRAMES.
    public class GameMain : MonoBehaviour
    {
        int quitAfter = -1, frames;

        void Awake()
        {
            var args = Environment.GetCommandLineArgs();
            for (int i = 1; i + 1 < args.Length; ++i)
                if (args[i] == "--quit-after") int.TryParse(args[i + 1], NumberStyles.Integer, CultureInfo.InvariantCulture, out quitAfter);
        }

        void Update()
        {
            ++frames;  // like frames_ in app.cpp: quit at the end of frame quitAfter
            if (quitAfter > 0 && frames >= quitAfter) Application.Quit();
        }
    }
}
