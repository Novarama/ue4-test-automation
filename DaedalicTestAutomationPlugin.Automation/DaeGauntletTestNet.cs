using System.Collections.Generic;
using Gauntlet;
using UE4Game;

namespace DaedalicTestAutomationPlugin.Automation
{
    public class DaeGauntletTestNet : DefaultTest
    {
        public DaeGauntletTestNet(UnrealTestContext InContext) : base(InContext)
        {
        }

        public override UE4TestConfig GetConfiguration()
        {
            UE4TestConfig Config = base.GetConfiguration();

            UnrealTestRole ServerRole = Config.RequireRole(UnrealTargetRole.Server);
            //ServerRole.MapOverride = "TestMap";
            ServerRole.Controllers.Add("DaeGauntletTestNetController");

            IEnumerable<UnrealTestRole> ClientRoles = Config.RequireRoles(UnrealTargetRole.Client, 2);
            foreach (var Client in ClientRoles)
            {
                Client.ExplicitClientCommandLine = " -ExecCmds=\"open 127.0.0.1:7777\"";
            }

            // Ignore user account management.
            //Config.NoMCP = true;

            return Config;
        }
    }
}
