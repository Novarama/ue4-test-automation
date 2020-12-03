﻿using Gauntlet;

namespace DaedalicTestAutomationPlugin.Automation
{
    public class DaeGauntletTestNet : DefaultTest
    {
        public DaeGauntletTest(UnrealTestContext InContext) : base(InContext)
        {
        }

        public override DaeTestConfig GetConfiguration()
        {
            DaeTestConfig Config = base.GetConfiguration();

            UnrealTestRole ServerRole = Config.RequireRole(UnrealTargetRole.Server);
            ServerRole.Controllers.Add("DaeGauntletTestNetController");

            IEnumerable<UnrealTestRole> ClientRoles = Config.RequireRoles(UnrealTargetRole.Client, 2);
            foreach (var Client in ClientRoles)
            {
                Client.ExplicitClientCommandLine = " -ExecCmds=\"open 127.0.0.1:7777\"";
            }

            // Ignore user account management.
            Config.NoMCP = true;

            return Config;
        }
    }
}
