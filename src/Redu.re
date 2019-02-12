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

type state = {
  root: string,
  sizes: Belt.Map.String.t(int),
  children: Belt.Map.String.t(list(string)),
  loading: Belt.Set.String.t,
};

let sortChildren = (children, sizes) =>
  children->Belt.List.sort((a, b) =>
    switch (sizes->Belt.Map.String.get(a), sizes->Belt.Map.String.get(b)) {
    | (Some(a), Some(b)) => b - a
    | (Some(a), None) => 1
    | (None, Some(a)) => (-1)
    | (None, None) => 0
    }
  );

let loadFolder = (state, folder) => {
  let children =
    Files.readDirectory(folder)
    ->Belt.List.map(name => Filename.concat(folder, name));
  let sizes =
    children->Belt.List.reduce(state.sizes, (sizes, full) =>
      if (Files.isFile(full)) {
        switch (Files.maybeStat(full)) {
        | None => sizes
        | Some(stat) =>
          sizes->Belt.Map.String.set(full, stat.Unix.st_size / 1000)
        };
      } else {
        sizes;
      }
    );
  {
    ...state,
    sizes,
    children:
      state.children
      ->Belt.Map.String.set(folder, sortChildren(children, sizes)),
  };
};

let reduce = (state, action) => {
  switch (state, action) {
    | (_, `SetRoot(root)) => 
      Some(loadFolder({root, sizes: Belt.Map.String.empty, children: Belt.Map.String.empty, loading: Belt.Set.String.empty}, root))
    | (Some(state), `Load(path)) =>
      print_endline("Loading " ++ path ++ " current count " ++ string_of_int(Belt.Map.String.size(state.children)))
      Some(loadFolder(state, path))
    | (Some(state), `LoadSizes(path, sizes)) =>
      print_endline("Loading Sizes current count " ++ string_of_int(Belt.Map.String.size(state.children)));
      let sizes = sizes->Belt.List.reduce(state.sizes, (sizes, (size, path)) => sizes->Belt.Map.String.set(path, size));
      let children =
        switch (state.children->Belt.Map.String.get(path)) {
        | None => state.children
        | Some(children) =>
          state.children->Belt.Map.String.set(path, sortChildren(children, sizes))
        };
      Some({
        ...state,
        children,
        sizes,
      })
  }
};

let getSizes = (base, onDone) => {
  print_endline("Getting for " ++ base);
  Commands.execCommand("/usr/bin/du", ["-kHd1", base], ((stdout, stderr, exitCode)) => {
    Printf.printf("OUT:\n%s\nERR:\n%s\nCODE: %d\n", stdout, stderr, exitCode);
    print_endline("Did it!")
    let lines = String.split_on_char('\n', stdout)
    ->Belt.List.keepMap(parseLine)
    ->Belt.List.sort((a, b) => fst(b) - fst(a));
    onDone(lines)
    /* |> String.concat("\n"); */
    /* setResults(lines); */
    /* setStatus(Processed); */
  });
};

let showItem = (state, path) => {
  let path = path == "" ? state.root : path;
  let name = Filename.basename(path);
  let size = switch (state.sizes->Belt.Map.String.get(path)) {
    | None => "%%"
    | Some(size) =>
      if (size < 100) {
        Printf.sprintf("%0.2dk", size)
      } else if (size < 100_000) {
        Printf.sprintf("%0.2fm", float_of_int(size) /. 1000.)
      } else {
        Printf.sprintf("%0.2fg", float_of_int(size) /. 1000_000.)
      }
  };
  size ++ "   " ++ name

  /* name ++ "  \t" ++ size */
    /* | Some(size) => string_of_float(float_of_int(size) /. 1000.) ++ "M"
  } */
};

let%component columned = hooks => {
  let%hook (state, dispatch) = useReducer(None, reduce);

  <view layout={Layout.style(~minWidth=200., ~minHeight=200., ~alignItems=AlignCenter, ())}>
    <button onPress={() => {
      Commands.openDirectory("~/", path => {
        dispatch(`SetRoot(path));
        getSizes(path, sizes => dispatch(`LoadSizes(path, sizes)));
      });
    }} title={"Select directory"} />
    {
      switch state {
        | None => <view />
        | Some(state) =>
          <view>
            <text contents=state.root />
            <columnBrowser
              isLeafItem={path => !Files.isDirectory(path == "" ? state.root : path)}
              childOfItem={((path, index)) => {
                let path = path == "" ? state.root : path;
                switch (state.children->Belt.Map.String.get(path)) {
                | None =>
                  print_endline("Not found child " ++ path);
                  ":not-found:"
                | Some(children) => switch (children->Belt.List.get(index)) {
                  | None =>
                    print_endline("Not found child " ++ path);
                    ":not-found:"
                  | Some(child) => child
                }
              }}}
              displayForItem={showItem(state)}
              childrenCount={path => {
                let path = path == "" ? state.root : path;
                switch (state.children->Belt.Map.String.get(path)) {
                | None =>
                  dispatch(`Load(path));
                  getSizes(path, sizes => dispatch(`LoadSizes(path, sizes)));
                  /* print_endline("Not found count " ++ path); */
                  0
                | Some(children) => List.length(children)
              }}}
              layout={Layout.style(~width=600., ~height=300., ())}
            />
        </view>
      }
    }
  </view>
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
      <columned />
    )
  });
}