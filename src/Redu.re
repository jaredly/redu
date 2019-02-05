open FluidMac;
open Fluid.Native;

let str = Fluid.string;

open Fluid.Hooks;

type status = Initial | Processing | Processed;

let%component main = (~a, hooks) => {
  let%hook (status, setStatus) = useState(Initial);
  let%hook cancel = useRef(() => ());
  let%hook start = useCallback(() => {
    print_endline("Starting I guess");
    ()
  }, ());
  let (action, actionTitle) = switch status {
    | Initial => (start, "Start at $HOME")
    | Processing => (cancel.contents, "Cancel")
    | Processed => (start, "Re-run analysis")
  };
  <view
  >
    {str("Finding all your large directories n stuff")}
    <button onPress={action} title={actionTitle} />
  </view>
};

let run = assetsDir => {
  Fluid.App.launch(() => {
    Fluid.App.setupAppMenu(
      ~title="Redu",
      ~appItems=[||],
      ~menus=[| Fluid.App.defaultEditMenu() |]
    );
    let win = Fluid.launchWindow(
      ~title="Redu",
      <main a=1 />
    )
  });
}