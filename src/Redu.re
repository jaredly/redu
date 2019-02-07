open FluidMac;
open Fluid.Native;

let str = Fluid.string;

open Fluid.Hooks;

type status = Initial | Processing | Processed;

let parseLine = line => line == "" ? None : {
  let numb = Buffer.create(10);
  let fileb = Buffer.create(100);
  let ln = String.length(line);
  let rec loop = (i, pos) => i >= ln ? {
    (Buffer.contents(numb)->int_of_string, Buffer.contents(fileb))
  } : switch (pos, line.[i]) {
    | (`Initial, '0'..'9') => Buffer.add_char(numb, line.[i]); loop(i + 1, `Initial)
    | (`Initial, _) => loop(i, `Mid)
    | (`Mid, ' ' | '\t') => loop(i + 1, `Mid)
    | (`Mid, _) => loop(i, `Last)
    | (`Last, m) => Buffer.add_char(fileb, m); loop(i + 1, `Last)
  };
  Some(loop(0, `Initial))
};

let%component main = (hooks) => {
  let%hook (status, setStatus) = useState(Initial);
  let%hook (directory, setDirectory) = useState("/Users/jared/Downloads");
  let%hook (depth, setDepth) = useState(3);
  let%hook (results, setResults) = useState("");
  let%hook cancel = useRef(() => ());
  let%hook start = useCallback(() => {
    setStatus(Processing);
    print_endline("Starting I guess");
    Commands.execCommand("/usr/bin/du", ["-mHd" ++ string_of_int(depth), directory], ((stdout, stderr, exitCode)) => {
      Printf.printf("OUT:\n%s\nERR:\n%s\nCODE: %d\n", stdout, stderr, exitCode);
      print_endline("Did it!")
      let lines = String.split_on_char('\n', stdout)
      ->Belt.List.keepMap(parseLine)
      ->Belt.List.sort((a, b) => fst(b) - fst(a))
      ->Belt.List.map(((size, name)) => Printf.sprintf("%d\t\t%s", size, name))
      |> String.concat("\n");
      setResults(lines);
      setStatus(Processed);
    });
    ()
  }, (depth, directory));
  let (action, actionTitle) = switch status {
    | Initial => (start, "Analyze")
    | Processing => (cancel.contents, "Cancel")
    | Processed => (start, "Re-run analysis")
  };
  <view>
    <view layout={Layout.style(~flexDirection=Row, ())}>
      <text contents=directory />
      <button onPress={() => {
        Commands.openDirectory(directory, setDirectory);
      }} title={"Select directory"} />
      {str("Depth: " ++ string_of_int(depth))}
      <button onPress={() => setDepth(max(1, depth - 1))} title="-" />
      <button onPress={() => setDepth(depth + 1)} title="+" />
      <button onPress={action} title={actionTitle} />
    </view>
    <scrollView
      layout={Layout.style(
        ~flexShrink=1.,
        ~alignItems=AlignStretch,
        ~alignSelf=AlignStretch,
        ~minHeight=50.,
        ~maxHeight=200.,
        ~width=500.,
        ~overflow=Scroll,
        (),
      )}
    >
      <text contents=results layout={Layout.style(~alignSelf=AlignStretch, ())} />
    </scrollView>
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
      <main />
    )
  });
}