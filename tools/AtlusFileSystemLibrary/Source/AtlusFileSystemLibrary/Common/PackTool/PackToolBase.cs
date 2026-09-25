using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AtlusFileSystemLibrary.Common.PackTool
{
    public abstract class PackToolBase
    {
        public abstract string Usage { get; }

        public abstract IReadOnlyDictionary< string, ICommand > Commands { get; }

        public void ToolMain(string[] args)
        {
            if ( args.Length == 0 )
            {
                Console.WriteLine( Usage );
                return;
            }

            if ( !Commands.TryGetValue( args[0], out var command ) )
            {
                Console.WriteLine( "Invalid command specified." );
                return;
            }

            var commandArgs = new string[args.Length - 1];
            if (args.Length > 1)
                Array.Copy( args, 1, commandArgs, 0, commandArgs.Length );

            if ( command.Execute( commandArgs ) )
            {
                Console.WriteLine( "Command executed successfully." );
            }
            else
            {
                Console.WriteLine( "Command failed." );
            }
        }
    }

    public interface ICommand
    {
        bool Execute( string[] args );
    }
}
