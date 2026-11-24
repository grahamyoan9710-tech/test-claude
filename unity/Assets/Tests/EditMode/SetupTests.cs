using System.IO;
using NUnit.Framework;
using UnityEngine;

namespace ClashHorizon.Tests
{
    public class SetupTests
    {
        [Test]
        public void Setup_Works()
        {
            var sheets = Resources.Load<TextAsset>("Sprites/sheets");
            Assert.IsNotNull(sheets, "Sprites/sheets missing from Resources: run scripts/unity_sync_assets.sh");
            var repoFile = Path.Combine(Application.dataPath, "../../assets/sprites/sheets.txt");
            CollectionAssert.AreEqual(File.ReadAllBytes(repoFile), sheets.bytes);
        }
    }
}
