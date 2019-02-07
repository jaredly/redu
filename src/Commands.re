
module CmdTracker = FluidMac.Tracker({type arg = (string, string, int); let name = "redu_execCommand_cb"; let once = true;});
external execCommand: (~cwd: string, ~cmd: string, array(string), CmdTracker.callbackId) => unit = "redu_execCommand";
let execCommand = (~cwd=".", ~cmd, args, cb) => execCommand(~cwd, ~cmd, Array.of_list(args), CmdTracker.track(cb));

external openDirectory: (string, FluidMac.StringTracker.callbackId) => unit = "redu_openDirectory";
let openDirectory = (current, cb) => openDirectory(current, FluidMac.StringTracker.track(cb));