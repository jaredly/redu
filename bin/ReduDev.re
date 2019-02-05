let (/+) = Filename.concat;
/* let assetDir = Filename.dirname(Sys.argv[0]) /+ ".." /+ ".." /+ ".." /+ "assets"; */
let assetDir = "." /+ "assets";
/* Redu.Api.debug := true; */
Redu.run(assetDir);
