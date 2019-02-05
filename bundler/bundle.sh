set -ex
echo Hi
rm -rf Redu.app
./macapp.sh Redu icon.png
DEST=Redu.app/Contents/MacOS
esy cp "#{self.target_dir / 'install' / 'default' / 'bin' / 'PhabradorProd'}" $DEST/Redu
cp -r ../assets   Redu.app/Contents/MacOS/
git rev-parse HEAD > Redu.app/Contents/MacOS/assets/git-head
zip -r Redu.zip Redu.app