git config --local user.email "41898282+github-actions[bot]@users.noreply.github.com"
git config --local user.name "github-actions[bot]"
git checkout --orphan latest-build

mkdir $1
cp ./bin/ezld $1/ezld
cp -r ./bin/plugins $1/plugins
cp ./bin/plugin-sdk.o $1/plugin-sdk.o

mkdir ezld
cp ./bin/ezld ./ezld/ezld
tar -cvzf ./$1/ezld-$1.tar.gz ./ezld
rm -rf ./ezld

git add $1/
